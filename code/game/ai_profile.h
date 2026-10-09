// ai_profile.h
//
// zzqAI M1 -- 对手档案（Opponent Profile）
//
// 在 brainworks 原有"按队伍 + 永久累积"的区域流量统计之外，新增一层
// "按玩家 + 随时间衰减"的长期行为模型：每个玩家在各区域停留的热度、
// 蹲守/移动时间、常用武器、死亡次数、最后一次被察觉的位置。
//
// 用途：
//   1) 当机器人丢失敌人时，把搜索视线偏向"该敌人最可能出现的区域"，
//      而不是像原版那样在 16 个方向里随机挑（见 ai_aim.c 的 BotSelectSearch）。
//   2) bot_profile_debug 输出每个玩家的画像，便于观察与调参。
//
// 设计取舍：热力图按"区域停留时长 + 衰减"统计，属于长期习惯；位置记忆只在
// 敌人被察觉（看到或听到）时更新，不读取隐身玩家的实时坐标，因此不会变成透视挂。
//
// 2026-10-09

#ifndef AI_PROFILE_H
#define AI_PROFILE_H

// 每个玩家一份画像
typedef struct player_profile_s
{
	float heat[MAX_REGIONS];			// 各区域的衰减热度（秒）
	float heat_total;					// 热度总和
	float camp_time;					// 衰减的"几乎静止"时间
	float move_time;					// 衰减的"移动中"时间
	float weapon_time[WP_NUM_WEAPONS];	// 观察到持各武器的时间
	int   last_region;					// 最后一次所在区域
	vec3_t last_origin;					// 上一次更新的坐标（算速度用）
	float alive_time;					// 存活累计时间
	float last_death_time;				// 最后一次死亡时刻
	int   deaths;						// 死亡次数
	qboolean was_alive;					// 上一帧是否存活（用来数死亡）
	float seen_time;					// 最后一次被察觉的时刻（0 = 从未）
	vec3_t seen_origin;					// 最后一次被察觉时的坐标
} player_profile_t;

// 所有玩家（按 client 索引）的画像
extern player_profile_t player_profile[MAX_CLIENTS];

// 清空所有画像（换图 / 重开时调用）
void LevelProfileReset(void);

// 每个服务器帧更新一次画像（elapsed = 距上次更新的秒数）
void LevelProfileUpdate(float elapsed);

// 预测某个敌人可能出现的位置：返回该机器人最可能碰到敌人的"热区"位置
qboolean ProfilePrediction(bot_state_t *bs, vec3_t location);

// 记录"某敌人被察觉到了"（由 ai_aware.c 的 BotAwareTrackEntity 调用）
void ProfileEnemySeen(gentity_t *seen, vec3_t origin);

// 打印所有玩家的画像报告（bot_profile_debug）
void BotProfileReport(void);

#endif // AI_PROFILE_H
