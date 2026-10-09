# 变更记录

## [1.2.0] — 2026-10-09

**独立发行版**：不再依附于已安装的 brainworks 目录，而是一个自带 gamedir 的
"解压即用"模组包 —— 放进游戏根目录，在 MODS 菜单里选中即可开玩，**不需要**任何
启动参数或控制台命令。

### 新增 — 独立 mod 包（`packaging/` + `tools/pack_mod.sh`）

- 新增 `tools/pack_mod.sh`：一步产出
  - `release/zzqai.pk3` —— mod 本体（`vm/` + `botfiles/` + `scripts/` + `models/` + `sound/`，51 个条目）
  - `release/zzqai-<版本>-standalone.zip` —— 完整独立包，解压出来就是 `<Quake3>/zzqai/`
- 新增 `packaging/description.txt`：MODS 菜单里显示的模组说明（纯 ASCII，带 Q3 颜色码）。
- 新增 `packaging/README.txt`：中文安装/操作说明，进包内作为 `zzqai/README.txt`。
- 装好后 `<Quake3>/zzqai/` 只含三个文件：`zzqai.pk3`、`description.txt`、`README.txt`。
  卸载 = 删掉这个文件夹，游戏本体零改动。

### 变更 — 服务器 gamename 改为 `zzqai`

- `code/game/g_local.h` 的 `GAMEVERSION` 由 `"brainworks"` 改为 `"zzqai"`。
  这是服务器广播的 `gamename`，独立取值可避免与已安装的 brainworks 互相串线。
- **副作用**：本模组不再能与原版 brainworks 服务器互通（同一份 QVM 的本地服务器不受影响）。

### 变更 — 机器人由玩家自己在菜单里添加

- `bot_minplayers` 默认值保持 `0`（不自动补人）。加机器人走游戏内
  **ESC → ADD BOTS** 菜单：菜单自带 **BOT SKILL** 五档难度
  （I Can Win / Bring It On / Hurt Me Plenty / Hardcore / Nightmare，对应 skill 1–5），
  勾选机器人后 FIGHT，底层就是 `addbot "<名字>" <skill>`，与 `Svcmd_AddBot_f`
  的参数解析完全兼容。
- 想要自动补人仍可 `set bot_minplayers N`。

### 说明

- `tools/pack_release.sh`（覆盖式打进 brainworks.pk3 的旧流程）保留，供仍在使用
  覆盖安装的用户回退，但不再是默认推荐路径。

### 验证

- 解压到 `D:/Quake 3 Arena-vulkan/` 后，`+set fs_game zzqai` 直接开图：
  QVM 正常加载、botlib 初始化、103 bots parsed、180 arenas parsed、0 报错。
- 不加任何 bot 相关参数时，全场只有玩家自己 —— 确认自动补人已彻底关闭。
- 手动 `addbot <名字> <难度>` 加机器人正常进场，难度参数生效。

## [1.1.0] — 2026-10-09

在 1.0.0 的四个模块之上，补齐 M5、M6，并把整个项目推进到"可发布"状态
（源码 / 构建脚本 / 发布产物 / 文档 / 验证证据齐备）。

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
