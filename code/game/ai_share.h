// ai_share.h
//
// zzqAI M6 -- 团队情报共享 + 角色化响应
//
// brainworks 原版已经有一整套队伍机制（队长选举、进攻/防守偏好、CTF 的
// 夺旗/回旗分配、语音指令），但缺了最基础的一件事：**互相告诉敌人在哪**。
// 每个机器人看到的敌人只有它自己知道；队友被杀时，其他人甚至不知道那边
// 刚发生过交火。于是团队模式下的机器人常常是一堆各自为战的个体。
//
// 本模块补上两件事：
//
//   1) 情报黑板 —— 每个队伍一块。谁察觉到了哪个敌人（视觉或听觉），就把
//      位置报进黑板。其他队友在"没有已知敌人"时，可以去最近的、还没过期的
//      情报点搜人。情报有存活时间（默认 8 秒），过期的自动作废。
//
//      关键的设计约束：**自己报的情报不算数**。自己的位置记忆是 M1（对手
//      档案）负责的东西；共享黑板只提供"从队友那里听来的、我自己没看到的"
//      信息。这条约束保证情报共享不会退化成"给每个 bot 加透视"。
//
//   2) 角色化响应 —— 队伍成员按进攻性分成三种角色，它们对情报的响应方式
//      不同：
//        突破（BREAKER）—— 进攻性最强的队员，会响应全场任何位置的情报；
//        守点（ANCHOR） —— 进攻性最弱的队员，只响应自己附近的情报，不乱跑；
//        策应（ESCORT） —— 其余队员，响应半径居中。
//      角色在队伍成员变化时重新分配（进攻性最高的当突破，最低的当守点），
//      所以不需要人工指定，一支队伍会自动分工。
//
// 只在团队模式（GS_TEAM）下生效；FFA 里没有队友可分享，模块完全静默。
//
// 2026-10-09

#ifndef AI_SHARE_H
#define AI_SHARE_H

// 每队最多同时存在多少条情报
#define MAX_TEAM_INTEL		8

// 队伍角色
#define TEAMROLE_NONE		0	// 未分配（非团队模式）
#define TEAMROLE_BREAKER	1	// 突破手：进攻性最强，响应全场情报
#define TEAMROLE_ANCHOR		2	// 守点：进攻性最弱，只在附近响应
#define TEAMROLE_ESCORT		3	// 策应：居中

// 重置所有情报与角色（换图 / 重开时调用）
void LevelShareReset(void);

// 每个服务器帧更新一次（负责角色分配）
void BotShareUpdate(float elapsed);

// 记录"我察觉到了这个敌人"，把位置报给队友（由 ai_aware.c 调用）
void ShareIntelReport(bot_state_t *bs, gentity_t *enemy, vec3_t origin);

// 查询队友的情报：返回是否有可用的情报点，并把位置写入 target
// age 返回该情报的新鲜度（秒），可为 NULL
qboolean ShareIntelFind(bot_state_t *bs, vec3_t target, float *age);

// 该 bot 的队伍角色
int BotShareRole(bot_state_t *bs);
const char *BotShareRoleName(int role);

// 打印一次角色分工与情报板状态（bot_share_report 1）
void BotShareReport(void);

#endif // AI_SHARE_H
