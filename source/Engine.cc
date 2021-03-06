/******************************************************************************
 *  Engine.cc
 *
 *  This file is part of ThiefLib, a library for Thief 1/2 script modules.
 *  Copyright (C) 2013-2014 Kevin Daughtridge <kevin@kdau.com>
 *  Adapted in part from Public Scripts and the Object Script Library
 *  Copyright (C) 2005-2013 Tom N Harris <telliamed@whoopdedo.org>
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



// Monolog::Streambuf

class Monolog::Streambuf : public std::streambuf
{
public:
	Streambuf (MPrintfProc proc);

	void write (const char* string);

protected:
	virtual int_type overflow (int_type ch = traits_type::eof ());
	virtual int sync ();

private:
	bool flush_to_mono ();

	static const size_t BUFFER_SIZE = 1000u; // imposed by the engine
	std::array<char_type, BUFFER_SIZE> buffer;
	MPrintfProc proc;
};

Monolog::Streambuf::Streambuf (MPrintfProc _proc)
	: proc (_proc)
{
	// Save one character for overflows.
	setp (&buffer.front (), &buffer.back ());
}

Monolog::Streambuf::int_type
Monolog::Streambuf::overflow (int_type ch)
{
	if (ch != traits_type::eof ())
	{
		assert (pptr () <= epptr ());
		*pptr () = ch;
		pbump (1);
		if (flush_to_mono ())
			return ch;
	}
	return traits_type::eof ();
}

int
Monolog::Streambuf::sync ()
{
	return flush_to_mono () ? 0 : -1;
}

bool
Monolog::Streambuf::flush_to_mono ()
{
	char_type *begin = pbase (), *current = pptr (), *end = epptr (),
		*front = begin;

	// Write any full lines in the buffer.
	for (char_type* p = begin; p != current; ++p)
		if (*p == '\n')
		{
			*p = '\0'; // show mprintf where to stop
			write (front);
			front = p + 1;
		}

	// If there weren't any, force out the whole thing.
	if (front == begin)
	{
		*current = '\0';
		write (begin);
		front = current;
	}

	// If partial lines were left in the buffer, move them back.
	if (front < current)
		std::copy (front, current, begin);

	// Send the current pointer backwards.
	setp (begin, end);
	pbump (current - front);

	// Success if there was something for write() to do.
	return proc || LG;
}

inline void
Monolog::Streambuf::write (const char* string)
{
	if (proc)
		proc ("%s\n", string);
	else if (LG)
		try
		{
			SService<IDebugScrSrv> (LG)->MPrint
				(string,"","","","","","","");
		}
		catch (...) {}
}



// Monolog

Monolog
mono;

Monolog
null_mono;

Monolog::Monolog ()
	: std::ostream (), buf ()
{}

Monolog::~Monolog ()
{}

void
Monolog::attach (MPrintfProc proc)
{
	buf.reset (proc ? new Streambuf (proc) : nullptr);
	rdbuf (buf.get ());
}

void
Monolog::log (const String& message)
{
	if (buf) buf->write (message.data ());
}

void
Monolog::log (const boost::format& message)
{
	if (buf) buf->write (message.str ().data ());
}



// CanvasSize

CanvasSize::CanvasSize ()
	: w (0), h (0)
{}

CanvasSize::CanvasSize (int _w, int _h)
	: w (_w), h (_h)
{}

bool
CanvasSize::valid () const
{
	// An empty size still counts as valid.
	return w >= 0 && h >= 0;
}

bool
CanvasSize::operator == (const CanvasSize& rhs) const
{
	return w == rhs.w && h == rhs.h;
}

bool
CanvasSize::operator != (const CanvasSize& rhs) const
{
	return w != rhs.w || h != rhs.h;
}



// Version

Version::Version (int _major, int _minor)
	: major (_major), minor (_minor)
{}

bool
Version::operator == (const Version& rhs) const
{
	return major == rhs.major && minor == rhs.minor;
}

bool
Version::operator < (const Version& rhs) const
{
	return major < rhs.major || (major == rhs.major && minor < rhs.minor);
}

bool
Version::operator >= (const Version& rhs) const
{
	return major > rhs.major || (major == rhs.major && minor >= rhs.minor);
}

std::ostream&
operator << (std::ostream& out, const Version& version)
{
        out << version.major << "." << version.minor;
        return out;
}



// Engine

String
Engine::get_app_name ()
{
	LGString name;
	SService<IVersionSrv> (LG)->GetAppName (true, name);
	return name;
}

String
Engine::get_long_app_name ()
{
	LGString name;
	SService<IVersionSrv> (LG)->GetAppName (false, name);
	return name;
}

Version
Engine::get_version ()
{
	Version version (0, 0);
	SService<IVersionSrv> (LG)->GetVersion (version.major, version.minor);
	return version;
}



// Engine: game mode and sim status

Engine::Mode
Engine::get_mode ()
{
	switch (SService<IVersionSrv> (LG)->IsEditor ())
	{
	case 1: return Mode::EDIT;
	default: return Mode::GAME;
	}
}

bool
Engine::is_editor ()
{
	return SService<IVersionSrv> (LG)->IsEditor () != 0;
}

bool
Engine::is_sim ()
{
	return SInterface<ISimManager> (LG)->LastMsg ()
		& (kSimStart | kSimResume);
}



// Engine: graphics

CanvasSize
Engine::get_canvas_size ()
{
	CanvasSize size;
	SService<IEngineSrv> (LG)->GetCanvasSize (size.w, size.h);
	return size;
}

float
Engine::get_aspect_ratio ()
{
	return SService<IEngineSrv> (LG)->GetAspectRatio ();
}

int
Engine::get_directx_version ()
{
	return SService<IEngineSrv> (LG)->IsRunningDX6 () ? 6 : 9;
}

bool
Engine::rendered_this_frame (const Object& object)
{
	LGBool rendered;
	SService<IObjectSrv> (LG)->RenderedThisFrame (rendered, object.number);
	return rendered;
}

Engine::RaycastHit
Engine::raycast (RaycastMode mode, const Vector& from, const Vector& to,
	bool include_mesh)
{
	int type = RaycastHit::NONE;
	LGVector location;
	LGObject object;
	if (mode == RaycastMode::TERRAIN)
		type = SService<IEngineSrv> (LG)->PortalRaycast
			(LGVector (from), LGVector (to), location)
				? RaycastHit::TERRAIN : RaycastHit::NONE;
	else
		type = SService<IEngineSrv> (LG)->ObjRaycast (LGVector (from),
			LGVector (to), location, object, eObjRaycast (mode),
			!include_mesh, Object::NONE.number, Object::NONE.number);
	return { RaycastHit::Type (type), location, object };
}

Engine::RaycastHit
Engine::raycast (RaycastMode mode, const Object& from, const Object& to,
	bool include_mesh)
{
	int type = RaycastHit::NONE;
	LGVector location;
	LGObject object;
	if (mode == RaycastMode::TERRAIN)
		type = SService<IEngineSrv> (LG)->PortalRaycast
			(LGVector (from.get_location ()),
			LGVector (to.get_location ()), location)
				? RaycastHit::TERRAIN : RaycastHit::NONE;
	else
		type = SService<IEngineSrv> (LG)->ObjRaycast
			(LGVector (from.get_location ()),
			LGVector (to.get_location ()),
			location, object, eObjRaycast (mode), !include_mesh,
			from.number, to.number);
	return { RaycastHit::Type (type), location, object };
}



// Engine: configuration

bool
Engine::has_config (const String& variable)
{
	return SService<IEngineSrv> (LG)->ConfigIsDefined (variable.data ());
}

template<>
int
Engine::get_config (const String& variable)
{
	int value = 0;
	if (!SService<IEngineSrv> (LG)->ConfigGetInt (variable.data (), value))
		throw std::runtime_error ("could not get config variable");
	return value;
}

template<>
float
Engine::get_config (const String& variable)
{
	float value = 0.0f;
	if (!SService<IEngineSrv> (LG)->ConfigGetFloat (variable.data (), value))
		throw std::runtime_error ("could not get config variable");
	return value;
}

template<>
String
Engine::get_config (const String& variable)
{
	LGString value;
	if (!SService<IEngineSrv> (LG)->ConfigGetRaw (variable.data (), value))
		throw std::runtime_error ("could not get config variable");
	return value;
}

float
Engine::get_binding_config (const String& variable)
{
	return SService<IEngineSrv> (LG)->BindingGetFloat (variable.data ());
}

bool
Engine::is_command_bound (const String& command)
{
	LGBool bound;
	SService<IDarkUISrv> (LG)->IsCommandBound (bound, command.data ());
	return bound;
}

String
Engine::get_command_binding (const String& command)
{
	if (!is_command_bound (command))
		return String ();
	LGString binding;
	SService<IDarkUISrv> (LG)->DescribeKeyBinding (binding, command.data ());
	return binding;
}




// Engine: miscellaneous

String
Engine::find_file_in_path (const String& type, const String& file)
{
	LGString _path;
	bool found = SService<IEngineSrv> (LG)->FindFileInPath
		(type.data (), file.data (), _path);
	return found ? (const char*) _path : "";
}

int
Engine::random_int (int minimum, int maximum)
{
	return SService<IDataSrv> (LG)->RandInt (minimum, maximum);
}

float
Engine::random_float ()
{
	return SService<IDataSrv> (LG)->RandFlt0to1 ();
}

float
Engine::random_float (float minimum, float maximum)
{
	return minimum + (maximum - minimum) *
		SService<IDataSrv> (LG)->RandFlt0to1 ();
}

void
Engine::run_command (const String& command, const String& arguments)
{
	SService<IDebugScrSrv> (LG)->Command
		(command.data (), arguments.data (), "", "", "", "", "", "");
}

void
Engine::write_to_game_log (const String& message)
{
	SService<IDebugScrSrv> (LG)->Log
		(message.data (), "", "", "", "", "", "", "");
}



// GameModeMessage

MESSAGE_WRAPPER_IMPL (GameModeMessage, "DarkGameModeChange"),
	event (MESSAGE_AS (sDarkGameModeScrMsg)->fResuming ? RESUME : SUSPEND)
{}

GameModeMessage::GameModeMessage (Event _event)
	: Message (new sDarkGameModeScrMsg ()), event (_event)
{
	message->message = "DarkGameModeChange";
	MESSAGE_AS (sDarkGameModeScrMsg)->fResuming = (event == RESUME);
	MESSAGE_AS (sDarkGameModeScrMsg)->fSuspending = (event == SUSPEND);
}



// SimMessage

MESSAGE_WRAPPER_IMPL (SimMessage, "Sim"),
	event (MESSAGE_AS (sSimMsg)->fStarting ? START : FINISH)
{}

SimMessage::SimMessage (Event _event)
	: Message (new sSimMsg ()), event (_event)
{
	message->message = "Sim";
	MESSAGE_AS (sSimMsg)->fStarting = (event == START);
}



} // namespace Thief

