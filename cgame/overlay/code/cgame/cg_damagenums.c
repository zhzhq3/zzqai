/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/*
===========================================================================

cg_damagenums.c -- 命中敌人时在其身上显示伤害数字

2026-10-09 为 brainworks 模组加入（cgame 基于 baseq3a + better-gibs）：

  服务端 (brainworks qagame) 在 G_Damage() 里，用服务器命令

      dmg <目标实体号> <伤害值>

  把"对敌方实际造成的伤害"发给攻击者客户端；本模块接收该命令，
  把数字绘制在敌人头顶（向上漂浮 + 淡出）。

  控制台变量：
    cg_damageNumbers       1 = 显示伤害数字（默认），0 = 关闭
    cg_damageNumbersScale  数字缩放（默认 1.0）
    cg_damageNumbersDebug  1 = 在控制台打印每一条收到的伤害（诊断用）

===========================================================================
*/

#include "cg_local.h"

#define MAX_DAMAGE_PLUMS		16
#define DAMAGE_PLUM_TIME		1000		// 数字存活时长(ms)
#define DAMAGE_PLUM_RISE		40.0f		// 存活期内向上漂浮的世界单位
#define DAMAGE_PLUM_HEIGHT		46.0f		// 相对敌人原点的起始高度
#define DAMAGE_PLUM_MERGE_MS	400			// 这段时间内对同一敌人的连续命中合并成一个数字
#define DAMAGE_PLUM_FONT_SCALE	0.5f		// 字号相对 BIGCHAR(16px) 的比例：0.5 = 8px（2026-10-09 减半）

typedef struct {
	qboolean	active;
	int			value;			// 已累计的伤害
	int			entnum;			// 目标实体号（对玩家来说就是 clientNum）
	int			startTime;
	vec3_t		origin;			// 收到伤害那一刻的世界坐标（实体离开快照后的兜底）
} damagePlum_t;

static damagePlum_t cg_damagePlums[MAX_DAMAGE_PLUMS];

/*
=================
CG_ClearDamagePlums
=================
*/
void CG_ClearDamagePlums( void ) {
	Com_Memset( cg_damagePlums, 0, sizeof( cg_damagePlums ) );
}

/*
=================
CG_AddDamagePlum
=================
*/
void CG_AddDamagePlum( int entnum, int value ) {
	int		i, slot, oldest;

	if ( value <= 0 ) {
		return;
	}
	if ( entnum < 0 || entnum >= MAX_GENTITIES ) {
		return;
	}

	// 短时间内打同一个敌人 -> 累加到已有的数字，避免糊成一片
	for ( i = 0; i < MAX_DAMAGE_PLUMS; i++ ) {
		if ( cg_damagePlums[i].active
			&& cg_damagePlums[i].entnum == entnum
			&& cg.time - cg_damagePlums[i].startTime <= DAMAGE_PLUM_MERGE_MS ) {
			cg_damagePlums[i].value += value;
			cg_damagePlums[i].startTime = cg.time;
			VectorCopy( cg_entities[entnum].lerpOrigin, cg_damagePlums[i].origin );
			return;
		}
	}

	// 找一个空槽；没有就覆盖最旧的
	slot = -1;
	oldest = 0;
	for ( i = 0; i < MAX_DAMAGE_PLUMS; i++ ) {
		if ( !cg_damagePlums[i].active ) {
			slot = i;
			break;
		}
		if ( cg_damagePlums[i].startTime < cg_damagePlums[oldest].startTime ) {
			oldest = i;
		}
	}
	if ( slot < 0 ) {
		slot = oldest;
	}

	cg_damagePlums[slot].active = qtrue;
	cg_damagePlums[slot].value = value;
	cg_damagePlums[slot].entnum = entnum;
	cg_damagePlums[slot].startTime = cg.time;
	VectorCopy( cg_entities[entnum].lerpOrigin, cg_damagePlums[slot].origin );

	if ( cg_damageNumbersDebug.integer ) {
		CG_Printf( "DamagePlum: ent=%i value=%i time=%i\n", entnum, value, cg.time );
	}
}

/*
=================
CG_DamagePlum_f

服务端命令入口：dmg <目标实体号> <伤害值>
=================
*/
void CG_DamagePlum_f( void ) {
	CG_AddDamagePlum( atoi( CG_Argv( 1 ) ), atoi( CG_Argv( 2 ) ) );
}

/*
=================
CG_WorldToScreen

把世界坐标投影到 cgame 的 640x480 虚拟屏幕坐标。
在视点后方或明显出屏时返回 qfalse。
=================
*/
static qboolean CG_WorldToScreen( const vec3_t point, float *x, float *y ) {
	vec3_t	trans;
	float	px, py, z;

	if ( cg.refdef.fov_x <= 0.0f || cg.refdef.fov_y <= 0.0f ) {
		return qfalse;
	}

	VectorSubtract( point, cg.refdef.vieworg, trans );

	z = DotProduct( trans, cg.refdef.viewaxis[0] );
	if ( z <= 1.0f ) {
		return qfalse;
	}

	px = tan( cg.refdef.fov_x * M_PI / 360.0f );
	py = tan( cg.refdef.fov_y * M_PI / 360.0f );
	if ( px <= 0.0f || py <= 0.0f ) {
		return qfalse;
	}

	*x = 320.0f - DotProduct( trans, cg.refdef.viewaxis[1] ) * 320.0f / ( z * px );
	*y = 240.0f - DotProduct( trans, cg.refdef.viewaxis[2] ) * 240.0f / ( z * py );

	if ( *x < -32.0f || *x > 672.0f || *y < -32.0f || *y > 512.0f ) {
		return qfalse;
	}
	return qtrue;
}

/*
=================
CG_DrawDamagePlums
=================
*/
void CG_DrawDamagePlums( void ) {
	damagePlum_t	*p;
	vec3_t			org;
	vec4_t			color;
	float			x, y, frac, scale, alpha;
	int				i, len, cw, ch;
	char			text[16];

	if ( !cg_damageNumbers.integer ) {
		return;
	}

	scale = cg_damageNumbersScale.value;
	if ( scale < 0.1f ) {
		scale = 0.1f;
	}
	// 2026-10-09: 字号减半 —— 基础字号改取 BIGCHAR(16px) 的一半（8px），
	// 仍可用 cg_damageNumbersScale 在此基础上继续放大/缩小。
	cw = (int)( BIGCHAR_WIDTH  * DAMAGE_PLUM_FONT_SCALE * scale + 0.5f );
	ch = (int)( BIGCHAR_HEIGHT * DAMAGE_PLUM_FONT_SCALE * scale + 0.5f );
	if ( cw < 1 ) {
		cw = 1;
	}
	if ( ch < 1 ) {
		ch = 1;
	}

	for ( i = 0; i < MAX_DAMAGE_PLUMS; i++ ) {
		p = &cg_damagePlums[i];
		if ( !p->active ) {
			continue;
		}

		frac = ( cg.time - p->startTime ) / (float)DAMAGE_PLUM_TIME;
		if ( frac >= 1.0f ) {
			p->active = qfalse;
			continue;
		}
		if ( frac < 0.0f ) {
			frac = 0.0f;
		}

		// 敌人还在快照里就跟着他走，否则用记录下来的位置
		if ( p->entnum >= 0 && p->entnum < MAX_GENTITIES
			&& cg_entities[p->entnum].currentValid ) {
			VectorCopy( cg_entities[p->entnum].lerpOrigin, org );
		} else {
			VectorCopy( p->origin, org );
		}
		org[2] += DAMAGE_PLUM_HEIGHT + DAMAGE_PLUM_RISE * frac;

		if ( !CG_WorldToScreen( org, &x, &y ) ) {
			continue;
		}

		// 伤害越大颜色越"重"，数字随存活时间淡出
		alpha = 1.0f - frac * frac;
		if ( p->value >= 100 ) {
			color[0] = 1.00f; color[1] = 0.30f; color[2] = 0.10f;
		} else if ( p->value >= 50 ) {
			color[0] = 1.00f; color[1] = 0.60f; color[2] = 0.10f;
		} else {
			color[0] = 1.00f; color[1] = 0.90f; color[2] = 0.25f;
		}
		color[3] = alpha;

		Com_sprintf( text, sizeof( text ), "%i", p->value );
		len = strlen( text );

		CG_DrawStringExt( (int)x - ( len * cw ) / 2, (int)y - ch / 2,
			text, color, qtrue, qtrue, cw, ch, 0 );
	}
}
