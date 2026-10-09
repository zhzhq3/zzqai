// ai_duel.h
//
// zzqAI M3/M4 -- 决斗策略机 + 战斗走位
//
// brainworks 原版已经具备感知（视觉 + 听觉）、瞄准提前量、物品簇与重生计时、
// 伤害比（BotEnemyDamageRatio）与闪避（MM_DODGE），但在"这一场要不要打、打到
// 什么程度、打不过怎么办"这一层是空白的：全工程没有任何 duel / 1v1 相关关键字，
// 机器人只要锁定敌人就会一直贴着打，哪怕自己只剩 10 点血、对面满血满甲。
//
// 本模块补上这一层：
//
//   M3 决斗策略机 —— 每帧算出"态势"(stance)：
//        PRESS      优势（血甲/武器明显占优，或对面残血）→ 压上收人头
//        TRADE      均势 → 正常对枪（行为等同原版）
//        DISENGAGE  劣势（血甲过低，或伤害交换明显吃亏）→ 脱离，改去补血甲
//     并提供一个 GOAL_DUEL_REGROUP 目标：脱离时优先去最近的血包/护甲，
//     而不是像原版那样继续追一个已经打不过的敌人。
//
//   M4 战斗走位 —— 在"已经站在理想交火距离"时不再原地站桩，而是绕圈横移
//     （circle strafe），保持面向敌人的同时让对方的准星一直追不上；劣势时
//     则是保持面向敌人向外后撤（边退边打）。
//
// 设计红线：不改瞄准精度、不改伤害数值，只改"站位与取舍"。所谓更强是决策
// 更强，不是枪法超人类。
//
// 2026-10-09

#ifndef AI_DUEL_H
#define AI_DUEL_H

// ---- 决斗态势 ----
#define DUEL_STANCE_NONE		0	// 未交战
#define DUEL_STANCE_PRESS		1	// 优势，压上
#define DUEL_STANCE_TRADE		2	// 均势，对枪
#define DUEL_STANCE_DISENGAGE	3	// 劣势，脱离

// ---- 战斗走位 ----
#define DUEL_MOVE_NONE			0
#define DUEL_MOVE_CIRCLE		1
#define DUEL_MOVE_RETREAT		2

// 重置（换图 / 重生 / 死亡时调用）
void BotDuelReset(bot_state_t *bs);

// 每个机器人帧更新一次态势（在 BotActions 的动作链里调用）
void BotDuelUpdate(bot_state_t *bs);

// 当前态势
int  BotDuelStance(bot_state_t *bs);
const char *BotDuelStanceName(int stance);

// 最近一次算出的优势比：>1 表示我方占优（按"可承受伤害"折算）
float BotDuelAdvantage(bot_state_t *bs);

// 是否处于有意义的交战状态（有瞄准敌人，或刚刚挨过打）
qboolean BotDuelEngaged(bot_state_t *bs);

// 是否应该脱离（劣势且还在交战）
qboolean BotDuelShouldRetreat(bot_state_t *bs);

// 是否应该绕圈横移（均势/优势，且已经站在理想交火距离）
qboolean BotDuelShouldCircle(bot_state_t *bs);

// 脱离时应该去哪个补血甲的点（返回 NULL 表示找不到）
gentity_t *BotDuelRegroupItem(bot_state_t *bs);

// 计算方向向量
void BotDuelRetreatDir(bot_state_t *bs, gentity_t *ent, vec3_t dir);

// 计算绕圈方向向量
void BotDuelCircleDir(bot_state_t *bs, gentity_t *ent, vec3_t dir);

// 打印一次所有机器人的态势汇总（bot_duel_report 1）
void BotDuelReport(void);

#endif // AI_DUEL_H
