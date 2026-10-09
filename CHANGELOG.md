# 变更记录

## [1.0.0] — 2026-10-09

首个可运行版本。基于 brainworks 1.0.1，新增四个决策层模块 + 两个附带交付物。

### 新增 — M1 对手档案（`ai_profile.c` / `ai_profile.h`）

- `player_profile_t`：按 client 索引，记录区域热度（随时间衰减）、蹲守/移动时间、
  常用武器时间、死亡次数、存活时间、最后被察觉时刻与坐标。
- `LevelProfileUpdate(elapsed)` 每个服务器帧更新；`ProfileEnemySeen()` 在敌人被
  察觉（看到或听到）时记录位置。
- `ProfilePrediction(bs, location)` 返回该机器人最可能碰到敌人的"热区"。
- 挂载点：`ai_main.c`（每帧更新 + 报告触发）、`ai_aware.c`（`BotAwareTrackEntity`
  记录位置）、`ai_aim.c`（`BotSelectSearch` 由"16 个方向随机挑"改为朝热区挑）。
- cvar：`bot_profile`（默认 1）、`bot_profile_debug`（默认 0）。

### 新增 — M2 物品时钟（改 `ai_pickup.c`）

- 原版逻辑 `if (pickup_time < this_option->soonest_respawn) skip` 让机器人永远
  不会提前到位等刷新。改为放宽 `bot_item_wait` 秒，并让资源预测按真实刷新时刻
  计入，机器人会主动提前占位。
- cvar：`bot_item_wait`（默认 2.0，**0 = 关闭**）、`bot_item_wait_diag`（默认 0）。

### 新增 — M3 决斗策略机（`ai_duel.c` / `ai_duel.h`）

- `BotDuelUpdate(bs)` 每帧算出态势：
  - `DUEL_STANCE_PRESS` 压上 —— 我方有 Quad 对面没有 / 对面残血 / 优势比 ≥ 1.45 且自身
    可承受伤害 ≥ 85
  - `DUEL_STANCE_TRADE` 对枪
  - `DUEL_STANCE_DISENGAGE` 脱离 —— 自身可承受伤害 ≤ 55 或优势比 < 0.80
- 优势比 = `EntityHealth(我方) / BotEnemyHealth(敌方)`（按可承受伤害折算，敌方护甲按 0 计）。
- 态势切换有迟滞（`DUEL_STANCE_COMMIT` 0.35 s），但**进入脱离立即生效**。
- 新增目标 `GOAL_DUEL_REGROUP` 与 `BotGoalDuelChoice()`：脱离时去最近的
  血包/护甲，**且该物品到敌人的距离必须大于到自己**（不朝枪口跑）。
- 新增动作 `BotActionDuel`，在 `BotActionGoal` 之前、`BotActionMoveSetup` 之后执行。
- cvar：`bot_duel`（默认 1）、`bot_duel_debug`（默认 0）、`bot_duel_report`（默认 0）。

### 新增 — M4 战斗走位（改 `ai_move.c`）

- 新增移动方向 `MOVE_CIRCLE`（`ai_main.h`，值 `0x0040`）。
- `BotMoveEnemyDir()` 在"处于理想交火距离"时，若 `BotDuelShouldCircle()` 成立则返回
  `MOVE_CIRCLE` 而非 `MOVE_STILL`；`BotMoveEnemy()` 用垂直于"敌人→我"的水平向量
  横移，每 `DUEL_CIRCLE_SWITCH`（1.4 s）随机换向。
- `BotMoveEnemy()` 开头：`BotDuelShouldRetreat()` 成立时按 `BotDuelRetreatDir()`
  向外后撤（带 ≤35° 横向偏角），瞄准不受影响，**边退边打**。
- `BotDuelShouldCircle()` 条件：均势或优势、有瞄准敌人、站在地面上、距离 72–768。
- cvar：`bot_duel_move`（默认 1）。

### 新增 — zzqBOT 机器人模式

- 纯运行时角色数据，**不修改任何 AI 逻辑**：`botfiles/bots/zzqbot_{c,w,i,t}.c`、
  `botfiles/bots/zzqbot/fw_weap.c`、`scripts/zzqbot.bot`（索引必须放 `scripts/`，
  角色文件必须放 `botfiles/`，`aifile` 路径相对 `botfiles/`）。
- 技能 5，使用 War 角色皮肤与音效（素材需自备，见 `NOTICE.md`）。

### 新增 — 伤害数字（cgame）

- 服务端 `G_Damage` 用 `trap_SendServerCommand(attacker, va("dmg %i %i", targ, take))`
  上报；cgame 新增 `cg_damagenums.c` 解析并把伤害值投影到世界坐标绘制成浮动数字。
- 字号 `DAMAGE_PLUM_FONT_SCALE = 0.5`（16 px → 8 px）。
- cvar：`cg_damageNumbers`（默认 1）、`cg_damageNumbersScale`（默认 1）、
  `cg_damageNumbersDebug`（默认 0）。
- 需要自编译 `cgame.qvm`，见 `cgame/README.md`。

### 验证

- `q3dm4`、3 bot、技能 4、约 2 分钟：146 行态势变化，压上/脱离/绕圈换向计数全部非零，0 报错。
- 对照组（`bot_duel 0` + `bot_duel_move 0`）：态势 0 行、计数全 0、绕圈 0，击杀照常发生。
- 编译 `0 total errors`。
