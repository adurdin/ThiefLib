/******************************************************************************
 *  Private.hh
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

#ifndef PRIVATE_HH
#define PRIVATE_HH

#include <Thief/Thief.hh>
#include <lg/lg.h>
#include <boost/preprocessor/repetition/repeat.hpp>

namespace Thief {

extern IScriptMan* LG;

typedef true_bool LGBool;



// Allocator

class Allocator
{
public:
	Allocator ();
	virtual ~Allocator ();

	void attach (IMalloc* allocator, const char* module_name);

	void* alloc (size_t size);
	void* realloc (void* ptr, size_t size);
	void free (void* ptr);

private:
	IMalloc* malloc;
#ifdef DEBUG
	IDebugMalloc* dbmalloc;
	char* module_name;
#endif
};

extern Allocator alloc;



// XYZColor: intermediate for CIE color space conversion

struct XYZColor
{
	double X, Y, Z;
	XYZColor (double _x, double _y, double _z) : X (_x), Y (_y), Z (_z) {}

	explicit XYZColor (const RGBColor& srgb);
	explicit operator RGBColor () const;

	explicit XYZColor (const LabColor& lab);
	explicit operator LabColor () const;

	static const XYZColor D65_WHITE;
};



// LGObject

class LGObject : public object
{
public:
	LGObject ()
	{}

	operator Object () const
	{
		return Object (id);
	}

	template <typename T, typename = THIEF_IS_OBJECT>
	operator T () const
	{
		return T (id);
	}
};



// LGString

class LGString : public cScrStr
{
public:
	LGString ()
		: cScrStr (0u), owned (true)
	{}

	virtual ~LGString ()
	{
		if (owned) Free ();
	}

	operator String () const
	{
		return (const char*) (*this);
	}

	bool owned;
};



// LGVector

class LGVector : public mxs_vector
{
public:
	LGVector ()
	{
		x = y = z = 0.0f;
	}

	LGVector (const mxs_vector* copy)
	{
		x = copy ? copy->x : 0.0f;
		y = copy ? copy->y : 0.0f;
		z = copy ? copy->z : 0.0f;
	}

	LGVector (const Vector& copy)
	{
		x = copy.x;
		y = copy.y;
		z = copy.z;
	}

	operator Vector () const
	{
		return Vector (x, y, z);
	}
};



// LinkMessageImpl

struct LinkMessageImpl : public sScrMsg
{
	typedef LinkMessage::Event Event;

	Event event;
	Flavor flavor;
	Link::Number link;
	Object source, dest;

	LinkMessageImpl ();
	virtual ~LinkMessageImpl ();
	virtual const char* __thiscall GetName () const;
};



// PropertyMessageImpl

struct PropertyMessageImpl : public sScrMsg
{
	typedef PropertyMessage::Event Event;

	Event event;
	bool inherited;
	Property property;
	Object object;

	PropertyMessageImpl ();
	virtual ~PropertyMessageImpl ();
	virtual const char* __thiscall GetName () const;
};



// ConversationMessageImpl

struct ConversationMessageImpl : public sScrMsg
{
	Object conversation;

	ConversationMessageImpl ();
	virtual ~ConversationMessageImpl ();
	virtual const char* __thiscall GetName () const;
};



// Field proxy convenience macros

#define PROXY_CONFIG_(Class, Member, Major, Minor, Type, Default, Detail, Getter, Setter) \
const FieldProxyConfig<Type>::Item \
Class::I_##Member { Major, Minor, int (Detail), Default }; \
const FieldProxyConfig<Type> \
Class::F_##Member { &I_##Member, 1u, Getter, Setter }

#define PROXY_CONFIG(Class, Member, Major, Minor, Type, Default) \
PROXY_CONFIG_ (Class, Member, Major, Minor, Type, Default, 0, \
	FieldProxyConfig<Type>::default_getter<Type>, \
	FieldProxyConfig<Type>::default_setter<Type>)

#define PROXY_CONV_CONFIG(Class, Member, Major, Minor, Type, Default, StorageType) \
PROXY_CONFIG_ (Class, Member, Major, Minor, Type, Default, 0, \
	FieldProxyConfig<Type>::default_getter<StorageType>, \
	FieldProxyConfig<Type>::default_setter<StorageType>)

#define PROXY_BIT_CONFIG(Class, Member, Major, Minor, Mask, Default) \
PROXY_CONFIG_ (Class, Member, Major, Minor, bool, Default, Mask, \
	FieldProxyConfig<bool>::bitmask_getter, \
	FieldProxyConfig<bool>::bitmask_setter)

#define PROXY_COMP_CONFIG(Class, Member, Major, Minor, _Component, Default) \
PROXY_CONFIG_ (Class, Member, Major, Minor, float, Default, Vector::_Component, \
	FieldProxyConfig<float>::component_getter, \
	FieldProxyConfig<float>::component_setter)

#define PROXY_NEG_CONFIG(Class, Member, Major, Minor, TypeIsBool, Default) \
PROXY_CONFIG_ (Class, Member, Major, Minor, bool, Default, -1, \
	FieldProxyConfig<bool>::bitmask_getter, \
	FieldProxyConfig<bool>::bitmask_setter)

#define PROXY_NEG_BIT_CONFIG(Class, Member, Major, Minor, Mask, Default) \
PROXY_BIT_CONFIG (Class, Member, Major, Minor, -Mask, Default)

#define PROXY_ARRAY_CONFIG_(Class, Member, Count, Type, Getter, Setter, ...) \
const FieldProxyConfig<Type>::Item \
Class::I_##Member [] { __VA_ARGS__ }; \
const FieldProxyConfig<Type> \
Class::F_##Member { I_##Member, Count, Getter, Setter }

#define PROXY_ARRAY_CONFIG(Class, Member, Count, Type, ...) \
PROXY_ARRAY_CONFIG_ (Class, Member, Count, Type, \
	FieldProxyConfig<Type>::default_getter<Type>, \
	FieldProxyConfig<Type>::default_setter<Type>, \
	__VA_ARGS__)

#define PROXY_CONV_ARRAY_CONFIG(Class, Member, Count, Type, StorageType, ...) \
PROXY_ARRAY_CONFIG_ (Class, Member, Count, Type, \
	FieldProxyConfig<Type>::default_getter<StorageType>, \
	FieldProxyConfig<Type>::default_setter<StorageType>, \
	__VA_ARGS__)

#define PROXY_BIT_ARRAY_CONFIG(Class, Member, Count, TypeIsBool, ...) \
PROXY_ARRAY_CONFIG_ (Class, Member, Count, bool, \
	FieldProxyConfig<bool>::bitmask_getter, \
	FieldProxyConfig<bool>::bitmask_setter, \
	__VA_ARGS__)

#define PROXY_ARRAY_ITEM(Major, Minor, Default) \
{ Major, Minor, 0, Default }

#define PROXY_BIT_ARRAY_ITEM(Major, Minor, Default, Mask) \
{ Major, Minor, int (Mask), Default }

#define PROXY_INIT(Member) Member (*this, 0u)

#define PROXY_ARRAY_INIT_ONE(z, n, text) { *this, n##u },
#define PROXY_ARRAY_INIT(Member, Count) \
Member { BOOST_PP_REPEAT (Count, PROXY_ARRAY_INIT_ONE, junk) }



// Object subclass convenience macros

#define OBJECT_TYPE_IMPL_(ClassName, ...) \
ClassName::ClassName () : Object (), __VA_ARGS__ {} \
ClassName::ClassName (const Object& object) : Object (object), __VA_ARGS__ {} \
ClassName::ClassName (const ClassName& copy) : Object (copy), __VA_ARGS__ {} \
ClassName& ClassName::operator = (const ClassName& copy) \
	{ number = copy.number; return *this; } \
ClassName::ClassName (Number _number) : Object (_number), __VA_ARGS__ {} \
ClassName::ClassName (const String& name) : Object (name), __VA_ARGS__ {}

#define OBJECT_TYPE_IMPL(ClassName) \
ClassName::ClassName () : Object () {} \
ClassName::ClassName (const Object& object) : Object (object) {} \
ClassName::ClassName (const ClassName& copy) : Object (copy) {} \
ClassName& ClassName::operator = (const ClassName& copy) \
	{ number = copy.number; return *this; } \
ClassName::ClassName (Number _number) : Object (_number) {} \
ClassName::ClassName (const String& name) : Object (name) {}



// Link subclass convenience macros

#define LINK_FLAVOR_IMPL(FlavorName, ...) \
Flavor \
FlavorName##Link::flavor (bool reverse) \
{ \
	static Flavor fwd (#FlavorName), rev ("~" #FlavorName); \
	return reverse ? rev : fwd; \
} \
\
FlavorName##Link& \
FlavorName##Link::operator = (const FlavorName##Link& copy) \
	{ number = copy.number; return *this; } \
\
void \
FlavorName##Link::check_valid () const \
{ \
	if (*this != Link::NONE && \
	    get_flavor () != flavor () && get_flavor () != flavor (true)) \
	{ \
		boost::format error ("Link %|| is of flavor %|| instead of " \
			"expected " #FlavorName " or ~" #FlavorName "."); \
		error % number % get_flavor (); \
		throw std::runtime_error (error.str ()); \
	} \
} \
\
FlavorName##Link::List \
FlavorName##Link::get_all (const Object& source, const Object& dest, \
	Inheritance inheritance, bool reverse) \
{ \
	auto _links = \
		Link::get_all (flavor (reverse), source, dest, inheritance); \
	List links; \
	for (auto& link : _links) \
		links.push_back (link); \
	return links; \
} \
\
FlavorName##Link::FlavorName##Link () \
	: Link (), __VA_ARGS__ {} \
\
FlavorName##Link::FlavorName##Link (const Link& link) \
	: Link (link), __VA_ARGS__ { check_valid (); } \
\
FlavorName##Link::FlavorName##Link (const FlavorName##Link& link) \
	: Link (link), __VA_ARGS__ {} \
\
FlavorName##Link::FlavorName##Link (Number _number) \
	: Link (_number), __VA_ARGS__ { check_valid (); }



// Message subclass convenience macros

#define MESSAGE_WRAPPER_IMPL_(Type, Tests) \
Type::Type (sScrMsg* _message, sMultiParm* _reply) \
	: Message (_message, _reply, (Tests), #Type)

#define MESSAGE_NAME_TEST(Name) \
(_message && _message->message && _stricmp (_message->message, Name) == 0)

#define MESSAGE_WRAPPER_IMPL(Type, Name) \
MESSAGE_WRAPPER_IMPL_ (Type, MESSAGE_NAME_TEST (Name))

#define MESSAGE_AS(LGType) static_cast<LGType*> (message)



} // namespace Thief

#endif // PRIVATE_HH

