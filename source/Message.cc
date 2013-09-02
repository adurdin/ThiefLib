/******************************************************************************
 *  Message.cc
 *
 *  This file is part of ThiefLib, a library for Thief 1/2 script modules.
 *  Copyright (C) 2013 Kevin Daughtridge <kevin@kdau.com>
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
 *
 *****************************************************************************/

#include "Private.hh"

namespace Thief {



// Timer

void
Timer::cancel ()
{
	if (id)
	{
		LG->KillTimedMessage (tScrTimer (id));
		id = nullptr;
	}
}



// Message

Message::Message (sScrMsg* _message, sMultiParm* _reply, bool)
	: message (_message),
	  reply (_reply ? _reply : new cMultiParm ()),
	  own_reply (!_reply)
{
	if (message)
		message->AddRef ();
	else
	{
		if (own_reply) delete reply;
		throw MessageWrapError (message, typeid (*this),
			"message is null");
	}
}

Message::Message (const Message& copy)
	: message (copy.message),
	  reply (copy.reply ? new cMultiParm (*copy.reply) : new cMultiParm ()),
	  own_reply (true)
{
	if (message)
		message->AddRef ();
	else
	{
		delete reply;
		throw MessageWrapError (message, typeid (*this),
			"message is null");
	}
}

Message::~Message ()
{
	if (message) message->Release ();
	if (own_reply) delete reply;
}

const char*
Message::get_name () const
{
	return message->message;
}

void
Message::send (const Object& from, const Object& to)
{
	message->from = from.number;
	message->to = to.number;
	LG->SendMessage (message, reply);
}

void
Message::post (const Object& from, const Object& to)
{
	message->from = from.number;
	message->to = to.number;
	LG->PostMessage (message);
}

Timer
Message::schedule (const Object& from, const Object& to,
	Time delay, bool repeating)
{
	message->from = from.number;
	message->to = to.number;
	return Timer (LG->SetTimedMessage (message, delay,
		repeating ? kSTM_Periodic : kSTM_OneShot));
}

void
Message::broadcast (const Links& links, Time delay)
{
	for (auto& link : links)
		if (delay > 0ul)
			schedule (link.get_source (), link.get_dest (),
				delay, false);
		else
			send (link.get_source (), link.get_dest ());
}

void
Message::broadcast (const Object& from, const Flavor& link_flavor, Time delay)
{
	broadcast (Link::get_all (link_flavor, from), delay);
}

Object
Message::get_from () const
{
	return message->from;
}

Object
Message::get_to () const
{
	return message->to;
}

Time
Message::get_time () const
{
	return message->time;
}

bool
Message::has_data (Datum datum) const
{
	switch (datum)
	{
	case DATA1: return message->data.type != kMT_Undef;
	case DATA2: return message->data2.type != kMT_Undef;
	case DATA3: return message->data3.type != kMT_Undef;
	default: return false;
	}
}

void
Message::_get_data (Datum datum, LGMultiBase& value) const
{
	switch (datum)
	{
	case DATA1: value = message->data; break;
	case DATA2: value = message->data2; break;
	case DATA3: value = message->data3; break;
	default: value.clear ();
	}
}

void
Message::_set_data (Datum datum, const LGMultiBase& value)
{
	switch (datum)
	{
	case DATA1: message->data = value; break;
	case DATA2: message->data2 = value; break;
	case DATA3: message->data3 = value; break;
	default: break;
	}
}

void
Message::_get_reply (LGMultiBase& value) const
{
	value = *reply;
}

void
Message::_set_reply (const LGMultiBase& value)
{
	*reply = value;
}

const char*
Message::get_lg_typename () const
{
	return message->Persistent_GetName ();
}



// Message wrapping

MessageWrapError::MessageWrapError (const sScrMsg* message,
	const std::type_info& wrap_type, const char* problem) noexcept
{
	std::ostringstream explain;
	explain << "Can't wrap a \"" << (message ? message->message : "")
		<< "\" message of engine type "
		<< (message ? message->Persistent_GetName () : "null")
		<< " as a " << wrap_type.name () << ": " << problem << ".";
	explanation = explain.str ();
}



// GenericMessage

MESSAGE_WRAPPER_IMPL_ (GenericMessage, true) // allow any message type

GenericMessage::GenericMessage (const char* name)
	: Message (new sGenericScrMsg ())
{
	message->message = name;
}



// TimerMessage

MESSAGE_WRAPPER_IMPL (TimerMessage, sScrTimerMsg)

TimerMessage::TimerMessage (const char* timer_name)
	: Message (new sScrTimerMsg ())
{
	message->message = "Timer";
	MESSAGE_AS (sScrTimerMsg)->name = timer_name;
}

MESSAGE_ACCESSOR (String, TimerMessage, get_timer_name, sScrTimerMsg, name)



} // namespace Thief

