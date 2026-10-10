# 07 · CPMA 对齐：机器人统一亮绿 + 切枪速度

> 1.3 版做的两件"看得见、摸得着"的事。都与 AI 逻辑无关 —— 纯粹是把 CPMA
> （Challenge ProMode Arena）最显眼的两个特征搬过来。
>
> ⚠️ 本文档记录的是**最终可行**的做法。中间走过一次弯路（在服务端给机器人写
> `color1/color2`），第四节把那条路为什么不成立写清楚了，避免以后重复踩。

---

## 一、为什么挑 CPMA

CPMA 是 Quake 3 最成功的竞技 mod（CPL / ESWC 的官方比赛 mod）。它有两个一眼就能
认出来的特征：

1. **敌人（含机器人）一律是明亮的绿色**，而且看起来都一样，不受对方本来选什么模型
   影响。
2. **切枪几乎没有延迟。** 中文维基的概括：*"CPMA 取消了原有游戏的 450 毫秒的武器
   切换延迟时间，从而可以让玩家在短时间内使用不同种类的几种武器击出威力很大的
   组合伤害。"*

---

## 二、绿色：CPMA 是怎么做的

CPMA 走的是**客户端**路线，靠两个 cvar：

| cvar | 作用 |
|---|---|
| `cg_enemyModel` | 敌人的模型/皮肤。填 `keel/pm`、`sarge/pm` 这种"`/pm` 结尾"的值 |
| `cg_enemyColors` | 染色码。CPMA 默认 `g2222` |

关键在那个 **`pm`** —— 它是 **ProMode brightskin**（"亮皮"）。CPMA 文档原话：
*"PM model highly recommended"*、*"fullbright custom colour enemies rule!"*。

### `/pm` 皮肤到底做了什么

以 `baseq3/pak8a.pk3` 里的 `models/players/anarki/head_pm.skin` 为例：

```
h_head,models/players/anarki/pm
```

它把头部表面指向一个 **shader**（不是 tga）。该 shader 定义在
`scripts/players.shader`：

```glsl
models/players/anarki/pm
{
	nopicmip
	{
		map models/players/anarki/color.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity          // ★ 用"实体颜色"渲染
	}
	{
		map models/players/anarki/color.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc LT128
		rgbGen identity
	}
}
```

`rgbGen entity` 意味着这一层的颜色取自实体的 `shaderRGBA`；而 cgame 里
（`cg_players.c` 的 `CG_Player()`）正好是这么赋值的：

```c
legs.shaderRGBA[0] = ci->legsColor[0] * 255;    // 腿
torso.shaderRGBA[0] = ci->bodyColor[0] * 255;   // 上身
```

`color.tga` 本身是一张近似纯白的蒙版，于是 **最终画出来就是 `bodyColor`/`legsColor`
这个纯色** —— 想让它绿就绿、想让它红就红。这就是 CPMA"亮皮敌人"的全部原理。

### 这条链在 cgame 里已经现成了

本模组用的 cgame（better-gibs 那棵树的 `code/cgame`）**完整保留了 CPMA 这套逻辑**：

```c
cg_players.c:6     #define PM_SKIN "pm"
cg_players.c:995   pm_model = ( Q_stricmp( cg_enemyModel.string, PM_SKIN ) == 0 );
cg_players.c:1026  CG_SetColorInfo( colors, newInfo ); newInfo->coloredSkin = qtrue;
```

FFA（你实际在玩的模式）走的是这一段：

```c
} else { // not team game
    if ( pm_model && myClientNum != clientNum && cgs.gametype != GT_SINGLE_PLAYER ) {
        Q_strncpyz( modelName, infomodel, ... );      // 各 bot 自己的模型
        Q_strncpyz( skinName, PM_SKIN, ... );         // 皮肤强制成 "pm"
        if ( !CG_IsKnownModel( modelName ) )
            Q_strncpyz( modelName, "sarge", ... );    // 不认识的模型退回 sarge
        if ( setColor ) {
            colors = CG_GetTeamColors( cg_enemyColors.string, newInfo->team );
            CG_SetColorInfo( colors, newInfo );
            newInfo->coloredSkin = qtrue;
        }
    }
```

所以**一行渲染代码都不用改**，只要把两个 cvar 默认值设对。

---

## 三、具体实现（绿色）

### 1. 改默认值 —— `cgame/cg_cvar.h`

```c
CG_CVAR( cg_enemyModel,  "cg_enemyModel",  "pm",   CVAR_ARCHIVE, NULL )
CG_CVAR( cg_enemyColors, "cg_enemyColors", "2222", CVAR_ARCHIVE, NULL )
```

`cg_enemyColors` 是 5 个字符，按 **头 / 身 / 腿 / color1 / color2** 解析
（`CG_SetColorInfo()`）。全填 `2` 即全身纯绿。Q3 的颜色码是 3 位掩码：

| 值 | 颜色 |
|---|---|
| 1 | 红 |
| 2 | **绿** |
| 4 | 蓝 |
| 3 | 黄 | 5 | 品红 | 6 | 青 | 7 | 白 |

### 2. 兜底补设 —— `cgame/cg_main.c` 的 `CG_RegisterCvars()` 末尾

这两项是 `CVAR_ARCHIVE`，老 `q3config.cfg` 里可能存着 `seta cg_enemyModel ""`
（**你这台机器上正是如此**），存档值会把注册默认值盖掉。所以注册完再补一次：

```c
	if ( !cg_enemyModel.string[0] ) {
		trap_Cvar_Set( "cg_enemyModel", "pm" );
	}
	if ( !cg_enemyColors.string[0] ) {
		trap_Cvar_Set( "cg_enemyColors", "2222" );
	}

	CG_Printf( "^2zzqAI^7 cgame 1.3 loaded | enemyModel=^3%s^7 enemyColors=^3%s^7\n",
		cg_enemyModel.string, cg_enemyColors.string );
```

玩家若自己设过非空值，则完全尊重、不覆盖。最后那行横幅用来确认跑的是本模组的
cgame，而不是 baseq3 里的那一份 —— 排查问题时非常省事。

### 3. 回退

```
set cg_enemyModel ""
set cg_enemyColors ""
```

---

## 四、为什么"服务端写 color1/color2"是错的（走过的弯路）

第一次尝试是**服务端**方案：在 `g_bot.c` 的 `G_AddBot()` 里给机器人写
`model = "War/Default"`、`color1 = color2 = "2"`，并加了个 `bot_greenbots` 开关。

**实测不绿，原因有两条：**

1. **Q3 的 `color1/color2` 是对"贴图"做染色，不是把模型涂成纯色。**
   普通皮肤的贴图是五颜六色的，乘上绿色只会得到"绿了吧唧的原色"。要真正变亮绿，
   必须换成 `color.tga` 这种**白蒙版 + `rgbGen entity`** 的 brightskin。
   而 `War/Default.tga` 是一张**近黑**的蒙版（采样平均 RGB ≈ (1,1,1)），
   乘什么颜色都是暗的。
2. **CPMA 的绿本来就是客户端渲染层的效果**，服务端给机器人写外观只是"改它的默认
   长相"，跟"一律显示成亮绿"是两回事。

结论：`bot_greenbots` 这个 cvar 已从 `g_bot.c` 移除，该文件回到 1.2.0 原状。
外观完全交给客户端的 `cg_enemyModel` / `cg_enemyColors`。

> **依赖说明**：`/pm` 皮肤与 `color.tga` 由 baseq3 里的 **better-gibs mod pak
> （`pak8a.pk3`）** 提供。本模组的 cgame 源码就是取自那个 mod，两者是一套。
> 若 baseq3 里没有这些资源，机器人会退回各自的默认皮肤（不会变绿，但游戏一切正常）。

---

## 五、切枪速度

`code/game/bg_pmove.c`：

```c
static void PM_BeginWeaponChange( int weapon ) {      // 放下旧枪
	...
	pm->ps->weaponstate = WEAPON_DROPPING;
	// 只有从出生武器（手锯 / 机枪）切出时才保留 200ms
	if ( pm->ps->weapon == WP_GAUNTLET || pm->ps->weapon == WP_MACHINEGUN ) {
		pm->ps->weaponTime += 200;
	}
	PM_StartTorsoAnim( TORSO_DROP );
}

static void PM_FinishWeaponChange( void ) {           // 举起新枪
	...
	pm->ps->weaponTime += ZZQ_WEAPON_RAISE_TIME;      // 原版 250 → 现在 0
}
```

- 原版一次切枪 = **200（放下）+ 250（举起）= 450 ms**
- 现在 = **0 + 0 ≈ 1~2 帧**（`PM_Weapon()` 的状态机是
  `冲枪 → DROPPING → RAISING → READY`，每步各占一帧）

依据（多个独立来源一致）：

- 中文维基：*"CPMA 取消了原有游戏的 450 毫秒的武器切换延迟时间"*。
- 英文综述：*"a key innovation is the **instant weapon switching** mechanic, which
  eliminates delays for the first weapon picked up after spawning (**0 ms**), while
  imposing a minor **200 ms** penalty only for transitions from starting weapons like
  the gauntlet or machinegun."*

即：**正常切换瞬时；只有"从出生武器切出"还留 200 ms**。这正是上面代码的形状。

想退回原版手感：把 `PM_FinishWeaponChange()` 的 `+0` 改回 `+250`，并去掉
`PM_BeginWeaponChange()` 里的出生武器判断，然后重编。

### 为什么两个 VM 都要重编

`bg_pmove.c` 位于 **bg 层（both games）**，qagame 与 cgame **各编译一份**，
cgame 拿它做本地玩家预测。两边数值不一致时，客户端先按一套时间预测、再被服务器
另一套纠正 —— 表现就是切枪时枪"弹回去"。所以 `qagame.qvm` 与 `cgame.qvm` 必须
同版本重编。`cgame/overlay/code/game/bg_pmove.c` 就是这份改过的文件。

---

## 六、验证

- 两个 VM 均编译通过：`qagame.qvm` 605,612 B、`cgame.qvm` 361,052 B。
- 独立包内 `vm/qagame.qvm`、`vm/cgame.qvm` 的 md5 与 `release/` 下的产物逐字节一致。
- 启动后控制台会打印 `zzqAI cgame 1.3 loaded | enemyModel=pm enemyColors=2222`
  —— 这行日志既是版本确认，也说明外观两项已生效。

> 未做：机器人绿皮/切枪的**录像级**对照（绿 bot 需要人眼确认观感）。这两项都只改
> 表现层与物理层的手感数值，不涉及 AI 决策，不影响 M1–M6 的验证结论。

---

## 七、cvar 与回退

| 项 | 默认 | 回退办法 |
|---|---|---|
| 机器人一律亮绿 | 开 | `set cg_enemyModel ""; set cg_enemyColors ""` |
| CPMA 瞬时切枪 | 开 | 改 `bg_pmove.c` 后重编（无 cvar 开关：它在 bg 层，qagame 与 cgame 必须一致，用 cvar 反而会引入预测不一致） |
