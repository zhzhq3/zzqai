// ai_duel.c
//
// zzqAI M3/M4 -- 决斗策略机 + 战斗走位（实现）
//
// 详见 ai_duel.h 的设计说明。这里只强调三个实现取舍：
//
// 1) 优势比用"可承受伤害"折算，而不是裸血量。
//    EntityHealth() 已经把护甲按 2/3 的价值折算进去，且考虑了 Battle Suit 减半，
//    所以用 EntityHealth(我方) / BotEnemyHealth(敌方) 得到的比值才可跨装备比较。
//    敌方护甲无法观察（保留信息不完全，不做透视），按 0 计，即"低估自己优势"，
//    这是保守方向，不会让机器人变得鲁莽。
//
// 2) 态势切换有迟滞（DUEL_STANCE_COMMIT），避免在阈值附近每帧抖动；
//    但"进入脱离"是立即生效的——保命要紧，不设延迟。
//
// 3) 脱离时只改"去哪儿"和"怎么走"，绝不改瞄准。机器人依然会朝敌人开枪
//    （边退边打），只是不再把自己送进打不赢的交火里。
//
// 2026-10-09

#include "ai_main.h"
#include "ai_vars.h"
#include "ai_duel.h"

#include "ai_entity.h"
#include "ai_item.h"
#include "ai_level.h"
#include "ai_self.h"
#include "ai_visible.h"
#include "ai_weapon.h"


// 挨打之后多少秒内仍算"在交战"（秒）
#define DUEL_ENGAGE_TIMEOUT		3.0

// 态势最短维持时间（秒）——防止在阈值附近每帧抖动
#define DUEL_STANCE_COMMIT		0.35

// 脱离阈值
#define DUEL_RETREAT_STACK		55.0	// 自身可承受伤害低于此值 → 考虑脱离
#define DUEL_RETREAT_ADV		0.80	// 优势比低于此值 → 考虑脱离

// 压上阈值
#define DUEL_PRESS_ADV			1.45	// 优势比高于此值 → 压上
#define DUEL_PRESS_STACK		85.0	// 压上还要求自身可承受伤害足够
#define DUEL_PRESS_ENEMY_LOW	45.0	// 敌方残血（可承受伤害低于此值）→ 压上

// 绕圈横移的距离窗口（世界单位）
#define DUEL_CIRCLE_MIN_DIST	72.0
#define DUEL_CIRCLE_MAX_DIST	768.0

// 绕圈方向切换间隔（秒）
#define DUEL_CIRCLE_SWITCH		1.4

// 脱离时愿意跑多远去找补血甲（世界单位）
#define DUEL_REGROUP_MAX_DIST	1600.0


/*
================
BotDuelStack

本机"可承受伤害"——血量 + 护甲折算 (+ 战衣翻倍)。
这是跨装备可比的口径，直接用裸血量会误判。
================
*/
static float BotDuelStack(bot_state_t *bs)
{
	float stack;

	stack = (float) EntityHealth(bs->ent);
	if (stack < 1.0)
		stack = 1.0;
	return stack;
}

/*
================
BotDuelEnemyStack

对敌方"可承受伤害"的估计。brainworks 原版已有：
  - skill >= 4 或非玩家目标：直接读到真实血量（相当于高水平玩家"手感准"）
  - 其他：靠痛叫声累计的 bs->enemy_health
敌方护甲不可观察，按 0 计（保守，宁低估自己优势）。
================
*/
static float BotDuelEnemyStack(bot_state_t *bs)
{
	float stack;

	if (!bs->aim_enemy)
		return 125.0;

	stack = (float) BotEnemyHealth(bs);
	if (stack < 1.0)
		stack = 1.0;
	return stack;
}

/*
================
BotDuelReset
================
*/
void BotDuelReset(bot_state_t *bs)
{
	bs->duel_stance = DUEL_STANCE_NONE;
	bs->duel_advantage = 1.0;
	bs->duel_stance_time = 0.0;
	bs->duel_damage_time = 0.0;
	bs->duel_last_stack = 0.0;
	bs->duel_circle_time = 0.0;
	bs->duel_circle_side = 1;
	bs->duel_press_count = 0;
	bs->duel_retreat_count = 0;
	bs->duel_circle_count = 0;
}

/*
================
BotDuelStanceName
================
*/
const char *BotDuelStanceName(int stance)
{
	switch (stance)
	{
		case DUEL_STANCE_PRESS:		return "压上";
		case DUEL_STANCE_TRADE:		return "对枪";
		case DUEL_STANCE_DISENGAGE:	return "脱离";
		default:					return "未交战";
	}
}

/*
================
BotDuelStance / BotDuelAdvantage
================
*/
int BotDuelStance(bot_state_t *bs)
{
	return bs->duel_stance;
}

float BotDuelAdvantage(bot_state_t *bs)
{
	return bs->duel_advantage;
}

/*
================
BotDuelEngaged

"这场架已经打起来了"：要么正在瞄准某个敌人，要么刚挨过打。
只靠 aim_enemy 会漏掉"背后挨枪"的情形，所以补一个挨打计时。
================
*/
qboolean BotDuelEngaged(bot_state_t *bs)
{
	if (bs->aim_enemy)
		return qtrue;

	if ( bs->duel_damage_time > 0.0 &&
		 (bs->command_time - bs->duel_damage_time) < DUEL_ENGAGE_TIMEOUT )
		return qtrue;

	return qfalse;
}

/*
================
BotDuelUpdate

每个机器人帧更新一次决斗态势。放在动作链中 BotActionGoal 之前，
所以 BotGoalDuelChoice() 和 BotMoveEnemyDir() 读到的都是本帧的值。
================
*/
void BotDuelUpdate(bot_state_t *bs)
{
	float my_stack, enemy_stack, adv;
	int stance;
	qboolean engaged;

	// 死亡 / 观战 / 中途：没有态势
	if (BotIsDead(bs) || BotIsObserver(bs) || BotInIntermission(bs))
	{
		bs->duel_stance = DUEL_STANCE_NONE;
		bs->duel_advantage = 1.0;
		bs->duel_last_stack = 0.0;
		return;
	}

	// M3/M4 全部关闭时直接短路：不判定、不计数、不打印，完全等同原版
	if (!bot_duel.integer && !bot_duel_move.integer)
	{
		bs->duel_stance = DUEL_STANCE_NONE;
		bs->duel_advantage = 1.0;
		bs->duel_last_stack = 0.0;
		return;
	}

	my_stack = BotDuelStack(bs);

	// 正在挨打？（本帧可承受伤害比上帧明显下降）
	if (bs->duel_last_stack > my_stack + 0.5)
		bs->duel_damage_time = bs->command_time;
	bs->duel_last_stack = my_stack;

	engaged = BotDuelEngaged(bs);

	if (!engaged)
	{
		bs->duel_stance = DUEL_STANCE_NONE;
		bs->duel_advantage = 1.0;
		return;
	}

	enemy_stack = BotDuelEnemyStack(bs);
	adv = my_stack / enemy_stack;

	//
	// 判定态势（顺序即优先级）
	//

	if ( bs->aim_enemy &&
		 bs->ps->powerups[PW_QUAD] &&
		 !bs->aim_enemy->client->ps.powerups[PW_QUAD] )
	{
		// 我方有 Quad 对面没有——这个窗口不用来杀人就是浪费
		stance = DUEL_STANCE_PRESS;
	}
	else if (enemy_stack <= DUEL_PRESS_ENEMY_LOW)
	{
		// 对面残血——收人头，别让他跑回去补血
		stance = DUEL_STANCE_PRESS;
	}
	else if ( my_stack <= DUEL_RETREAT_STACK || adv < DUEL_RETREAT_ADV )
	{
		// 血甲见底 或 伤害交换明显吃亏——脱离
		stance = DUEL_STANCE_DISENGAGE;
	}
	else if ( adv >= DUEL_PRESS_ADV && my_stack >= DUEL_PRESS_STACK )
	{
		// 明显占优——压上
		stance = DUEL_STANCE_PRESS;
	}
	else
	{
		stance = DUEL_STANCE_TRADE;
	}

	//
	// 迟滞：进入"脱离"立即生效；其余切换要等最短维持时间过去
	//
	if ( bs->duel_stance != DUEL_STANCE_NONE &&
		 bs->duel_stance != stance &&
		 stance != DUEL_STANCE_DISENGAGE &&
		 (bs->command_time - bs->duel_stance_time) < DUEL_STANCE_COMMIT )
	{
		stance = bs->duel_stance;
	}

	// 记录与诊断
	if (stance != bs->duel_stance)
	{
		if (stance == DUEL_STANCE_DISENGAGE)
			bs->duel_retreat_count++;
		else if (stance == DUEL_STANCE_PRESS)
			bs->duel_press_count++;

		if (bot_duel_debug.integer)
			BotAI_Print(PRT_MESSAGE,
						"zzqAI M3: %s 态势 %s -> %s (优势比 %.2f, 自身 %.0f, 敌方 %.0f)\n",
						EntityNameFast(bs->ent),
						BotDuelStanceName(bs->duel_stance),
						BotDuelStanceName(stance),
						adv, my_stack, enemy_stack);

		bs->duel_stance = stance;
		bs->duel_stance_time = bs->command_time;
	}

	bs->duel_advantage = adv;
}

/*
================
BotDuelShouldRetreat

该不该撤？（M4：走位层）
================
*/
qboolean BotDuelShouldRetreat(bot_state_t *bs)
{
	if (!bot_duel_move.integer)
		return qfalse;

	if (!BotDuelEngaged(bs))
		return qfalse;

	return (bs->duel_stance == DUEL_STANCE_DISENGAGE);
}

/*
================
BotDuelShouldCircle

该不该绕圈横移？（M4：走位层）

条件：均势或优势、有瞄准敌人、站在地上、距离在合理交火窗口内。
================
*/
qboolean BotDuelShouldCircle(bot_state_t *bs)
{
	float dist;

	if (!bot_duel.integer || !bot_duel_move.integer)
		return qfalse;

	if (!bs->aim_enemy)
		return qfalse;

	if ( bs->duel_stance != DUEL_STANCE_TRADE &&
		 bs->duel_stance != DUEL_STANCE_PRESS )
		return qfalse;

	// 只在地面上绕圈（空中/水里交给原版的闪避与移动逻辑）
	if (bs->now.physics.type != PHYS_GROUND)
		return qfalse;

	dist = sqrt(DistanceSquared(bs->now.origin, bs->aim_enemy->r.currentOrigin));
	if (dist < DUEL_CIRCLE_MIN_DIST || dist > DUEL_CIRCLE_MAX_DIST)
		return qfalse;

	return qtrue;
}

/*
================
BotDuelRegroupItem

脱离态势下最近的"补血甲"点。

选取规则：
  - 必须是血包（IT_HEALTH）或护甲（IT_ARMOR）
  - 距离不超过 DUEL_REGROUP_MAX_DIST
  - 并且**不能比敌人更靠近敌人**：物品到敌人的距离必须大于物品到自己的距离，
    否则等于往枪口上撞（这条是"脱离"能真正生效的关键）
================
*/
gentity_t *BotDuelRegroupItem(bot_state_t *bs)
{
	int i;
	gentity_t *ent, *best;
	float dist, best_dist, enemy_dist;

	if (!bs->aim_enemy)
		return NULL;

	best = NULL;
	best_dist = -1.0;

	for (i = 0; i < level.num_entities; i++)
	{
		ent = &g_entities[i];

		if (!ent->inuse || !ent->item)
			continue;

		if (ent->item->giType != IT_HEALTH && ent->item->giType != IT_ARMOR)
			continue;

		dist = sqrt(DistanceSquared(ent->r.currentOrigin, bs->now.origin));
		if (dist > DUEL_REGROUP_MAX_DIST)
			continue;

		// 不要朝敌人的方向跑
		enemy_dist = sqrt(DistanceSquared(ent->r.currentOrigin,
										  bs->aim_enemy->r.currentOrigin));
		if (enemy_dist < dist)
			continue;

		if (best_dist < 0.0 || dist < best_dist)
		{
			best_dist = dist;
			best = ent;
		}
	}

	return best;
}

/*
================
BotDuelRetreatDir

脱离方向：背离敌人，并带一点横向分量，避免走成一条直线被人预判。
================
*/
void BotDuelRetreatDir(bot_state_t *bs, gentity_t *ent, vec3_t dir)
{
	vec3_t away, side;
	vec3_t up = { 0, 0, 1 };

	VectorSubtract(bs->now.origin, ent->r.currentOrigin, away);
	away[2] = 0.0;

	// 敌人正好在正上/正下方时，没有水平背离方向，随便挑一个
	if (VectorNormalize(away) < 0.1)
	{
		VectorSet(away, cos(bs->command_time), sin(bs->command_time), 0);
		VectorNormalize(away);
	}

	// 加最多 35 度的横向偏角
	CrossProduct(up, away, side);
	VectorNormalize(side);
	VectorMA(away, 0.7 * (bs->duel_circle_side >= 0 ? 1.0 : -1.0), side, dir);
	VectorNormalize(dir);
}

/*
================
BotDuelCircleDir

绕圈方向：以敌人为中心，垂直于"敌人→我"的水平向量。
每 DUEL_CIRCLE_SWITCH 秒换一次方向，模拟玩家的"左右变向"。
================
*/
void BotDuelCircleDir(bot_state_t *bs, gentity_t *ent, vec3_t dir)
{
	vec3_t to_enemy, side;
	vec3_t up = { 0, 0, 1 };

	VectorSubtract(ent->r.currentOrigin, bs->now.origin, to_enemy);
	to_enemy[2] = 0.0;

	if (VectorNormalize(to_enemy) < 0.1)
	{
		VectorSet(side, cos(bs->command_time), sin(bs->command_time), 0);
		VectorNormalize(side);
		VectorCopy(side, dir);
		return;
	}

	// 换向
	if ((bs->command_time - bs->duel_circle_time) > DUEL_CIRCLE_SWITCH)
	{
		bs->duel_circle_time = bs->command_time;
		bs->duel_circle_side = (rand() & 1) ? 1 : -1;
		bs->duel_circle_count++;
	}

	CrossProduct(up, to_enemy, side);
	VectorNormalize(side);
	if (bs->duel_circle_side < 0)
		VectorNegate(side, side);

	VectorCopy(side, dir);
}

/*
================
BotDuelReport

把当前所有机器人的态势汇总打印一次（bot_duel_report 1）。
这是给实跑验证用的直接证据。
================
*/
void BotDuelReport(void)
{
	int i;
	bot_state_t *bs;

	BotAI_Print(PRT_MESSAGE, "===== zzqAI M3/M4 决斗态势报告 =====\n");
	BotAI_Print(PRT_MESSAGE, "%-12s %-8s %-8s %-8s %-8s %-8s\n",
				"机器人", "态势", "优势比", "压上", "脱离", "绕圈换向");

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		bs = bot_states[i];
		if (!bs || !bs->inuse || !bs->ent || !bs->ent->inuse)
			continue;
		if (!bs->ent->client || bs->ent->client->pers.connected != CON_CONNECTED)
			continue;

		BotAI_Print(PRT_MESSAGE, "%-12s %-8s %-8.2f %-8i %-8i %-8i\n",
					EntityNameFast(bs->ent),
					BotDuelStanceName(bs->duel_stance),
					bs->duel_advantage,
					bs->duel_press_count,
					bs->duel_retreat_count,
					bs->duel_circle_count);
	}
	BotAI_Print(PRT_MESSAGE, "===================================\n");
}
