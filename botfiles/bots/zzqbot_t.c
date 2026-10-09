//===========================================================================
//
// Name:			zzqbot_t.c
// Function:		zzqBOT 聊天台词（2026-10-09 设计）
// Notes:			chat 名字必须与 zzqbot_c.c 的 CHARACTERISTIC_CHAT_NAME 一致。
// Tab Size:		4 (real tabs)
//===========================================================================

chat "zzqbot"
{
	//the teamplay.h file is included for all kinds of teamplay chats
	#include "teamplay.h"

	//======================================================
	//======================================================

	type "game_enter" //initiated when the bot enters the game
	{
		"zzqBOT online. Let's see who survives.";
		"Systems green. Targets acquired.";
		"Another arena, another set of problems.";
		"Hello, ", 1, ". Try to keep up.";
	} //end type

	type "game_exit" //initiated when the bot exits the game
	{
		"Connection terminated. Good game.";
		"Analysis complete. Signing off.";
		"Leaving while I'm ahead.";
	} //end type

	type "level_start" //initiated when a new level starts
	{
		"I already know where the railgun spawns.";
		"Rule one: control the heavy weapons.";
		"Everyone starts equal. Not for long.";
		"Let the lesson begin.";
	} //end type

	type "level_end" //initiated when a level ends and the bot is not first and not last in the rankings
	{
		"Adequate. But not optimal.";
		"I'll adjust my routing next round.";
		"Interesting data. I'll use it.";
	} //end type

	type "level_end_victory" //initiated when a level ends and the bot is first in the rankings
	{
		"Predicted outcome achieved.";
		"Railgun, rockets, and better aim. Simple math.";
		"Control the map, control the score.";
		"Victory is just good logistics.";
	} //end type

	type "level_end_lose" //initiated when a level ends and the bot is last in the rankings
	{
		"Recalculating. That will not happen twice.";
		"I lost the weapon race. Noted.";
		"Next round I take the railgun first.";
	} //end type

	//======================================================
	//======================================================

	type "hit_talking" //bot is hit while chat balloon is visible
	{
		"Rude, ", 0, ". I was talking.";
		"That is a poor argument, ", 0, ".";
	} //end type

	type "hit_nodeath" //bot is hit by an opponent's weapon attack
	{
		"Close. Not close enough, ", 0, ".";
		"Good shot. There won't be a second.";
		"You have my attention now.";
		"That tickled.";
	} //end type

	type "hit_nokill" //bot hits an opponent but does not kill it
	{
		"Bleeding is a good look on you, ", 0, ".";
		"Run. I enjoy the chase.";
		"You are already dead, ", 0, ". You just haven't fallen.";
	} //end type

	//======================================================
	//======================================================

	type "death_telefrag" //the bot is killed by a telefrag
	{
		"Cheap trick, ", 0, ".";
		"A teleporter. How original.";
	} //end type

	type "death_cratered" //the bot is killed by falling damage
	{
		"Gravity remains undefeated.";
		"Noted: no shortcuts.";
	} //end type

	type "death_lava" //the bot dies in lava
	{
		"Temperature warnings were ignored.";
		"Lava: still hot.";
	} //end type

	type "death_slime" //the bot dies in slime
	{
		"Slime damage. Sloppy of me.";
	} //end type

	type "death_drown" //the bot drowns
	{
		"Airtanks, next revision.";
	} //end type

	type "death_suicide" //the bot blows itself up
	{
		"Rocket jump miscalculation.";
		"That is on me.";
	} //end type

	type "death_gauntlet" //the bot is killed by a gauntlet
	{
		"Melee range. Embarrassing, ", 0, ".";
	} //end type

	type "death_rail" //the bot is killed by a rail gun shot
	{
		"Clean shot, ", 0, ". I respect it.";
		"Railgun. Of course.";
	} //end type

	type "death_bfg" //the bot died by a BFG
	{
		"Big gun, small skill, ", 0, ".";
	} //end type

	type "death_insult" //insult initiated when the bot died
	{
		"A temporary setback, ", 0, ".";
		"Enjoy it. It won't repeat.";
		"Noted, ", 0, ". Now I know your habits.";
	} //end type

	type "death_praise" //praise initiated when the bot died
	{
		"Textbook execution, ", 0, ".";
		"You earned that one.";
	} //end type

	//======================================================
	//======================================================

	type "kill_rail" //the bot kills someone with rail gun
	{
		"Right through the eye, ", 0, ".";
		"Railgun. Every time.";
		"Predictable movement, ", 0, ".";
	} //end type

	type "kill_gauntlet" //the bot kills someone with gauntlet
	{
		"Out of ammo is not an excuse.";
		"Hand to hand, ", 0, "? Bold.";
	} //end type

	type "kill_telefrag" //the bot telefragged someone
	{
		"Move, ", 0, ".";
		"Occupied.";
	} //end type

	type "kill_suicide" //the player kills self
	{
		"Did that help, ", 0, "?";
		"Self-inflicted. My favorite kind.";
	} //end type

	type "kill_insult" //insult initiated when the bot killed someone
	{
		"Collect the armor first, ", 0, ". Then we talk.";
		"You brought a machinegun to a railgun fight.";
		"Good positioning. For me.";
		"", 0, ", meet the respawn queue.";
		"That is what map control looks like.";
	} //end type

	type "kill_praise" //praise initiated when the bot killed someone
	{
		"Good fight, ", 0, ".";
		"You almost had the angle right.";
	} //end type

	//======================================================
	//======================================================

	type "random_insult"
	{
		"", 0, ", you are moving exactly where I predicted.";
		"Does anyone else want the railgun? I'll take it anyway.";
		"Camping is a strategy. Losing is not.";
		"", 0, ", your aim needs a patch.";
	} //end type

	type "random_misc"
	{
		"Armor decays. Discipline doesn't.";
		"I never leave a rocket launcher behind.";
		"Quad in ", 0, " seconds. Get ready.";
		"Map control beats reflexes.";
	} //end type
} //end zzqbot chat
