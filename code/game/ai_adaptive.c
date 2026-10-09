// ai_adaptive.c
//
// zzqAI M5 -- 连续自适应难度
//
// 见 ai_adaptive.h 的设计说明。核心是三件事：
//
//   1) 每个评估周期统计"同场其他玩家的平均分"，作为该 bot 的参照系；
//   2) 把自己的得分与参照系比较，落后为正误差、领先为负误差；
//   3) 把误差折算成 skill 偏移，每周期最多移动 bot_adaptive_rate，避免跳变。
//
// 2026-10-09

#include "ai_main.h"
#include "ai_vars.h"
#include "ai_adaptive.h"

#include "ai_client.h"
#include "ai_entity.h"
#include "ai_self.h"


// 每个 bot 的原始 skill（即 addbot 时指定的值）
static float adaptive_base[MAX_CLIENTS];

// 上一帧写进 bs->settings.skill 的值，用于检测"这个 client 换了机器人"或
// 有外部代码改写了 skill —— 一旦对不上就重新以当前值为基准。
static float adaptive_next[MAX_CLIENTS];

// 伺服状态
static float adaptive_offset[MAX_CLIENTS];	// 当前 skill 偏移
static float adaptive_score[MAX_CLIENTS];	// 最近一次评估时该 bot 的得分
static float adaptive_peer[MAX_CLIENTS];	// 最近一次评估时的同场平均分
static int   adaptive_count[MAX_CLIENTS];	// 参与评估的轮数

// 本帧的得分快照（用于算"其他玩家的平均分"）
static float adaptive_scratch[MAX_CLIENTS];

// 全局面板
static float adaptive_next_eval;			// 下次评估时刻
static int   adaptive_evals;				// 评估总轮数


/*
==================
LevelAdaptiveReset
==================
*/
void LevelAdaptiveReset(void)
{
	memset(adaptive_base,   0, sizeof(adaptive_base));
	memset(adaptive_next,   0, sizeof(adaptive_next));
	memset(adaptive_offset, 0, sizeof(adaptive_offset));
	memset(adaptive_score,  0, sizeof(adaptive_score));
	memset(adaptive_peer,   0, sizeof(adaptive_peer));
	memset(adaptive_count,  0, sizeof(adaptive_count));
	memset(adaptive_scratch,0, sizeof(adaptive_scratch));

	adaptive_next_eval = 0;
	adaptive_evals = 0;
}

/*
==================
BotAdaptiveSkill

该 bot 当前的有效 skill：原始值 + 偏移，再按 cvar 区间裁剪。
==================
*/
float BotAdaptiveSkill(bot_state_t *bs)
{
	int i;
	float v;

	if (!bs || !bs->inuse)
		return 0;

	i = bs->client;
	if (i < 0 || i >= MAX_CLIENTS)
		return bs->settings.skill;

	v = adaptive_base[i];
	if (bot_adaptive.integer)
		v += adaptive_offset[i];

	if (v < bot_adaptive_min.value)
		v = bot_adaptive_min.value;
	if (v > bot_adaptive_max.value)
		v = bot_adaptive_max.value;

	return v;
}

/*
==================
BotAdaptiveApply

把有效 skill 写回 bs->settings.skill。

这是"零侵入"的关键：不改动任何读取 settings.skill 的代码，只在这里统一覆写。
于是感知半径、反应阈值、瞄准精度、物品预测深度全部自动跟随难度变化。
==================
*/
void BotAdaptiveApply(bot_state_t *bs)
{
	int i;
	float v;

	if (!bs || !bs->inuse)
		return;

	i = bs->client;
	if (i < 0 || i >= MAX_CLIENTS)
		return;

	// 首次见到这个 client，或检测到 settings.skill 被外部改写
	// （例如这个 client 位置换了另一个机器人），就重新建立基准。
	if (adaptive_base[i] <= 0.0 || fabs(bs->settings.skill - adaptive_next[i]) > 0.5)
	{
		adaptive_base[i] = bs->settings.skill;
		adaptive_offset[i] = 0.0;
	}

	v = BotAdaptiveSkill(bs);
	bs->settings.skill = v;
	adaptive_next[i] = v;
}

/*
==================
BotAdaptiveUpdate

每个服务器帧调用一次（在机器人思考之前）。按 bot_adaptive_interval 节流。
==================
*/
void BotAdaptiveUpdate(float elapsed)
{
	int i, num_players;
	float total, score, peer, err, denom, target, move, step;
	gentity_t *ent;
	bot_state_t *bs;

	// 关了就不做任何事（BotAdaptiveApply 会把 skill 还原成原始值）
	if (!bot_adaptive.integer)
		return;

	// 节流：每个评估周期只跑一次
	if (server_time < adaptive_next_eval)
		return;

	step = bot_adaptive_interval.value;
	if (step < 0.25)
		step = 0.25;
	adaptive_next_eval = server_time + step;
	adaptive_evals++;

	// 收集本帧所有在场玩家的得分（人类和机器人都算，观战者不算）
	num_players = 0;
	total = 0;
	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++)
	{
		ent = &g_entities[i];
		if (!ent->inuse || !ent->client)
			continue;
		if (ent->client->pers.connected != CON_CONNECTED)
			continue;
		if (EntityTeam(ent) == TEAM_SPECTATOR)
			continue;

		score = ent->client->ps.persistant[PERS_SCORE];
		adaptive_scratch[i] = score;
		total += score;
		num_players++;
	}

	// 场上不足两人就没有"参照系"，本轮跳过
	if (num_players < 2)
		return;

	// 逐个机器人评估
	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++)
	{
		bs = bot_states[i];
		if (!bs || !bs->inuse)
			continue;

		ent = bs->ent;
		if (!ent || !ent->client)
			continue;
		if (ent->client->pers.connected != CON_CONNECTED)
			continue;

		score = adaptive_scratch[i];
		peer = (total - score) / (num_players - 1);

		// 归一化误差：落后为正、领先为负，大致落在 [-1, 1]
		denom = fabs(peer) + fabs(score) + 1.0;
		err = (peer - score) / denom;
		if (err > 1.0)
			err = 1.0;
		else if (err < -1.0)
			err = -1.0;

		// 目标偏移
		target = err * bot_adaptive_range.value;
		if (target > bot_adaptive_range.value)
			target = bot_adaptive_range.value;
		else if (target < -bot_adaptive_range.value)
			target = -bot_adaptive_range.value;

		// 平滑：每周期最多移动 bot_adaptive_rate
		step = bot_adaptive_rate.value;
		if (step < 0)
			step = 0;

		move = target - adaptive_offset[i];
		if (move > step)
			move = step;
		else if (move < -step)
			move = -step;
		adaptive_offset[i] += move;

		// 幅度裁剪（用户可能中途改小了 range）
		if (adaptive_offset[i] > bot_adaptive_range.value)
			adaptive_offset[i] = bot_adaptive_range.value;
		else if (adaptive_offset[i] < -bot_adaptive_range.value)
			adaptive_offset[i] = -bot_adaptive_range.value;

		adaptive_score[i] = score;
		adaptive_peer[i] = peer;
		adaptive_count[i]++;

		// 诊断输出
		if (bot_adaptive_debug.integer)
		{
			BotAI_Print(PRT_MESSAGE,
				"zzqAI M5: %s 得分 %.0f / 同场均分 %.1f -> 偏移 %.2f，有效 skill %.2f\n",
				EntityNameFast(ent), score, peer,
				adaptive_offset[i], BotAdaptiveSkill(bs));
		}
	}
}

/*
==================
BotAdaptiveReport

打印一次难度伺服汇总（bot_adaptive_report 1，自动清零）。
==================
*/
void BotAdaptiveReport(void)
{
	int i;
	bot_state_t *bs;
	float base;

	BotAI_Print(PRT_MESSAGE, "===== zzqAI M5 难度伺服报告 =====\n");
	BotAI_Print(PRT_MESSAGE, "%-16s %-8s %-8s %-8s %-8s %-8s\n",
				"机器人", "原始", "偏移", "有效", "得分", "同场均分");

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		bs = bot_states[i];
		if (!bs || !bs->inuse || !bs->ent || !bs->ent->inuse)
			continue;
		if (!bs->ent->client || bs->ent->client->pers.connected != CON_CONNECTED)
			continue;

		base = (adaptive_base[i] > 0.0 ? adaptive_base[i] : bs->settings.skill);
		BotAI_Print(PRT_MESSAGE, "%-16s %-8.2f %-8.2f %-8.2f %-8.0f %-8.1f\n",
					EntityNameFast(bs->ent),
					base, adaptive_offset[i], BotAdaptiveSkill(bs),
					adaptive_score[i], adaptive_peer[i]);
	}

	BotAI_Print(PRT_MESSAGE,
				"评估轮数 %i，开关 %i，步长 %.2f，幅度 %.2f，区间 %.1f ~ %.1f\n",
				adaptive_evals, bot_adaptive.integer,
				bot_adaptive_rate.value, bot_adaptive_range.value,
				bot_adaptive_min.value, bot_adaptive_max.value);
	BotAI_Print(PRT_MESSAGE, "=================================\n");
}
