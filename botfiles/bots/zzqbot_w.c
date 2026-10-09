//===========================================================================
//
// Name:			zzqbot_w.c
// Function:		zzqBOT 武器偏好权重（2026-10-09 设计）
// Notes:			轨道炮 / 火箭筒 为偏好武器，权重最高；
//					近战与骚扰武器权重压低。
//					实际武器切换逻辑见 bots/zzqbot/fw_weap.c
// Tab Size:		4 (real tabs)
//===========================================================================

#include "inv.h"

#define W_GAUNTLET				10
#define W_SHOTGUN				45
#define W_MACHINEGUN			20
#define W_GRENADELAUNCHER		50
#define W_ROCKETLAUNCHER		300
#define W_RAILGUN				640
#define W_BFG10K				150
#define W_LIGHTNING				210
#define W_PLASMAGUN				190
#define W_GRAPPLE				15

//
#include "bots/zzqbot/fw_weap.c"
