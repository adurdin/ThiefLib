//! \file Script.inl

/*  This file is part of ThiefLib, a library for Thief 1/2 script modules.
 *  Copyright (C) 2013-2014 Kevin Daughtridge <kevin@kdau.com>
 *  Adapted in part from Public Scripts and the Object Script Library
 *  Copyright (C) 2005-2013 Tom N Harris <telliamed@whoopdedo.org>
 *  Adapted in part from TWScript
 *  Copyright (C) 2012-2013 Chris Page <chris@starforge.co.uk>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef THIEF_SCRIPT_HH
#error "This file should only be included from <Thief/Script.hh>."
#endif

#ifndef THIEF_SCRIPT_INL
#define THIEF_SCRIPT_INL

namespace Thief {



// ScriptMessageHandler

template <typename _Script, typename _Message>
struct ScriptMessageHandler : public MessageHandler,
	private std::function<Message::Result (_Script&, _Message&)>
{
	typedef Message::Result (_Script::*Method) (_Message&);

	static Ptr create (Method method);

	virtual Message::Result handle (Script&, sScrMsg*, sMultiParm*);

private:
	ScriptMessageHandler (Method method);
};

template <typename _Script, typename _Message>
MessageHandler::Ptr
ScriptMessageHandler<_Script, _Message>::create (Method method)
{
	return Ptr (new ScriptMessageHandler (method));
}

template <typename _Script, typename _Message>
ScriptMessageHandler<_Script, _Message>::ScriptMessageHandler (Method method)
	: std::function<Message::Result (_Script&, _Message&)> (method)
{}

template <typename _Script, typename _Message>
Message::Result
ScriptMessageHandler<_Script, _Message>::handle (Script& script,
	sScrMsg* _message, sMultiParm* reply)
{
	_Message message (_message, reply);
	return (*this) (static_cast<_Script&> (script), message);
}



// Script

inline const String&
Script::name () const
{
	return script_name;
}

inline ScriptHost
Script::host () const
{
	return ScriptHost (host_obj);
}

template <typename T, typename>
inline T
Script::host_as () const
{
	return T (host_obj);
}

template <typename... Args>
inline void
Script::log (Log level, const String& _format, const Args&... args) const
{
	if (int (level) >= int (min_level))
	{
		boost::format format { _format };
		log_step (level, format, args...);
	}
}

template <typename T, typename... Args>
inline void
Script::log_step (Log level, boost::format& format, const T& arg,
	const Args&... args) const
{
	log_step (level, format % arg, args...);
}

inline void
Script::log_step (Log level, boost::format& format) const
{
	mono (level) << format.str () << std::endl;
}

template <typename _Script, typename _Message>
inline void
Script::listen_message (const CIString& message,
	Message::Result (_Script::*handler) (_Message&))
{
	message_handlers.emplace (message,
		ScriptMessageHandler<_Script, _Message>::create (handler));
}

template <typename _Script>
inline void
Script::listen_timer (const CIString& timer,
	Message::Result (_Script::*handler) (Message&))
{
	timer_handlers.emplace (timer,
		ScriptMessageHandler<_Script, Message>::create (handler));
}

template <typename _Script>
inline void
Script::listen_timer (const CIString& timer,
	Message::Result (_Script::*handler) (TimerMessage&))
{
	timer_handlers.emplace (timer,
		ScriptMessageHandler<_Script, TimerMessage>::create (handler));
}

inline Timer
Script::start_timer (const char* timer, Time delay, bool repeating)
{
	return _start_timer (timer, delay, repeating, LGMulti<Empty> ());
}

template <typename T>
inline Timer
Script::start_timer (const char* timer, Time delay, bool repeating,
	const T& data)
{
	return _start_timer (timer, delay, repeating, LGMulti<T> (data));
}



// Persistent

template <typename T>
inline
Persistent<T>::Persistent (Script& _script, const String& _name)
	: PersistentBase (_script, _name), has_default_value (false)
{}

template <typename T>
inline
Persistent<T>::Persistent (Script& _script, const String& _name,
		const T& _default_value)
	: PersistentBase (_script, _name), has_default_value (false)
{
	set_default_value (_default_value);
}

template <typename T>
inline
Persistent<T>::operator const T& () const
{
	get_value ();
	return value;
}

template <typename T>
inline T*
Persistent<T>::operator -> ()
{
	get_value ();
	return &value;
}

template <typename T>
inline const T*
Persistent<T>::operator -> () const
{
	get_value ();
	return &value;
}

template <typename T>
inline bool
Persistent<T>::operator == (const T& rhs) const
{
	get_value ();
	return value == rhs;
}

template <typename T>
inline bool
Persistent<T>::operator != (const T& rhs) const
{
	get_value ();
	return value != rhs;
}

template <typename T>
inline Persistent<T>&
Persistent<T>::operator = (const T& _value)
{
	value = _value;
	set (LGMulti<T> (value));
	return *this;
}

template <typename T>
inline void
Persistent<T>::set_default_value (const T& _default_value)
{
	default_value = _default_value;
	has_default_value = true;
}

template <typename T>
inline void
Persistent<T>::get_value () const
{
	if (has_default_value && !exists ())
		value = default_value;
	else
	{
		LGMulti<T> _value;
		get (_value);
		value = _value;
	}
}



// Transition

template <typename _Script>
inline
Transition::Transition (_Script& _host, bool (_Script::*_step_method) (),
		const String& _name, Time _resolution, Time default_length,
		Curve default_curve, const CIString& length_param,
		const CIString& curve_param)
	: length (_host.host (), length_param, { default_length }),
	  curve (_host.host (), curve_param, { default_curve }),
	  host (_host), step_method (std::bind (_step_method, &_host)),
	  name (_name), resolution (_resolution),
	  timer (host, "transition_timer_" + name),
	  remaining (host, "transition_remaining_" + name)
{
	initialize ();
}

template <typename T>
inline THIEF_INTERPOLATE_RESULT (T)
Transition::interpolate (const T& from, const T& to) const
{
	return Thief::interpolate (from, to, get_progress (), curve);
}

template <typename T>
inline T
Transition::interpolate (const Persistent<T>& from,
	const Persistent<T>& to) const
{
	return Thief::interpolate (T (from), T (to), get_progress (), curve);
}



} // namespace Thief

#endif // THIEF_SCRIPT_INL

