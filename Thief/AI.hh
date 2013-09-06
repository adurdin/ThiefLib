/******************************************************************************
 *  AI.hh
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

#ifndef THIEF_AI_HH
#define THIEF_AI_HH

#include <Thief/Base.hh>
#include <Thief/Link.hh>
#include <Thief/Message.hh>
#include <Thief/Physics.hh>
#include <Thief/Sound.hh>
#include <Thief/Types.hh>

namespace Thief {

class AI;
typedef std::vector<AI> AIs;



class AI : public virtual Interactive, public Being, public Container
{
public:
	THIEF_OBJECT_TYPE (AI)
	bool is_ai () const;

	static size_t count_ais ();
	static AIs get_all_ais ();

	// State

	enum class Mode
	{
		ASLEEP,
		SUPER_EFFICIENT,
		EFFICIENT,
		NORMAL,
		COMBAT,
		DEAD
	};

	THIEF_PROP_FIELD (Mode, mode); // Changing this may be dangerous. //TESTME
	bool is_dead () const; //TESTME
	bool is_dying () const; //TESTME

	void set_mode_dead ();
	void set_mode_normal ();

	THIEF_PROP_FIELD_CONST (Time, frozen_start);
	THIEF_PROP_FIELD_CONST (Time, frozen_duration);
	void freeze (Time duration);
	void unfreeze ();

	// Actions

	enum class Priority
		{ DEFAULT, VERY_LOW, LOW, NORMAL, HIGH, VERY_HIGH, ABSOLUTE };

	enum class Speed { SLOW, NORMAL, FAST };
	enum class ActionPriority { LOW, NORMAL, HIGH };

	bool go_to_location (const Object& nearby, Speed = Speed::NORMAL,
		ActionPriority = ActionPriority::NORMAL); //TESTME

	template <typename T>
	bool go_to_location (const Object& nearby, Speed, ActionPriority,
		const T& result_data); //TESTME

	bool face_object (const Object& target); //TESTME

	bool frob_object (const Object& target,
		const Object& tool = Object::NONE,
		ActionPriority = ActionPriority::NORMAL); //TESTME

	template <typename T>
	bool frob_object (const Object& target, const Object& tool,
		ActionPriority, const T& result_data); //TESTME

	// Abilities and attributes

	THIEF_PROP_FIELD (String, behavior_set); //TESTME

	enum class Rating
		{ NONE, WELL_BELOW, BELOW, AVERAGE, ABOVE, WELL_ABOVE };
	THIEF_PROP_FIELD (Rating, aggression); //TESTME
	THIEF_PROP_FIELD (Rating, aptitude); //TESTME
	THIEF_PROP_FIELD (Rating, defensiveness); //TESTME
	THIEF_PROP_FIELD (Rating, dodginess); //TESTME
	THIEF_PROP_FIELD (Rating, hearing); //TESTME
	THIEF_PROP_FIELD (Rating, sloth); //TESTME
	THIEF_PROP_FIELD (Rating, verbosity); //TESTME
	THIEF_PROP_FIELD (Rating, vision); //TESTME

	THIEF_PROP_FIELD (float, time_warp); //TESTME

	THIEF_PROP_FIELD (bool, uses_doors); //TESTME
	THIEF_PROP_FIELD (bool, needs_big_doors); //TESTME

	THIEF_PROP_FIELD (bool, flash_vulnerable); //TESTME

	THIEF_PROP_FIELD (bool, head_tracks_player); //TESTME

	THIEF_PROP_FIELD (bool, investigates); //TESTME

	THIEF_PROP_FIELD (bool, patrols);
	THIEF_PROP_FIELD (bool, patrols_randomly); //TESTME

	THIEF_PROP_FIELD (bool, traverses_water); //TESTME

	// Alertness and awareness

	THIEF_PROP_FIELD (bool, notices_bodies); //TESTME
	THIEF_PROP_FIELD (bool, notices_cameras); //TESTME
	THIEF_PROP_FIELD (bool, notices_damage); //TESTME
	THIEF_PROP_FIELD (bool, notices_other_ai); //TESTME
	THIEF_PROP_FIELD (bool, notices_projectiles); //TESTME

	enum class Alert { NONE, LOW, MODERATE, HIGH };

	Alert get_current_alert () const; //TESTME
	void set_minimum_alert (Alert); //TESTME
	void clear_alertness (); //TESTME

	// Model

	enum class CreatureType
	{
		NONE = -1, HUMANOID, PLAYER_ARM, PLAYER_BOW_ARM, BURRICK,
		SPIDER, BUGBEAST, CRAYMAN, CONSTANTINE, APPARITION, SWEEL, ROPE,
		ZOMBIE, SMALL_SPIDER, FROG, CUTTY,
#ifdef IS_THIEF2
		AVATAR, ROBOT, SMALL_ROBOT, SPIDER_BOT
#endif
	};
	THIEF_PROP_FIELD (CreatureType, creature_type); //TESTME
	THIEF_PROP_FIELD (float, creature_scale); //TESTME
	THIEF_PROP_FIELD (bool, is_small_creature); //TESTME

	enum class Joint
	{
		NONE, HEAD, NECK, ABDOMEN, BUTT,
		LEFT_SHOULDER, RIGHT_SHOULDER, LEFT_ELBOW, RIGHT_ELBOW,
		LEFT_WRIST, RIGHT_WRIST, LEFT_FINGERS, RIGHT_FINGERS,
		LEFT_HIP, RIGHT_HIP, LEFT_KNEE, RIGHT_KNEE,
		LEFT_ANKLE, RIGHT_ANKLE, LEFT_TOE, RIGHT_TOE, TAIL
	};

	// Motions

	THIEF_PROP_FIELD (String, motion_tags); //TESTME
	THIEF_PROP_FIELD (String, motion_tags_standing); //TESTME

	bool play_motion (const String& motion); //TESTME

	// Speech

	THIEF_PROP_FIELD_CONST (String, voice);

	THIEF_PROP_FIELD_CONST (bool, is_speaking);
	THIEF_PROP_FIELD_CONST (Time, last_speech_time);
	THIEF_PROP_FIELD_CONST (SoundSchema, last_speech_schema);
	THIEF_PROP_FIELD_CONST (int, last_speech_concept);

	void set_speech_enabled (bool enabled); //TESTME
	void halt_speech (); //TESTME

	// Miscellaneous

	THIEF_PROP_FIELD (bool, is_innocent);
	THIEF_PROP_FIELD (bool, is_robot); //TESTME

	void send_signal (const String& signal); //TESTME

private:
	bool _go_to_location (const Object& nearby, Speed, ActionPriority,
		const LGMultiBase& result_data);

	bool _frob_object (const Object& target, const Object& tool,
		ActionPriority, const LGMultiBase& result_data);
};



class Conversation : public virtual Object
{
public:
	THIEF_OBJECT_TYPE (Conversation)
	bool is_conversation () const;

	bool start_conversation ();

	AI get_actor (size_t number) const;
	void set_actor (size_t number, const Object& actor);
	void remove_actor (size_t number);

	THIEF_PROP_FIELD (AI::Alert, abort_level); //TESTME
	THIEF_PROP_FIELD (AI::Priority, abort_priority); //TESTME
	THIEF_PROP_FIELD (bool, save_conversation);

	bool subscribe (const Object& host = Object::SELF);
	bool unsubscribe (const Object& host = Object::SELF); //TESTME
};



THIEF_FLAVORED_LINK (AIAwareness) //TESTME
{
	THIEF_FLAVORED_LINK_COMMON (AIAwareness)

	enum Flags
	{
		SEEN = 1u, HEARD = 2u,
		CAN_RAYCAST = 4u, HAVE_LOS = 8u,
		BLIND = 16u, DEAF = 32u,
		HIGHEST = 64u, FIRSTHAND = 128u
	};

	enum class Level { NONE, LOW, MODERATE, HIGH };

	static AIAwarenessLink create (const Object& source, const Object& dest,
		unsigned flags, Level, Time, const Vector&, int vision_cone);

	THIEF_LINK_FIELD (bool, seen);
	THIEF_LINK_FIELD (bool, heard);
	THIEF_LINK_FIELD (bool, can_raycast);
	THIEF_LINK_FIELD (bool, have_los);
	THIEF_LINK_FIELD (bool, blind);
	THIEF_LINK_FIELD (bool, deaf);
	THIEF_LINK_FIELD (bool, highest);
	THIEF_LINK_FIELD (bool, firsthand);

	THIEF_LINK_FIELD_CONST (Level, current_level);
	THIEF_LINK_FIELD_CONST (Level, peak_level);
	THIEF_LINK_FIELD_CONST (Level, pulse_level);
	THIEF_LINK_FIELD_CONST (Time, level_time);
	void update_level (Level, Time);

	THIEF_LINK_FIELD_CONST (Vector, contact_location);
	THIEF_LINK_FIELD_CONST (Time, contact_time);
	THIEF_LINK_FIELD_CONST (Time, firsthand_time);
	void update_contact (const Vector&, Time, bool firsthand);

	THIEF_LINK_FIELD (int, vision_cone);

	THIEF_LINK_FIELD_CONST (Time, update_time);
	THIEF_LINK_FIELD_CONST (Time, los_update_time);
	void update (Time, bool have_los);
};



THIEF_FLAVORED_LINK (CreatureAttachment)
{
	THIEF_FLAVORED_LINK_COMMON (CreatureAttachment)

	static CreatureAttachmentLink create (const Object& source,
		const Object& dest, AI::Joint = AI::Joint::NONE);

	THIEF_LINK_FIELD (AI::Joint, joint);
};



THIEF_FLAVORED_LINK (DetailAttachement)
{
	THIEF_FLAVORED_LINK_COMMON (DetailAttachement)

	enum class Type { OBJECT, VHOT, JOINT, SUBMODEL, SUBOBJECT, DECAL };

	static DetailAttachementLink create (const Object& source,
		const Object& dest, Type, int vhot_submodel = 0,
		AI::Joint = AI::Joint::NONE, Vector rel_location = Vector (),
		Vector rel_rotation = Vector (), Vector decal_normal = Vector (),
		float decal_dist = 0.0f);

	THIEF_LINK_FIELD (Type, type);
	THIEF_LINK_FIELD (int, vhot_submodel);
	THIEF_LINK_FIELD (AI::Joint, joint);

	THIEF_LINK_FIELD (Vector, rel_location);
	THIEF_LINK_FIELD (Vector, rel_rotation);

	THIEF_LINK_FIELD (Vector, decal_normal);
	THIEF_LINK_FIELD (float, decal_dist);
};



class AIActionResultMessage : public Message //TESTME
{
public:
	enum Action { NONE, GOTO, FROB, MANEUVER };
	enum Result { DONE, FAILED, NOT_ATTEMPTED };

	AIActionResultMessage (Action action, Result result,
		const Object& target, const LGMultiBase& data);
	THIEF_MESSAGE_WRAP (AIActionResultMessage);

	Action get_action () const;
	Result get_result () const;
	Object get_target () const;
	MULTI_GET (get_result_data);

private:
	void _get_result_data (LGMultiBase& value) const;
};



class AIAlertnessMessage : public Message //TESTME HighAlert only
{
public:
	AIAlertnessMessage (AI::Alert new_level, AI::Alert old_level,
		bool high_alert = false);
	THIEF_MESSAGE_WRAP (AIAlertnessMessage);

	bool is_high_alert () const;
	AI::Alert get_new_level () const;
	AI::Alert get_old_level () const;
};



class AIModeChangeMessage : public Message
{
public:
	AIModeChangeMessage (AI::Mode new_mode, AI::Mode old_mode);
	THIEF_MESSAGE_WRAP (AIModeChangeMessage);

	AI::Mode get_new_mode () const;
	AI::Mode get_old_mode () const;
};



class AIMotionMessage : public Message //TESTME
{
public:
	enum Event { START, END, FLAG_REACHED };

	AIMotionMessage (Event event, const char* motion_name,
		int motion_flag = 0);
	THIEF_MESSAGE_WRAP (AIMotionMessage);

	Event get_event () const;
	const char* get_motion_name () const;
	int get_motion_flag () const;
};



class AIPatrolPointMessage : public Message //TESTME
{
public:
	AIPatrolPointMessage (const Object& patrol_point);
	THIEF_MESSAGE_WRAP (AIPatrolPointMessage);

	Marker get_patrol_point () const;
};



class AISignalMessage : public Message //TESTME
{
public:
	AISignalMessage (const char* signal);
	THIEF_MESSAGE_WRAP (AISignalMessage);

	String get_signal () const;
};



// ConversationMessage: "ConversationEnd", sent when a subscribed conversation
//	ends (either after the last normal step or after the abort step)

class ConversationMessage : public Message
{
public:
	ConversationMessage (const Object& conversation);
	THIEF_MESSAGE_WRAP (ConversationMessage);

	Conversation get_conversation () const;
};



} // namespace Thief

#include <Thief/AI.inl>

#endif // THIEF_AI_HH

