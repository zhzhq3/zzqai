// ai_share.c
//
// zzqAI M6 -- 团队情报共享 + 角色化响应
//
// 见 ai_share.h 的设计说明。核心是两块：
//
//   情报黑板：level_intel[队伍][槽位]，每条记录"谁在什么时候在哪看到了敌人"。
//             写入端是 ai_aware.c（机器人真的察觉到了敌人），读取端是
//             目标选择（BotGoalShareChoice）。
//
//   角色分配：按角色特征里的进攻性排序，队内最高的当突破、最低的当守点，
//             其余当策应；角色决定情报响应半径。
//
// 2026-10-09

#include "ai_main.h"
#include "ai_vars.h"
#include "ai_share.h"

#include "ai_client.h"
#include "ai_entity.h"
#include "ai_self.h"


// 一条情报
typedef struct team_intel_s
{
	qboolean valid;			// 槽位是否有效
	int      reporter;		// 上报者的 client 号
	vec3_t   origin;		// 上报时敌人所在位置
	float    report_time;	// 上报时刻（秒）
} team_intel_t;

// 每队一块情报板
static team_intel_t level_intel[TEAM_NUM_TEAMS][MAX_TEAM_INTEL];

// 每个 bot 的队伍角色
static int   bot_role[MAX_CLIENTS];

// 诊断
static int   share_reports;				// 累计上报条数
static int   share_recv[MAX_CLIENTS];	// 每个 bot 累计收到情报次数
static float share_lastlog[MAX_CLIENTS];// 日志节流
static float share_next_role;			// 下次角色重算时刻
static int   share_role_count;			// 上次重算时的在场 bot 数


/*
==================
LevelShareReset
==================
*/
void LevelShareReset(void)
{
	memset(level_intel,   0, sizeof(level_intel));
	memset(bot_role,      0, sizeof(bot_role));
	memset(share_recv,    0, sizeof(share_recv));
	memset(share_lastlog, 0, sizeof(share_lastlog));

	share_reports = 0;
	share_next_role = 0;
	share_role_count = -1;
}

/*
==================
BotShareRoleName
==================
*/
const char *BotShareRoleName(int role)
{
	switch (role)
	{
		case TEAMROLE_BREAKER:	return "突破";
		case TEAMROLE_ANCHOR:	return "守点";
		case TEAMROLE_ESCORT:	return "策应";
		default:				return "无";
	}
}

/*
==================
BotShareRole

该 bot 当前的队伍角色。
==================
*/
int BotShareRole(bot_state_t *bs)
{
	if (!bs || !bs->inuse)
		return TEAMROLE_NONE;
	if (bs->client < 0 || bs->client >= MAX_CLIENTS)
		return TEAMROLE_NONE;

	return bot_role[bs->client];
}

/*
==================
BotShareAggression

该 bot 的进攻性评分（越高越好斗）。用角色特征里的进攻性与蹲守倾向折算。
==================
*/
static float BotShareAggression(bot_state_t *bs)
{
	float aggr, camper;

	if (!bs || !bs->character)
		return 0.5;

	aggr   = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AGGRESSION, 0, 1);
	camper = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CAMPER, 0, 1);

	return aggr - camper * 0.5;
}

/*
==================
BotShareUpdateRoles

重新分配每队的角色。规则：
  进攻性最高 → 突破；进攻性最低（且队里有 2 人以上）→ 守点；其余 → 策应。
==================
*/
static void BotShareUpdateRoles(void)
{
	int team, i, j, k, n;
	bot_state_t *members[MAX_CLIENTS];
	float aggr[MAX_CLIENTS];
	bot_state_t *swap_bs;
	float swap_f;
	bot_state_t *bs;

	// 先全部清空
	for (i = 0; i < MAX_CLIENTS; i++)
		bot_role[i] = TEAMROLE_NONE;

	// 只有团队模式才谈得上分工
	if ( !(game_style & GS_TEAM) )
		return;

	for (team = 0; team < TEAM_NUM_TEAMS; team++)
	{
		if (team != TEAM_RED && team != TEAM_BLUE)
			continue;

		// 收集本队所有机器人
		n = 0;
		for (i = 0; i < maxclients && i < MAX_CLIENTS; i++)
		{
			bs = bot_states[i];
			if (!bs || !bs->inuse || !bs->ent || !bs->ent->client)
				continue;
			if (BotTeam(bs) != team)
				continue;

			members[n] = bs;
			aggr[n] = BotShareAggression(bs);
			n++;
		}
		if (!n)
			continue;

		// 按进攻性降序排序（冒泡，成员数量很小）
		for (j = 0; j < n - 1; j++)
		{
			for (k = 0; k < n - 1 - j; k++)
			{
				if (aggr[k] < aggr[k + 1])
				{
					swap_bs = members[k];   members[k]   = members[k + 1];   members[k + 1] = swap_bs;
					swap_f  = aggr[k];      aggr[k]      = aggr[k + 1];      aggr[k + 1]    = swap_f;
				}
			}
		}

		// 分配角色
		for (j = 0; j < n; j++)
		{
			int role;

			if (j == 0)
				role = TEAMROLE_BREAKER;
			else if (j == n - 1 && n >= 2)
				role = TEAMROLE_ANCHOR;
			else
				role = TEAMROLE_ESCORT;

			bot_role[members[j]->client] = role;
		}
	}
}

/*
==================
BotShareUpdate

每个服务器帧调用一次。角色分配本身不需要每帧做，节流到 5 秒。
==================
*/
void BotShareUpdate(float elapsed)
{
	int i, total;

	if (!bot_share.integer)
		return;

	// 统计在场机器人数量：一旦有机器人加入或离开就立刻重算角色，
	// 这样新开局的机器人不用等满一个节流周期就有角色。
	total = 0;
	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++)
	{
		if (bot_states[i] && bot_states[i]->inuse)
			total++;
	}

	if (total == share_role_count && server_time < share_next_role)
		return;

	share_role_count = total;
	share_next_role = server_time + 5.0;
	BotShareUpdateRoles();
}

/*
==================
ShareIntelReport

"我察觉到了这个敌人"—— 把位置报给队友。

由 ai_aware.c 的 BotAwareTrackEntity 在确认察觉之后调用。此时机器人确实是
通过视觉或听觉察觉到了敌人，不是凭空虚报。
==================
*/
void ShareIntelReport(bot_state_t *bs, gentity_t *enemy, vec3_t origin)
{
	team_intel_t *board;
	int team, i, slot;
	float oldest;

	if (!bot_share.integer)
		return;

	// 只有团队模式才有队友可分享
	if ( !(game_style & GS_TEAM) )
		return;

	if (!bs || !bs->inuse || !bs->ent || !bs->ent->client)
		return;
	if (!enemy || !enemy->inuse || !enemy->client)
		return;

	// 不共享队友的位置
	if (BotSameTeam(bs, enemy))
		return;

	team = BotTeam(bs);
	if (team != TEAM_RED && team != TEAM_BLUE)
		return;

	board = level_intel[team];

	// 优先覆盖自己上一次的报告，一个人只占一条，避免刷屏挤掉别人
	slot = -1;
	for (i = 0; i < MAX_TEAM_INTEL; i++)
	{
		if (board[i].valid && board[i].reporter == bs->client)
		{
			slot = i;
			break;
		}
	}

	// 其次找空位
	if (slot < 0)
	{
		for (i = 0; i < MAX_TEAM_INTEL; i++)
		{
			if (!board[i].valid)
			{
				slot = i;
				break;
			}
		}
	}

	// 最后抢占最旧的一条
	if (slot < 0)
	{
		oldest = board[0].report_time;
		slot = 0;
		for (i = 1; i < MAX_TEAM_INTEL; i++)
		{
			if (board[i].report_time < oldest)
			{
				oldest = board[i].report_time;
				slot = i;
			}
		}
	}

	board[slot].valid = qtrue;
	board[slot].reporter = bs->client;
	VectorCopy(origin, board[slot].origin);
	board[slot].report_time = server_time;

	share_reports++;
}

/*
==================
ShareIntelFind

查队友的情报：挑一条"最新 + 最近"的、还没过期的，并把位置写进 target。

返回 qfalse 表示没有可用情报（没有队友、情报都过期了、或者自己就是上报者）。
==================
*/
qboolean ShareIntelFind(bot_state_t *bs, vec3_t target, float *age)
{
	team_intel_t *board;
	int team, i, best;
	float t, dist, score, best_score, life, range;

	if (!bot_share.integer)
		return qfalse;

	if ( !(game_style & GS_TEAM) )
		return qfalse;

	if (!bs || !bs->inuse || !bs->ent)
		return qfalse;

	team = BotTeam(bs);
	if (team != TEAM_RED && team != TEAM_BLUE)
		return qfalse;

	life  = bot_share_lifetime.value;
	range = bot_share_range.value;

	// 角色修正响应半径：守点不乱跑，突破哪儿都去
	switch (BotShareRole(bs))
	{
		case TEAMROLE_ANCHOR:
			range *= 0.50;
			break;
		case TEAMROLE_ESCORT:
			range *= 0.75;
			break;
		default:
			break;
	}

	if (range < 256.0)
		range = 256.0;

	board = level_intel[team];
	best = -1;
	best_score = 0;

	for (i = 0; i < MAX_TEAM_INTEL; i++)
	{
		if (!board[i].valid)
			continue;

		// 自己报的不算——那是自己的位置记忆（M1 负责），不是队友情报
		if (board[i].reporter == bs->client)
			continue;

		t = server_time - board[i].report_time;
		if (t < 0 || t > life)
		{
			board[i].valid = qfalse;
			continue;
		}

		dist = sqrt(DistanceSquared(bs->now.origin, board[i].origin));

		// 太远够不着，或者太近不值得专门跑一趟
		if (dist > range || dist < 300.0)
			continue;

		// 打分：越新越好，越近越好
		score = (life - t) * 1000.0 / (dist + 200.0);

		if (score > best_score)
		{
			best_score = score;
			best = i;
		}
	}

	if (best < 0)
		return qfalse;

	VectorCopy(board[best].origin, target);
	if (age)
		*age = server_time - board[best].report_time;

	share_recv[bs->client]++;

	// 诊断输出（每个 bot 最多 1.5 秒一条，避免刷屏）
	if (bot_share_debug.integer && server_time > share_lastlog[bs->client] + 1.5)
	{
		share_lastlog[bs->client] = server_time;
		BotAI_Print(PRT_MESSAGE,
			"zzqAI M6: %s（%s）收到 %s 的情报 -> 距 %.0f，情报龄 %.1fs\n",
			EntityNameFast(bs->ent), BotShareRoleName(BotShareRole(bs)),
			EntityNameFast(&g_entities[board[best].reporter]),
			sqrt(DistanceSquared(bs->now.origin, board[best].origin)),
			server_time - board[best].report_time);
	}

	return qtrue;
}

/*
==================
BotShareReport

打印一次角色分工与情报板状态（bot_share_report 1，自动清零）。
==================
*/
void BotShareReport(void)
{
	int i, team, num;
	bot_state_t *bs;
	const char *teamname;

	BotAI_Print(PRT_MESSAGE, "===== zzqAI M6 团队情报报告 =====\n");
	BotAI_Print(PRT_MESSAGE, "%-16s %-6s %-6s %-8s\n",
				"机器人", "队伍", "角色", "收到情报");

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		bs = bot_states[i];
		if (!bs || !bs->inuse || !bs->ent || !bs->ent->inuse)
			continue;
		if (!bs->ent->client || bs->ent->client->pers.connected != CON_CONNECTED)
			continue;

		if (BotTeam(bs) == TEAM_RED)
			teamname = "红";
		else if (BotTeam(bs) == TEAM_BLUE)
			teamname = "蓝";
		else
			teamname = "-";

		BotAI_Print(PRT_MESSAGE, "%-16s %-6s %-6s %-8i\n",
					EntityNameFast(bs->ent), teamname,
					BotShareRoleName(BotShareRole(bs)),
					share_recv[i]);
	}

	for (team = 0; team < TEAM_NUM_TEAMS; team++)
	{
		if (team != TEAM_RED && team != TEAM_BLUE)
			continue;

		num = 0;
		for (i = 0; i < MAX_TEAM_INTEL; i++)
		{
			if (!level_intel[team][i].valid)
				continue;
			if (server_time - level_intel[team][i].report_time > bot_share_lifetime.value)
				continue;
			num++;
		}

		BotAI_Print(PRT_MESSAGE, "%s队情报板：%i 条有效（有效期 %.1fs，响应半径 %.0f）\n",
					(team == TEAM_RED ? "红" : "蓝"),
					num, bot_share_lifetime.value, bot_share_range.value);
	}

	BotAI_Print(PRT_MESSAGE, "累计情报上报 %i 条，开关 %i\n",
				share_reports, bot_share.integer);
	BotAI_Print(PRT_MESSAGE, "=================================\n");
}
