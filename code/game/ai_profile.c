// ai_profile.c
//
// zzqAI M1 -- 对手档案（Opponent Profile）
//
// 见 ai_profile.h 的说明。
//
// 2026-10-09

#include "ai_main.h"
#include "ai_vars.h"

#include "ai_profile.h"

#include "ai_client.h"
#include "ai_entity.h"
#include "ai_level.h"
#include "ai_region.h"
#include "ai_weapon.h"


// 每个玩家的画像
player_profile_t player_profile[MAX_CLIENTS];

// 热度每秒衰减比例（0.06 => 时间常数约 16.7 秒）
#define PROFILE_DECAY_PER_SEC	0.06

// 速度低于此值（单位/秒）视为"蹲着不动"
#define PROFILE_CAMP_SPEED		40.0

// 累计热度低于此值的玩家不参与预测（数据太少）
#define PROFILE_MIN_HEAT		0.75


/*
==================
LevelProfileReset

换图 / 重开游戏时清空所有画像。
==================
*/
void LevelProfileReset(void)
{
	memset(player_profile, 0, sizeof(player_profile));
}

/*
==================
LevelProfileUpdate

每个服务器帧更新一次所有玩家的画像。
elapsed = 距上次服务器帧推进的秒数（正常为 0.05）。
==================
*/
void LevelProfileUpdate(float elapsed)
{
	int i, region, weapon;
	float decay, speed;
	vec3_t delta;
	gentity_t *ent;
	player_profile_t *p;

	// 区域尚未建立时无法按区域统计
	if (!CanProcessRegions())
		return;

	if (elapsed <= 0.0)
		return;

	// 计算本帧的衰减系数（线性近似指数衰减）
	decay = 1.0 - PROFILE_DECAY_PER_SEC * elapsed;
	if (decay < 0.0)
		decay = 0.0;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		ent = &g_entities[i];
		p = &player_profile[i];

		// 先整体衰减
		for (region = 0; region < num_regions; region++)
		{
			p->heat[region] *= decay;
			if (p->heat[region] < 0.001)
				p->heat[region] = 0.0;
		}
		p->heat_total *= decay;
		p->camp_time  *= decay;
		p->move_time  *= decay;
		for (weapon = 0; weapon < WP_NUM_WEAPONS; weapon++)
			p->weapon_time[weapon] *= decay;

		// 只统计真正在场上的玩家
		if (!ent->inuse || !ent->client ||
			ent->client->pers.connected != CON_CONNECTED ||
			EntityTeam(ent) == TEAM_SPECTATOR)
		{
			p->was_alive = qfalse;
			VectorCopy(ent->r.currentOrigin, p->last_origin);
			continue;
		}

		// 死亡中：只在存活 -> 死亡的瞬间记一次死亡
		if (!EntityIsAlive(ent))
		{
			if (p->was_alive)
			{
				p->deaths++;
				p->last_death_time = server_time;
			}
			p->was_alive = qfalse;
			VectorCopy(ent->r.currentOrigin, p->last_origin);
			continue;
		}

		p->was_alive = qtrue;
		p->alive_time += elapsed;

		// ---- 区域热度 ----
		region = LevelNearestRegionIndex(ent->r.currentOrigin);
		if (region >= 0 && region < num_regions)
		{
			p->heat[region] += elapsed;
			p->heat_total   += elapsed;
			p->last_region = region;
		}

		// ---- 蹲守 / 移动 ----
		VectorSubtract(ent->r.currentOrigin, p->last_origin, delta);
		speed = VectorLength(delta) / elapsed;
		if (speed < PROFILE_CAMP_SPEED)
			p->camp_time += elapsed;
		else
			p->move_time += elapsed;
		VectorCopy(ent->r.currentOrigin, p->last_origin);

		// ---- 常用武器 ----
		weapon = ent->client->ps.weapon;
		if (weapon > WP_NONE && weapon < WP_NUM_WEAPONS)
			p->weapon_time[weapon] += elapsed;
	}
}

/*
==================
ProfileEnemySeen

某个敌人被（任一机器人）察觉/看到时记录它的位置。
配合已有的听觉事件系统，就等于"最后已知位置"。
==================
*/
void ProfileEnemySeen(gentity_t *seen, vec3_t origin)
{
	int index;

	if (!seen)
		return;

	index = seen - g_entities;
	if (index < 0 || index >= MAX_CLIENTS)
		return;

	player_profile[index].seen_time = server_time;
	VectorCopy(origin, player_profile[index].seen_origin);
}

/*
==================
ProfileWeaponMostUsed

返回该玩家被观察到使用最多的武器编号，没有数据返回 WP_NONE。
==================
*/
static int ProfileWeaponMostUsed(int index)
{
	int weapon, best;
	float value, best_value;

	best = WP_NONE;
	best_value = 0.0;
	for (weapon = WP_NONE + 1; weapon < WP_NUM_WEAPONS; weapon++)
	{
		value = player_profile[index].weapon_time[weapon];
		if (value > best_value)
		{
			best_value = value;
			best = weapon;
		}
	}
	return best;
}

/*
==================
ProfileHottestRegion

返回该玩家热度最高的区域下标，没有则返回 -1。
==================
*/
static int ProfileHottestRegion(int index, int exclude)
{
	int region, best;
	player_profile_t *p;

	p = &player_profile[index];
	best = -1;
	for (region = 0; region < num_regions; region++)
	{
		if (region == exclude)
			continue;
		if (best < 0 || p->heat[region] > p->heat[best])
			best = region;
	}
	if (best >= 0 && p->heat[best] <= 0.0)
		return -1;
	return best;
}

/*
==================
ProfilePrediction

预测"该机器人最可能碰上敌人"的位置，写入 location。
返回 qfalse 表示没有足够数据。

策略：
  1) 只考虑该机器人的敌人；
  2) 每个敌人取其热度最高的区域（长期习惯点）；
  3) 综合"该敌人的总热度"与"离机器人有多近"选出最该去看的那个点。
==================
*/
qboolean ProfilePrediction(bot_state_t *bs, vec3_t location)
{
	int i, region, best_region;
	float dist, weight, score, best_score;
	player_profile_t *p;
	gentity_t *ent;

	if (!CanProcessRegions())
		return qfalse;

	best_score = 0.0;
	best_region = -1;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		ent = &g_entities[i];
		p = &player_profile[i];

		// 只关心敌人的画像
		if (!ent->inuse || !ent->client)
			continue;
		if (!BotEnemyTeam(bs, ent))
			continue;

		// 数据太少的敌人不参与预测
		if (p->heat_total < PROFILE_MIN_HEAT)
			continue;

		// 找这个敌人热度最高的区域
		region = ProfileHottestRegion(i, -1);
		if (region < 0)
			continue;

		// 越近的区域越优先；越常出没的敌人越优先
		dist = sqrt(DistanceSquared(bs->now.origin, RegionLocation(&region_list[region])));
		weight = 1.0 / (1.0 + dist / 1024.0);
		score = p->heat_total * p->heat[region] * weight;

		if (score > best_score)
		{
			best_score = score;
			best_region = region;
		}
	}

	if (best_region < 0)
		return qfalse;

	VectorCopy(RegionLocation(&region_list[best_region]), location);
	return qtrue;
}

/*
==================
BotProfileReport

打印所有玩家的画像（由 bot_profile_debug cvar 触发）。
==================
*/
void BotProfileReport(void)
{
	int i, index, second;
	gentity_t *ent;
	player_profile_t *p;
	char name[MAX_NETNAME];

	G_Printf("===== zzqAI 对手档案 =====\n");

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		ent = &g_entities[i];
		p = &player_profile[i];

		if (!ent->inuse || !ent->client)
			continue;
		if (EntityTeam(ent) == TEAM_SPECTATOR)
			continue;

		EntityName(ent, name, sizeof(name));

		G_Printf("%s  热度 %.1f  蹲守 %.0f%%  存活 %.1fs  死亡 %i  常用武器 %s\n",
				 name,
				 p->heat_total,
				 (p->camp_time + p->move_time > 0.0)
					? (100.0 * p->camp_time / (p->camp_time + p->move_time))
					: 0.0,
				 p->alive_time,
				 p->deaths,
				 WeaponName(ProfileWeaponMostUsed(i)));

		index = ProfileHottestRegion(i, -1);
		if (index >= 0)
		{
			second = ProfileHottestRegion(i, index);
			G_Printf("    热点区域：%s (%.1f)", RegionName(&region_list[index]), p->heat[index]);
			if (second >= 0)
				G_Printf("  次热：%s (%.1f)", RegionName(&region_list[second]), p->heat[second]);
			G_Printf("\n");
		}
	}

	G_Printf("==========================\n");
}
