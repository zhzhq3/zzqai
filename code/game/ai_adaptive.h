// ai_adaptive.h
//
// zzqAI M5 -- 连续自适应难度（Continuous Adaptive Difficulty）
//
// brainworks 原版只有 skill 1~5 五档整数难度，一旦开局就固定不变：玩家越打
// 越强，机器人还是那个水平；玩家是新手，机器人也不会手下留情。更糟的是
// 一支队伍里所有 bot 共用一个 skill，强的更强、弱的更弱，对局很快失衡。
//
// 本模块补上"难度伺服"这一层：
//
//   1) 每个 bot 单独评估——把自己这一局的得分和"同场其他玩家的平均得分"比较，
//      落后就提高自己的有效 skill，领先就降低。
//   2) 连续可调——skill 不再只能取 1/2/3/4/5，而是可以在 base ± range 之间
//      连续浮动（默认 ±1.5），所以难度的变化是平滑的而不是跳变的。
//   3) 有阻尼——每个评估周期（默认 6 秒）最多移动 rate（默认 0.30），避免
//      一个运气好的连杀就直接把难度拉满。
//
// 实现方式刻意做得"零侵入"：不改动任何一处读取 bs->settings.skill 的代码，
// 而是每个服务器帧把 base_skill + offset 写回 bs->settings.skill。于是
// 感知半径（ai_aware.c）、反应阈值（ai_scan.c）、瞄准精度（ai_self.c）、
// 物品预测深度（ai_pickup.c）等等全部自动跟随难度变化。
//
// 设计红线：只调"难度档位"，不直接改瞄准误差与伤害数值。玩家变强 → 机器人
// 变得更认真；玩家是新手 → 机器人放水。而不是让机器人变成超人类枪法。
//
// 注意：botlib 的角色数据（trap_BotLoadCharacter）在加载时已按原始 skill
// 冻结，运行期改 skill 不会重载性格；但每帧读取 skill 的逻辑会全部生效。
//
// 2026-10-09

#ifndef AI_ADAPTIVE_H
#define AI_ADAPTIVE_H

// 清空所有伺服状态（换图 / 重开时调用）
void LevelAdaptiveReset(void);

// 每个服务器帧更新一次（在机器人思考之前调用）
void BotAdaptiveUpdate(float elapsed);

// 该 bot 当前的有效 skill（base + offset，已 clamp）
float BotAdaptiveSkill(bot_state_t *bs);

// 把有效 skill 写回 bs->settings.skill（bot_adaptive 关闭时写回原始值）
void BotAdaptiveApply(bot_state_t *bs);

// 打印一次难度伺服汇总（bot_adaptive_report 1）
void BotAdaptiveReport(void);

#endif // AI_ADAPTIVE_H
