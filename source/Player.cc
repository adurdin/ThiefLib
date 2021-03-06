/******************************************************************************
 *  Player.cc
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



// Player

static bool
Player_arm_visible_getter (const FieldProxyConfig<bool>::Item& item,
	const LGMultiBase& multi)
{
	return multi.empty () ? item.default_value
		: (reinterpret_cast<const LGMulti<int>&> (multi) != -1);
}

static void
Player_arm_visible_setter (const FieldProxyConfig<bool>::Item&,
	LGMultiBase& multi, const bool& value)
{
	reinterpret_cast<LGMulti<int>&> (multi) = value ? 0 : -1;
}

PROXY_CONFIG (Player, visibility, "AI_Visibility", "Level", int, 0);
PROXY_CONFIG (Player, vis_light_rating, "AI_Visibility", "Light rating",
	int, 0);
PROXY_CONFIG (Player, vis_movement_rating, "AI_Visibility", "Movement rating",
	int, 0);
PROXY_CONFIG (Player, vis_exposure_rating, "AI_Visibility", "Exposure rating",
	int, 0);
PROXY_CONFIG (Player, vis_last_update, "AI_Visibility", "Last update time",
	Time, 0ul);
PROXY_CONFIG_ (Player, arm_visible, "INVISIBLE", nullptr, bool, true, 0,
	Player_arm_visible_getter, Player_arm_visible_setter);

Player::Player ()
	: Object ("Player"),
	  PROXY_INIT (visibility),
	  PROXY_INIT (vis_light_rating),
	  PROXY_INIT (vis_movement_rating),
	  PROXY_INIT (vis_exposure_rating),
	  PROXY_INIT (vis_last_update),
	  PROXY_INIT (arm_visible)
{}



// Player: inventory

bool
Player::is_in_inventory (const Object& object) const
{
	return SInterface<IContainSys> (LG)->Contains (number, object.number);
}

Container::Contents
Player::get_inventory () const
{
	return Container (*this).get_contents ();
}

void
Player::add_to_inventory (const Object& object)
{
	SInterface<IInventory> (LG)->Add (object.number);
}

void
Player::remove_from_inventory (const Object& object)
{
	SInterface<IInventory> (LG)->Remove (object.number);
}

Interactive
Player::get_selected_item () const
{
	return Object (SInterface<IInventory> (LG)->Selection (kInvItem));
}

bool
Player::is_wielding_junk () const
{
	return SInterface<IInventory> (LG)->WieldingJunk ();
}

void
Player::select_item (const Object& item)
{
	SInterface<IInventory> (LG)->Select (item.number);
}

void
Player::select_loot ()
{
	Engine::run_command ("loot_select");
}

void
Player::select_newest_item ()
{
	Engine::run_command ("select_newest_item");
}

void
Player::cycle_item_selection (Cycle direction)
{
	SInterface<IInventory> (LG)->CycleSelection (kInvItem,
		eCycleDirection (direction));
}

void
Player::clear_item ()
{
	SInterface<IInventory> (LG)->ClearSelection (kInvItem);
}

void
Player::start_tool_use ()
{
	Engine::run_command ("use_item 0");
}

void
Player::finish_tool_use ()
{
	Engine::run_command ("use_item 1");
}

void
Player::drop_item ()
{
	// There does not appear to be any script access to this.
	Engine::run_command ("drop_item");
}

bool
Player::has_touched (const Object& object) const
{
	// Consider the starting point as well.
	Object player = exists () ? Object (*this)
		: Link::get_one ("PlayerFactory").get_source ();
	if (!object.exists () || !player.exists ()) return false;

	// Is the player holding the object?
	if (Link::any_exist ("Contains", player, object))
		return true;

	// Has the player held but dropped the object (still culpable for it)?
	if (Link::any_exist ("CulpableFor", player, object))
		return true;

	// Is the object currently attached to the player arm or bow arm?
	AI attachment = Link::get_one ("~CreatureAttachment", object).get_dest ();
	switch (attachment.creature_type)
	{
	case AI::CreatureType::PLAYER_ARM:
	case AI::CreatureType::PLAYER_BOW_ARM:
		return true;
	default:
		break;
	}

	return false;
}



// Player: combat

Weapon
Player::get_selected_weapon () const
{
	return Object (SInterface<IInventory> (LG)->Selection (kInvWeapon));
}

bool
Player::is_bow_selected () const
{
	return SService<IBowSrv> (LG)->IsEquipped ();
}

void
Player::select_weapon (const Weapon& weapon)
{
	SInterface<IInventory> (LG)->Select (weapon.number);
}

void
Player::cycle_weapon_selection (Cycle direction)
{
	SInterface<IInventory> (LG)->CycleSelection (kInvWeapon,
		eCycleDirection (direction));
}

void
Player::clear_weapon ()
{
	SInterface<IInventory> (LG)->ClearSelection (kInvWeapon);
}

bool
Player::start_attack ()
{
	Weapon weapon = get_selected_weapon ();
	if (SService<IBowSrv> (LG)->IsEquipped ())
		return SService<IBowSrv> (LG)->StartAttack ();
	else
		return SService<IWeaponSrv> (LG)->StartAttack
			(number, weapon.number);
}

bool
Player::finish_attack ()
{
	Weapon weapon = get_selected_weapon ();
	if (SService<IBowSrv> (LG)->IsEquipped ())
		return SService<IBowSrv> (LG)->FinishAttack ();
	else
		return SService<IWeaponSrv> (LG)->FinishAttack
			(number, weapon.number);
}

bool
Player::abort_attack ()
{
	// "Weapon" (sword/blackjack) attacks cannot be aborted.
	return SService<IBowSrv> (LG)->IsEquipped () &&
		SService<IBowSrv> (LG)->AbortAttack ();
}



// Player: physics and movement

#ifdef IS_THIEF2

Physical
Player::get_climbing_object () const
{
	LGObject object;
	SService<IPhysSrv> (LG)->GetClimbingObject (number, object);
	return object;
}

void
Player::nudge_physics (int submodel, const Vector& _by)
{
	LGVector by (_by);
	SService<IPhysSrv> (LG)->PlayerMotionSetOffset (submodel, by);
}

#endif // IS_THIEF2

void
Player::unstick ()
{
	Engine::run_command ("unstick_player");
}

void
Player::add_speed_control (const String& name, float factor)
{
	SService<IDarkInvSrv> (LG)->AddSpeedControl
		(name.data (), factor, factor);
}

void
Player::remove_speed_control (const String& name)
{
	SService<IDarkInvSrv> (LG)->RemoveSpeedControl (name.data ());
}



// Player: limb model (player arm)

bool
Player::show_arm ()
{
	return SService<IPlayerLimbsSrv> (LG)->Equip
		(get_selected_item ().number);
}

bool
Player::start_arm_use ()
{
	return SService<IPlayerLimbsSrv> (LG)->StartUse
		(get_selected_item ().number);
}

bool
Player::finish_arm_use ()
{
	return SService<IPlayerLimbsSrv> (LG)->FinishUse
		(get_selected_item ().number);
}

bool
Player::hide_arm ()
{
	return SService<IPlayerLimbsSrv> (LG)->UnEquip
		(get_selected_item ().number);
}



// Player: miscellaneous

bool
Player::drop_dead ()
{
	return SService<IDarkGameSrv> (LG)->KillPlayer () == S_OK;
}

void
Player::enable_world_focus ()
{
	// Controlling the other three capabilities does not have any effect.
	SService<IDarkInvSrv> (LG)->CapabilityControl
		(kDrkInvCapWorldFocus, kDrkInvControlOn);
}

void
Player::disable_world_focus ()
{
	SService<IDarkInvSrv> (LG)->CapabilityControl
		(kDrkInvCapWorldFocus, kDrkInvControlOff);
}



// Camera

#ifndef IS_OSL

Object
Camera::get ()
{
	if (Engine::get_version () < Version (1, 22))
		throw std::runtime_error ("Camera::get"
			" is not implemented before engine version 1.22.");
	LGObject camera;
	SService<ICameraSrv> (LG)->GetCameraParent (camera);
	return camera;
}

bool
Camera::is_remote ()
{
	if (Engine::get_version () < Version (1, 22))
		throw std::runtime_error ("Camera::is_remote"
			" is not implemented before engine version 1.22.");

	LGBool remote;
	SService<ICameraSrv> (LG)->IsRemote (remote);
	return remote;
}

void
Camera::attach (const Object& to, bool freelook)
{
	if (freelook)
		SService<ICameraSrv> (LG)->DynamicAttach (to.number);
	else
		SService<ICameraSrv> (LG)->StaticAttach (to.number);
}

bool
Camera::detach (const Object& from)
{
	if (from == Object::ANY)
		return SService<ICameraSrv> (LG)->ForceCameraReturn () == S_OK;
	else
		return SService<ICameraSrv> (LG)->CameraReturn
			(from.number) == S_OK;
}

Vector
Camera::get_location ()
{
	if (Engine::get_version () < Version (1, 22))
		throw std::runtime_error ("Camera::get_location"
			" is not implemented before engine version 1.22.");
	LGVector location;
	SService<ICameraSrv> (LG)->GetPosition (location);
	return location;
}

Vector
Camera::get_rotation ()
{
	if (Engine::get_version () < Version (1, 22))
		throw std::runtime_error ("Camera::get_rotation"
			" is not implemented before engine version 1.22.");
	LGVector rotation;
	SService<ICameraSrv> (LG)->GetFacing (rotation);
	return rotation;
}

#endif // !IS_OSL



//TODO wrap property (on what?): Renderer\Camera Overlay = CameraOverlay



} // namespace Thief

