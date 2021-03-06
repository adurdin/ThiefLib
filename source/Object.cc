/******************************************************************************
 *  Object.cc
 *
 *  This file is part of ThiefLib, a library for Thief 1/2 script modules.
 *  Copyright (C) 2013 Kevin Daughtridge <kevin@kdau.com>
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



// Locating and wrapping objects

bool
Object::exists () const
{
	LGBool _exists;
	SService<IObjectSrv> (LG)->Exists (_exists, number);
	return _exists;
}

#ifdef IS_THIEF2
Object
Object::find_closest (const Object& archetype, const Object& nearby)
{
	LGObject closest;
	SService<IObjectSrv> (LG)->FindClosestObjectNamed (closest,
		nearby.number, archetype.get_name ().data ());
	return closest;
}
#endif // IS_THIEF2



// Creating and destroying objects

Object
Object::create (const Object& archetype)
{
	LGObject created;
	SService<IObjectSrv> (LG)->Create (created, archetype.number);
	return created;
}

Object
Object::start_create (const Object& archetype)
{
	LGObject created;
	SService<IObjectSrv> (LG)->BeginCreate (created, archetype.number);
	return created;
}

void
Object::finish_create ()
{
	if (SService<IObjectSrv> (LG)->EndCreate (number) != S_OK)
		throw std::runtime_error ("could not finish creating object");
}

Object
Object::create_temp_fnord (Time lifespan)
{
	Object fnord = create (Object ("Marker"));
	fnord.set_transient (true);
	if (lifespan != 0ul)
		fnord.schedule_destruction (lifespan);
	return fnord;
}

Object
Object::create_archetype (const Object& parent, const String& name)
{
	return Object (SInterface<ITraitManager> (LG)->CreateArchetype
		(name.data (), parent.number));
}

Object
Object::create_metaprop (const Object& parent, const String& name)
{
	return Object (SInterface<ITraitManager> (LG)->CreateMetaProperty
		(name.data (), parent.number));
}

Object
Object::clone () const
{
	return create (*this);
}

void
Object::destroy ()
{
	SService<IObjectSrv> (LG)->Destroy (number);
}

void
Object::schedule_destruction (Time lifespan)
{
	if (lifespan == 0ul)
		destroy ();
	else
	{
		DeleteTweq tweq (*this);
		tweq.simulate_always = true;
		tweq.duration = lifespan;
		tweq.halt_action = Tweq::Halt::DESTROY_OBJECT;
		tweq.active = true;
	}
}



// Object numbers and names

const Object
Object::NONE { 0 };

const Object
Object::ANY { 0 };

const Object
Object::SELF { INT_MAX };

String
Object::get_name () const
{
	LGString name;
	SService<IObjectSrv> (LG)->GetName (name, number);
	return name;
}

void
Object::set_name (const String& name)
{
	SService<IObjectSrv> (LG)->SetName (number, name.data ());
}

String
Object::get_editor_name () const
{
	static const boost::format NAMED ("%|| (%||)"), UNNAMED ("A %|| (%||)");
	boost::format editor_name;

	String name = (*this == NONE) ? "None"
		: exists () ? get_name () : "NONEXISTENT";

	if (!name.empty ())
	{
		editor_name = NAMED;
		editor_name % name % number;
	}
	else
	{
		editor_name = UNNAMED;
		editor_name % get_archetype ().get_name () % number;
	}

	return editor_name.str ();
}

//TODO wrap property: Inventory\Object Name = GameName
String
Object::get_display_name () const
{
	LGString name;
	SService<IDataSrv> (LG)->GetObjString (name, number, "objnames");
	return name;
}

//TODO wrap property: Inventory\Long Description = GameDesc
String
Object::get_description () const
{
	LGString desc;
	SService<IDataSrv> (LG)->GetObjString (desc, number, "objdescs");
	return desc;
}



// Inheritance and transience

Object::Type
Object::get_type () const
{
	if (!exists ())
		return Type::NONE;
	else if (SInterface<ITraitManager> (LG)->IsArchetype (number))
		return Type::ARCHETYPE;
	else if (SInterface<ITraitManager> (LG)->IsMetaProperty (number))
		return Type::METAPROPERTY;
	else
		return Type::CONCRETE;
}

bool
Object::inherits_from (const Object& ancestor) const
{
	LGBool inherits;
	SService<IObjectSrv> (LG)->InheritsFrom
		(inherits, number, ancestor.number);
	return inherits;
}

Object::List
Object::get_ancestors () const
{
	SInterface<IObjectQuery> _ancestors =
		SInterface<ITraitManager> (LG)->Query
			(number, kTraitQueryMetaProps | kTraitQueryFull);

	List ancestors;

	if (_ancestors)
		for (_ancestors->Next (); // Skip this object itself.
		     !_ancestors->Done (); _ancestors->Next ())
			ancestors.emplace_back (_ancestors->Object ());

	return ancestors;
}

Object::List
Object::get_descendants (bool include_indirect) const
{
	unsigned flags = kTraitQueryChildren;
	if (include_indirect) flags |= kTraitQueryFull;
	SInterface<IObjectQuery> _descendants =
		SInterface<ITraitManager> (LG)->Query (number, flags);

	List descendants;

	if (_descendants)
		for (; !_descendants->Done (); _descendants->Next ())
			descendants.emplace_back (_descendants->Object ());

	return descendants;
}

Object
Object::get_archetype () const
{
	return Object (SInterface<ITraitManager> (LG)->GetArchetype (number));
}

void
Object::set_archetype (const Object& archetype)
{
	SInterface<ITraitManager> (LG)->SetArchetype (number, archetype.number);
}

bool
Object::has_metaprop (const Object& metaprop) const
{
	LGBool has;
	SService<IObjectSrv> (LG)->HasMetaProperty
		(has, number, metaprop.number);
	return has;
}

bool
Object::add_metaprop (const Object& metaprop, bool single)
{
	if (single && has_metaprop (metaprop)) return false;
	SService<IObjectSrv> (LG)->AddMetaProperty (number, metaprop.number);
	return true;
}

bool
Object::remove_metaprop (const Object& metaprop)
{
	if (!has_metaprop (metaprop)) return false;
	SService<IObjectSrv> (LG)->RemoveMetaProperty (number, metaprop.number);
	return true;
}

bool
Object::is_transient () const
{
	LGBool transient;
	SService<IObjectSrv> (LG)->IsTransient (transient, number);
	return transient;
}

void
Object::set_transient (bool transient)
{
	SService<IObjectSrv> (LG)->SetTransience (number, transient);
}



// World position

Vector
Object::get_location () const
{
	LGVector location;
	SService<IObjectSrv> (LG)->Position (location, number);
	return location;
}

void
Object::set_location (const Vector& location)
{
	set_position (location, get_rotation ());
}

Vector
Object::get_rotation () const
{
	LGVector rotation;
	SService<IObjectSrv> (LG)->Facing (rotation, number);
	return rotation;
}

void
Object::set_rotation (const Vector& rotation)
{
	set_position (get_location (), rotation);
}

void
Object::set_position (const Vector& location, const Vector& rotation,
	const Object& relative)
{
	SService<IObjectSrv> (LG)->Teleport (number,
		LGVector (location), LGVector (rotation),
		(relative == SELF) ? number : relative.number);
}

Vector
Object::object_to_world (const Vector& relative) const
{
	LGVector absolute;
	SService<IObjectSrv> (LG)->ObjectToWorld
		(absolute, number, LGVector (relative));
	return absolute;
}



// Miscellaneous

Object
Object::get_container () const
{
	return Object (SInterface<IContainSys> (LG)->GetContainer (number));
}

bool
Object::has_refs () const
{
	return ObjectProperty ("HasRefs", *this).get (true);
}

Object::~Object ()
{}

Object::Number
Object::find (const String& name)
{
	LGObject named;
	SService<IObjectSrv> (LG)->Named (named, name.data ());
	if (named) return named;

	Object numbered;
	try
	{
		numbered.number = std::stoi (name, nullptr, 10);
	}
	catch (...) {}

	return numbered.exists () ? numbered.number : NONE.number;
}

std::ostream&
operator << (std::ostream& out, const Object& object)
{
	out << object.get_editor_name ();
	return out;
}



} // namespace Thief

