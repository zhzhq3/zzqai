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

### 新增 — M5 连续自适应难度（`ai_adaptive.c` / `ai_adaptive.h`）

- 原版 skill 是 1~5 的整数、开局定死、全队共用。本模块给**每个机器人单独**算一个
  skill 偏移（默认 ±1.5），按"自己得分 vs 同场其他玩家平均得分"连续调整：
  落后则抬高、领先则压低；每 `bot_adaptive_interval` 秒评估一次，
  每次最多移动 `bot_adaptive_rate`。
- **零侵入**：不改任何读取 `bs->settings.skill` 的代码，而是每帧把
  `原始 skill + 偏移` 写回该字段，于是感知半径（`ai_aware.c`）、反应阈值
  （`ai_scan.c`）、瞄准精度（`ai_self.c`）、物品预测深度（`ai_pickup.c`）等
  自动跟随难度变化。
- 写回前做自愈检测：`settings.skill` 与上一帧写入值对不上（例如换了机器人）时
  重建基准。
- 挂载点：`ai_main.c`（`BotAdaptiveUpdate` + 每帧 `BotAdaptiveApply` + 报告触发）。
- cvar：`bot_adaptive`（默认 1）、`bot_adaptive_rate`(0.30)、`bot_adaptive_range`(1.5)、
  `bot_adaptive_min`(1.0)、`bot_adaptive_max`(5.0)、`bot_adaptive_interval`(6.0)、
  `bot_adaptive_debug`(0)、`bot_adaptive_report`(0)。

### 新增 — M6 团队情报共享 + 角色化响应（`ai_share.c` / `ai_share.h`）

- 每队一块情报黑板（`MAX_TEAM_INTEL` 8 个槽位）：谁上报的、敌人在哪、什么时候。
  写入端是 `ai_aware.c`（机器人真的察觉到一个敌人时）；读取端是新增目标
  `GOAL_SHARE_PURSUE` / `BotGoalShareChoice`。
- **红线**：`reporter == bs->client` 的情报被跳过 —— 自己报的不算数，
  避免情报共享退化成透视。情报有 `bot_share_lifetime`（默认 8 秒）有效期。
- 目标顺序：劣势补血甲 → 打看得见的敌人 → **（M6）去情报点** → 去物品点 → 蹲点。
  放在"去物品点"之前是有意的：队友刚上报的位置比"碰运气"更值得跑。
- 队内**自动角色分工**（按 `CHARACTERISTIC_AGGRESSION − 0.5 × CHARACTERISTIC_CAMPER`
  排序）：进攻性最高 = 突破（响应半径 ×1.0）、中间 = 策应（×0.75）、
  最低 = 守点（×0.5）。成员变化时立即重算。
- **仅团队模式生效**（`game_style & GS_TEAM`）；FFA 下所有入口直接返回。
- cvar：`bot_share`（默认 1）、`bot_share_lifetime`(8.0)、`bot_share_range`(2500)、
  `bot_share_debug`(0)、`bot_share_report`(0)。

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
- M5（`q3dm4`、4 bot、两组不同初始 skill）：强组 skill 4.00 → 3.42、弱组 2.00 → 2.67，
  差距从 2.00 收敛到 0.75；36 轮评估，0 报错。
- M6（`q3dm6`、TDM、红蓝各 2 bot）：40 条情报接收日志，四台机器人接收计数全非零
  （65 / 35 / 84 / 106），角色分工正确，0 报错。
- 对照组（`bot_adaptive 0` + `bot_share 0`）：偏移全 0、角色全"无"、情报板 0 条、0 报错。
- 编译 `0 total errors`。
