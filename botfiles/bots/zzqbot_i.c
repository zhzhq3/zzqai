//===========================================================================
//
// Name:			zzqbot_i.c
// Function:		zzqBOT 拾取物权重（2026-10-09 设计）
// Notes:			目标偏好：轨道炮 > 火箭筒 > 红甲/大血 > 强化道具。
//					权重文件（fw_items.c）沿用 baseq3 标准实现，
//					差异化全部通过下面的 W_ / GWW_ / FS_ 宏完成。
// Tab Size:		4 (real tabs)
//===========================================================================

#include "inv.h"

//initial health/armor states
#define FS_HEALTH				2
#define FS_ARMOR				2

//initial weapon weights
#define W_SHOTGUN				50
#define W_MACHINEGUN			40
#define W_GRENADELAUNCHER		40
#define W_ROCKETLAUNCHER		180
#define W_RAILGUN				520
#define W_BFG10K				130
#define W_LIGHTNING				170
#define W_PLASMAGUN				160

//the bot has the weapons, so the weights change a little bit
#define GWW_SHOTGUN				35
#define GWW_MACHINEGUN			50
#define GWW_GRENADELAUNCHER		30
#define GWW_ROCKETLAUNCHER		90
#define GWW_RAILGUN				420
#define GWW_BFG10K				101
#define GWW_LIGHTNING			130
#define GWW_PLASMAGUN			130

//initial powerup weights
#define W_TELEPORTER			60
#define W_MEDKIT				120
#define W_QUAD					450
#define W_ENVIRO				300
#define W_HASTE					250
#define W_INVISIBILITY			250
#define W_REGEN					350
#define W_FLIGHT				40

//flag weight
#define FLAG_WEIGHT				50

//
#include "fw_items.c"
