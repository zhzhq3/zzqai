# zzqAI — 给 Quake 3 机器人补上"该不该打"的那一层

> 基于 **[brainworks](https://www.quake3world.com/)**（Ted Vessenes, 2002–2007）的 Quake 3 机器人 AI 增强模组。
> brainworks 已经是公开可得的 Q3 机器人 AI 里最完整的一个；本项目**不重写它**，
> 只在它空白的"决斗决策层"上补四个模块。

[中文](#-这是什么) · [English](#english)

---

## 这是什么

Quake 3 的机器人 AI 普遍存在一个共同的空白：**感知、瞄准、走位都做得不错，但不会"取舍"**。

brainworks 已经具备：

| 能力 | 实现位置 |
|---|---|
| 视觉 + **听觉**感知（脚步 512、跳跃/开枪 1024、子弹打墙 256，按声量给半径） | `ai_scan.c` / `ai_aware.c` |
| 射击提前量、按武器与距离选武器 | `ai_attack.c` / `ai_weapon.c` |
| 伤害比计算（`BotEnemyDamageRatio`）、闪避（`MM_DODGE`）、strafe jump、rocket jump | `ai_move.c` / `ai_dodge.c` |
| 物品簇 + 重生计时、资源预测 | `ai_item.c` / `ai_pickup.c` |
| 敌方血量估计（靠痛叫声）、`aware_record[]` 带衰减记忆 | `ai_self.c` / `ai_scan.c` |

但它**没有**：全工程搜不到任何 `duel` / 1v1 相关逻辑，机器人一旦锁定敌人就会一直贴着打 ——
哪怕自己只剩 10 点血、对面满血满甲。也不会提前去占物品刷新点，更不会在丢失敌人后
按对手的活动习惯去搜。

zzqAI 补上六件事（**M1–M6**）：

| 模块 | 做什么 | 新增/改动 |
|---|---|---|
| **M1 对手档案** | 给每个玩家建长期行为画像（区域热度、蹲守时长、常用武器、死亡、最后被察觉位置），丢失敌人后**朝该对手惯常出没的区域搜索** | `ai_profile.c/.h`（新）；挂 `ai_main.c` / `ai_aware.c` / `ai_aim.c` |
| **M2 物品时钟** | 允许机器人为高价值物品**提前到位等待**（原版有一条 `pickup_time < soonest_respawn` 就直接放弃，所以永远不会提前占位） | `ai_pickup.c` |
| **M3 决斗策略机** | 每帧算出态势 **压上 / 对枪 / 脱离**；劣势时不再送人头，而是去最近的血包/护甲 | `ai_duel.c/.h`（新）；挂 `ai_maingoal.c` / `ai_action.c` |
| **M4 战斗走位** | 站在理想交火距离时**绕圈横移**（circle strafe）而不是原地站桩；劣势时保持面向敌人向外后撤（边退边打） | `ai_move.c`（新增 `MOVE_CIRCLE`） |
| **M5 连续自适应难度** | skill 不再固定 1~5。按每个机器人这一局的得分表现连续地抬高或压低它的有效 skill —— 打得好的被压、打得差的被抬，队伍自动趋衡 | `ai_adaptive.c/.h`（新）；挂 `ai_main.c` / `ai_vars.c` |
| **M6 团队情报共享** | 团队模式里互相通报敌人位置；队友没看到人时会去情报点搜人。队内按进攻性自动分工（突破 / 策应 / 守点），角色决定响应半径 | `ai_share.c/.h`（新）；挂 `ai_aware.c` / `ai_maingoal.c` |

另外附带两个独立交付物：

- **zzqBOT** —— 一个 bot 模式（纯运行时角色数据，不碰 AI 逻辑）。
- **伤害数字** —— 打中敌人时对方身上冒伤害值（CPMA 风格），由自编译的 `cgame.qvm` 提供。

> 想只关掉 zzqAI 而保留 zzqBOT / 伤害数字：把六个模块的 cvar 设 0 即可
> （`bot_profile` / `bot_item_wait` / `bot_duel` / `bot_duel_move` / `bot_adaptive` / `bot_share`）。

---

## 设计红线

> **"更强"强在决策，不强在枪法。**

不改瞄准精度、不改伤害数值、不降低命中误差。如果一个模组让人恶心，那通常不是因为它聪明，
而是因为它"枪法超人类"。本项目只改**站位与取舍**：

- 优势比用"可承受伤害"折算（`EntityHealth` 已把护甲按 2/3 折价、战衣减半计入），跨装备可比。
- 敌方护甲**按 0 计** —— 宁可低估自己的优势，也不打破信息不完全。
- M1 的热力图只统计停留时长，**不读取隐身玩家的实时坐标**，不会变成透视。

---

## 快速开始

### 一、直接玩（Windows）

```
1. 把 release/brainworks.pk3 放进 Quake 3 的 brainworks/ 目录（覆盖原文件，先备份）
2. 启动，控制台执行：  exec zzqbot.cfg
```

或在命令行：

```
quake3e.x64.exe +set fs_game brainworks +devmap q3dm4 +exec zzqbot.cfg
```

> 必须用 `devmap` 而不是 `map`：`bot_*` 系列 cvar 带 `CVAR_CHEAT`，
> 普通开局改不动它们。

### 二、自己编译

```bash
# 需要 Windows + Git Bash（仓库自带 lcc/q3asm 工具链）
bash build/build_qagame.sh
# 产物： code/game/vm/qagame.qvm
```

重新打包 pk3：

```bash
bash tools/pack_release.sh release/brainworks.pk3 release/brainworks-new.pk3 Z:/path/to/zzqbot-assets
```

cgame（伤害数字）见 [`cgame/README.md`](cgame/README.md)。

### 三、验证

```bash
# 打开诊断，跑一局 bot vs bot
quake3e.x64.exe +set fs_game brainworks +set logfile 1 +devmap q3dm4 +exec m3test.cfg
```

关掉全部功能跑对照组：`+exec m34control.cfg`。

---

## cvar 一览

| cvar | 默认 | 说明 |
|---|---|---|
| `bot_profile` | 1 | M1 对手档案总开关（丢失敌人后按对手习惯区域搜索） |
| `bot_profile_debug` | 0 | 置 1 打印一次全部玩家画像（自动清零） |
| `bot_item_wait` | 2.0 | M2 提前占位秒数；**0 = 关闭，退回原版行为** |
| `bot_item_wait_diag` | 0 | 打印"提前占位"诊断 |
| `bot_duel` | 1 | M3 决斗态势判定 + 劣势补血甲 |
| `bot_duel_move` | 1 | M4 战斗走位（绕圈横移 + 劣势后撤） |
| `bot_duel_debug` | 0 | 实时打印态势变化（较刷屏） |
| `bot_duel_report` | 0 | 置 1 打印一次全部机器人态势汇总（自动清零） |
| `bot_adaptive` | 1 | M5 连续自适应难度总开关 |
| `bot_adaptive_rate` | 0.30 | 每个评估周期最多移动的 skill 量 |
| `bot_adaptive_range` | 1.5 | skill 偏移的绝对值上限 |
| `bot_adaptive_min` / `_max` | 1.0 / 5.0 | 有效 skill 的上下限 |
| `bot_adaptive_interval` | 6.0 | 评估周期（秒） |
| `bot_adaptive_debug` / `_report` | 0 | 实时调整日志 / 一次汇总 |
| `bot_share` | 1 | M6 团队情报共享总开关（**仅团队模式生效**） |
| `bot_share_lifetime` | 8.0 | 一条情报的有效期（秒） |
| `bot_share_range` | 2500 | 情报响应距离上限 |
| `bot_share_debug` / `_report` | 0 | 实时情报日志 / 一次汇总 |

全部为 0 时行为**完全等同 brainworks 原版**（已在对照实验中验证）。

---

## 实测证据

`q3dm4`，3 个机器人，技能 4，约 2 分钟：

**态势变化（`bot_duel_debug 1`，摘录）**

```
zzqAI M3: AI-One   态势 未交战 -> 对枪 (优势比 0.99, 自身 124, 敌方 125)
zzqAI M3: AI-Three 态势 对枪   -> 压上 (优势比 1.57, 自身 104, 敌方 66)
zzqAI M3: AI-Two   态势 压上   -> 脱离 (优势比 0.46, 自身 56,  敌方 121)
zzqAI M3: AI-One   态势 对枪   -> 脱离 (优势比 0.21, 自身 23,  敌方 106)
zzqAI M3: AI-Three 态势 未交战 -> 脱离 (优势比 0.21, 自身 26,  敌方 120)
```

**汇总报告（`bot_duel_report 1`）**

```
===== zzqAI M3/M4 决斗态势报告 =====
机器人    态势   优势比 压上   脱离   绕圈换向
AI-One       压上          5.40 19       15       7
AI-Two       压上          2.41 12       7        4
AI-Three     压上          0.41 15       13       6
===================================
```

- 146 行态势变化，压上/脱离/绕圈换向全部非零 —— M3 与 M4 都确实在跑。
- **对照组**（`bot_duel 0` + `bot_duel_move 0`）：态势行 0 行、所有计数 0、绕圈 0，
  但击杀照常发生（说明只是新增逻辑被关掉，原版 AI 完好）。
- 两次运行 **0 报错**。

完整日志：[`docs/evidence/qconsole-m34.log`](docs/evidence/qconsole-m34.log)、
[`docs/05-验证记录.md`](docs/05-验证记录.md)。

### M5 难度伺服（`q3dm4`，4 个机器人，两组不同初始 skill）

`Low*` 初始 skill 2，`High*` 初始 skill 4。跑约 2 分钟后的汇总：

```
机器人        原始   偏移   有效   得分   同场均分
zzqBOT-Low1   2.00  -0.17   1.82    9     7.0
zzqBOT-Low2   2.00   0.65   2.65    3     8.5     ← 得分最低，被抬起来
zzqBOT-High1  4.00  -0.59   3.40   14     5.7     ← 得分最高，被压下去
zzqBOT-High2  4.00  -0.36   3.63   11     6.5
```

两组的 skill 差距从 **2.00** 收敛到 **0.75** —— 伺服按**个体表现**生效，不是按初始档位。
140 条调整日志，36 轮评估，0 报错。

### M6 团队情报（`q3dm6`，TDM，红蓝各 2 个机器人）

```
机器人        队伍 角色 收到情报
zzqBOT-Red1      红    突破 65
zzqBOT-Red2      红    守点 35
zzqBOT-Blue1     蓝    突破 84
zzqBOT-Blue2     蓝    守点 106
红队情报板：2 条有效（有效期 10.0s，响应半径 4000）
蓝队情报板：2 条有效（有效期 10.0s，响应半径 4000）
累计情报上报 6323 条
```

```
zzqAI M6: zzqBOT-Red1（突破）收到 zzqBOT-Red2 的情报 -> 距 1894，情报龄 0.0s
```

### 对照组（M5 + M6 全关）

偏移全 0、有效 skill = 原始值、角色全"无"、情报板 0 条、上报 0 条、**0 报错**，
击杀照常发生 ⇒ 关掉后完全退回原版。

详见 [`docs/06-M5-M6-实现说明.md`](docs/06-M5-M6-实现说明.md)。

---

---

## 发布到 GitHub

本目录**已经是一个初始化好的 git 仓库**（含首个提交 `zzqAI 1.0.0`）：

```bash
cd <本目录>

# 1) 新建一个空仓库（GitHub 网页端，不要勾选 README/License，避免冲突）
#    然后：
git remote add origin git@github.com:<你的用户名>/zzqAI.git
git branch -M main
git push -u origin main
```

推送前建议确认：

```bash
git status          # 应为 clean
git log --oneline   # 应看到 zzqAI 1.0.0
du -sh .git         # 约 7 MB 以内
```

**关于大文件**：`release/brainworks.pk3`（3.4 MB）、`release/*.qvm`、
`upstream/*.pk3` 都是二进制，GitHub 单文件 100 MB 以内都没问题，不需要 LFS。

> ⚠️ **授权**：本仓库是 GPL-2.0 衍生作品，**必须**保持 GPL-2.0 并保留
> `NOTICE.md` 里的上游归属。不要把它改成闭源或更严格的许可证。
> 另外**不要**把 War 模型 / 音效（第三方美术素材）提交进来 ——
> 本仓库刻意只含角色**定义文件**，素材由使用者自备。

---

## 仓库结构```
code/game/          完整的 brainworks 游戏源码（含 zzqAI 全部改动，可直接编译）
bin/                lcc / q3asm 工具链（Windows 版，让仓库自包含）
ui/                 game 代码 include 需要的菜单定义
build/              编译脚本
cgame/              伤害数字功能的 overlay 文件与说明
botfiles/bots/      zzqBOT 角色定义（引擎扫 botfiles/ 找角色文件）
scripts/zzqbot.bot  zzqBOT 索引条目（引擎扫 scripts/*.bot 发现机器人）
config/             zzqbot.cfg 与 M1–M6 验证/对照配置
release/            已编译好的 qagame.qvm / cgame.qvm 与组装好的 brainworks.pk3
docs/               设计文档、实现说明、验证记录
upstream/           上游原版 pk3 与许可证，便于对比与回退
tools/pack_release.sh  把产物重新打进 pk3
```

---

## 文档

| 文件 | 内容 |
|---|---|
| [`docs/01-三方对比与第四代设计.md`](docs/01-三方对比与第四代设计.md) | brainworks / Quake3AI / Quake3bot 三方横向对比，"第四代"方案 |
| [`docs/02-M1-M2-实现说明.md`](docs/02-M1-M2-实现说明.md) | M1 对手档案 + M2 物品时钟的实现细节 |
| [`docs/03-M3-M4-实现说明.md`](docs/03-M3-M4-实现说明.md) | M3 决斗策略机 + M4 战斗走位的实现细节 |
| [`docs/06-M5-M6-实现说明.md`](docs/06-M5-M6-实现说明.md) | M5 连续自适应难度 + M6 团队情报共享的实现细节 |
| [`docs/04-构建与安装.md`](docs/04-构建与安装.md) | 从零构建、打包、踩坑记录 |
| [`docs/05-验证记录.md`](docs/05-验证记录.md) | 实跑验证数据与对照组结果 |
| [`CHANGELOG.md`](CHANGELOG.md) | 版本变更 |
| [`NOTICE.md`](NOTICE.md) | 第三方组件与授权归属 |

---

## 踩坑备忘（给后来者）

1. **Q3 QVM 新增 cvar 必须同时做两件事**：在 `ai_vars.c` 写定义 + 注册，**并且**在
   `LevelUpdateVariables()` 里补 `trap_Cvar_Update(&var)`。少了后半句，VM 里读到的
   永远是注册时的默认值 —— 症状是 `cvarlist` 显示你设的值，但代码毫无反应。
2. **gamedir 的 pk3 优先于松散文件**，所以改了 `qagame.qvm` 必须重打包 pk3，
   放在 `brainworks/vm/` 下是没用的。
3. **`sv_cheats` 在某些发行版被 ROM 锁**，`give all` 之类会失败；用 `devmap` 开图。
4. 机器人进场很晚，脚本里 `wait` 要给够（本仓库测试配置用 1500 帧起）。
5. `SetRespawn()` 之类的函数会被编译器内联，**不要用"有没有 call 指令"判断改动是否生效**，
   要看它引用的常量。
6. **新目标在 `goal_sieve` 里的顺序决定它到底会不会被执行。** 原版目标检查是"先命中者胜"，
   所以一条排在 `BotGoalItemChoice`（"去重要物品点"）之后的新目标，实战中几乎永远轮不到 ——
   M6 的情报目标最初就踩了这个坑，整局"收到情报"计数为 0，把它提前到物品检查之前
   立刻就有了 40 条日志。
7. **M6 只在团队模式生效**（`game_style & GS_TEAM`）。用 FFA 图跑 TDM 时必须显式
   `+set g_gametype 3`，否则 `BotTeam()` 返回 `TEAM_FREE`，所有情报入口直接返回，看起来像坏了。
8. 想观察"自适应难度有没有真的在动"，别只看 `cvarlist` —— 报告里给出的是
   **原始 skill / 偏移 / 有效 skill** 三列，只看 cvar 是看不出偏移的。

---

## 授权

GPL-2.0（Quake 3 引擎与游戏代码的授权要求）。详见 [`LICENSE`](LICENSE) 与 [`NOTICE.md`](NOTICE.md)。

---

## English

**zzqAI** is an AI enhancement mod for **Quake 3** built on top of
[brainworks](https://www.quake3world.com/) (Ted Vessenes, 2002–2007).

brainworks already has the best perception / aiming / navigation of any publicly
available Q3 bot AI. What it lacks is a **decision layer**: there is no `duel`
logic anywhere in the codebase, so a bot that acquires an enemy will keep
fighting even at 10 HP against a fully stacked opponent. It also never arrives
early to camp an item spawn, and when it loses an enemy it searches in random
directions.

This project adds four modules:

- **M1 — Opponent profile.** Per-player, time-decayed behaviour model (region
  heat, camping time, favourite weapon, deaths, last seen position). When the
  bot loses an enemy, it searches where *that* opponent usually goes.
- **M2 — Item clock.** Lets bots arrive at and wait for valuable item spawns.
  Vanilla brainworks skips any item that has not respawned yet, so it can never
  hold a position.
- **M3 — Duel stance.** Every frame the bot computes *press / trade / disengage*
  from a damage-equivalent advantage ratio. When behind, it stops feeding and
  goes for the nearest health/armor instead of chasing.
- **M4 — Combat movement.** Circle-strafes around the enemy at the ideal
  engagement range instead of standing still, and backs off while keeping the
  enemy in view when disengaging.

**Red line: stronger decisions, not superhuman aim.** Aim accuracy, damage
numbers and hit-error models are untouched. Enemy armor is assumed to be zero
(deliberately underestimating your own advantage, since armor is unobservable).
M1 tracks dwell time only — it never reads the real-time position of invisible
players, so it is not a wallhack.

Bundled extras: **zzqBOT** (a bot "mode" that is pure runtime character data) and
**damage numbers** (CPMA-style floating damage values, implemented in a
self-compiled `cgame.qvm`).

Quick start:

```
quake3e.x64.exe +set fs_game brainworks +devmap q3dm4 +exec zzqbot.cfg
```

Build:

```bash
bash build/build_qagame.sh          # -> code/game/vm/qagame.qvm
```

All `bot_*` cvars default to on; setting `bot_profile 0`, `bot_item_wait 0`,
`bot_duel 0` and `bot_duel_move 0` restores **exactly** vanilla brainworks
behaviour (verified with a control run).

Licensed under **GPL-2.0**.
