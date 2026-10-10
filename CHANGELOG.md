# 变更记录

## [1.3.2] — 2026-10-10

**修掉"选中本 mod 就会弹 CD key 输入框"**：独立包里补一个松散文件
`zzqai/q3key`。`zzqai.pk3` 一个字节都没动。

### 根因（完整取证见 `docs/09-CDKey提示修复.md`）

- 主菜单前的弹框来自界面的 `UI_MainMenu()`：只要
  `trap_VerifyCDKey(trap_GetCDKey())` 失败就弹（`ui_menu.c:276`）。
- 本 mod 沿用 baseq3 的界面，而那是 ioquake3 系的 `q3_ui`，
  它对 `UI_HASUNIQUECDKEY` 返回 **qtrue**（`q3_ui/ui_main.c:81`，
  源码注释原文：*"change this to qfalse for mods!"*）
  ⇒ **每个 gamedir 各自要一份 CD key**。
- 于是界面取的是 `cl_cdkey[16..31]`，而这一段的唯一来源是
  `Com_AppendCDKey(fs_game)` 读 `<gamedir>/q3key`（`files.c:3433`）。
- `zzqai/q3key` 不存在 ⇒ 取到空串 ⇒ 长度 0 ⇒ 校验失败 ⇒ 弹框。
  换回 baseq3 时 `fs_game` 为空，取的是 `cl_cdkey[0..15]`（`baseq3/q3key`，
  正常装过游戏就有），所以不弹。
- 别的 mod 不弹，是因为它们大多自带 `vm/ui.qvm`，而自带界面的一般把
  `UI_HASUNIQUECDKEY` 编译成 qfalse，根本不走这条路。

### 修复

- 新增 `packaging/q3key`：前 16 字节 `2ab3cd7gahljprst`（CRLF、无 BOM），
  后接说明注释。合法依据是 `CL_CDKeyValidate(key, NULL)` 在 checksum 为
  NULL 时**只检查长度 16 + 字符落在 `237abcdghjlprstw` 内**（`cl_main.c:4704`）。
- `tools/pack_mod.sh`：
  - 独立包内增加 `zzqai/q3key`；
  - 打包时**强校验**前 16 字节合法，否则直接报错退出（避免又把"没修好的包"发出去）；
  - 默认版本号 1.3.1 → 1.3.2。
- **必须是松散文件，不能打进 pk3**：引擎读它走 `FS_BaseDir_FOpenFileRead`
  → `Sys_FOpen`，不查 pk3。打包脚本因此把它写进 zip 顶层的 `zzqai/` 而不是 pk3。

### 位置为何必然生效（实证）

- `FS_GetModDescription()` 用**同一个函数**、**同一种形状**
  （`"%s/description.txt"`）读 MODS 菜单说明（`files.c:2459`），
  而它确实能从 mod 目录读到。`q3key` 的调用形状与之同构。
- 引擎实测（专用服务器 + `+which description.txt +path`）：
  `File "description.txt" found at "D:\Quake 3 Arena-vulkan\zzqai"`，
  version 串 `Q3 1.32e win_mingw-x86_64` ⇒ 安装根即 `fs_basepath`，
  而 `fs_basepath` 正是 `FS_InitPathVars()` 里的第 4 项。
- ⚠️ `which zzqai/q3key` 会报 `File not found`，这是**预期**的 ——
  `which` 走 `FS_FOpenFileReadDir`（search path 根），与 CD key 的函数不同，
  不能拿它当反证。

### 产物

```
release/zzqai.pk3                      6,660,203 B  md5 2cfb938adab7a35002c1a7312ce82451（与 1.3.1 相同）
release/zzqai-1.3.2-standalone.zip     6,615,572 B  md5 6d346543fbda3d3845d99ac3ed0cbcc8
```

两个 VM 无改动。

### 文档

- 新增 `docs/09-CDKey提示修复.md`（触发条件 / 两段式 key 结构 / 校验算法 /
  取证链 / 排查顺序）。
- `packaging/README.txt`：安装树补 `q3key`，新增"六之二、它老是问我 CD key"。
- `release/README.md` / `packaging/description.txt`：版本号与产物信息。

## [1.3.1] — 2026-10-10

**亮皮资源随包自带**：把 CPMA ProMode brightskin 的蒙版、皮肤、图标和 shader
全部打进 `zzqai.pk3`，本模组从此**不依赖 baseq3 里的 `pak8a.pk3`**。

### 新增 — 自带亮皮资源（136 个文件）

- 新增 `tools/extract_brightskin.py`：从 `baseq3/pak8a.pk3` **原样抽出**
  - `models/players/<模型>/{head,lower,upper}_pm.skin`（69）—— mesh → `pm*` shader
  - `models/players/<模型>/color.tga`（23）+ `color_h.tga`（16）+ `color_x.tga`（4）
    —— 亮皮蒙版（alpha=255 处走 `rgbGen entity`，被 `cg_enemyColors` 染色）
  - `models/players/<模型>/icon_pm.tga`（23）—— 记分板头像，缺了皮肤会注册失败
  - `scripts/zzqai_brightskin.shader`（1）—— 44 个 shader 定义，与 pak8a 逐字一致
- 抽出的文件落在 `assets/brightskin/`（含 `README.md` 说明来源与许可）。
- `tools/pack_mod.sh` 新增第 3b 步把该目录整份打进 pk3，并且：
  - **故意不产出 `scripts/players.shader`** —— 那个名字会把别人的同名文件整份盖掉；
    换成 `scripts/zzqai_brightskin.shader`，shader 是按**名字**注册的，位置无关。
  - 亮皮条目排在素材之后，优先级最高（不会被 `q3zzqbot` 素材覆盖）。
- 效果：干净安装（只有 `pak0.pk3`）的 Q3 上也能开亮皮；装了 `pak8a.pk3` 也不冲突
  （同名 shader 定义内容相同，谁先解析都一样）。
- 代价：`zzqai.pk3` 由 3,434,258 B 增至 6,664,150 B（亮皮蒙版是大头）。

### 新增 — 自洽性校验脚本

- `tools/check_brightskin.py`：全链路检查
  1. 每个 `*_pm.skin` 引用的 shader 在我们自带的 shader 文件里有定义；
  2. 每个 `*_pm.skin` 引用的贴图能解析到；
  3. shader 里每条 `map` 的贴图能解析到；
  4. pk3 内**不得**出现 `scripts/players.shader`。
- 支持 `ZZQAI_SKIP_PK3=pak8a.pk3` 把源包排除后再跑 —— 用来证明"自带资源单独就够用"。
- 实测：常规与排除 pak8a 两种情况均**全部通过**。

### 打包

- `tools/pack_mod.sh` 默认版本号提升到 `1.3.1`；产物
  `release/zzqai.pk3` 与 `release/zzqai-1.3.1-standalone.zip`。
- 两个 VM（`qagame.qvm` / `cgame.qvm`）**无改动**，仍是 1.3.0 的两个文件。

### 文档

- 新增 `assets/brightskin/README.md`（来源、内容清单、与 pak8a 的共存规则、许可）。
- `docs/07-CPMA对齐说明.md`：把原来的"依赖说明"改写为"资源自带说明"。
- `README.md` / `release/README.md`：版本号、体积、新 md5 与自带亮皮说明。

---

## [1.3.0] — 2026-10-10

**CPMA 对齐**：按 Challenge ProMode Arena 的做法统一机器人外观，并把切枪速度改到
CPMA 的水准。两项都是"看得见、摸得着"的改动，与 AI 逻辑无关。

### 新增 — 所有机器人显示为明亮的绿色（CPMA 风格）

- **不改服务端**，走 CPMA 的原生机制：把 cgame 的两个默认值设为
  `cg_enemyModel = "pm"`（`cgame/cg_cvar.h`）与 `cg_enemyColors = "2222"`。
  `cg_players.c` 的 `PM_SKIN`("pm") 分支会把每个机器人**保留自己的模型、
  皮肤强制成 ProMode brightskin**，再用 `cg_enemyColors` 染色。
- 零配置：这两项是 `CVAR_ARCHIVE`，老 `q3config.cfg` 里可能存着空串把默认值
  盖掉，所以在 `CG_RegisterCvars()` 末尾补了一次 `trap_Cvar_Set`；
  玩家若自己设过非空值则完全尊重，不覆盖。
- 同时在 `CG_RegisterCvars()` 末尾打印一行版本横幅
  （`zzqAI cgame 1.3 loaded | enemyModel=... enemyColors=...`），
  用来确认跑的确实是本模组的 cgame 而不是 baseq3 里的那一份。
- 依据：CPMA 用 `cg_enemyModel` + `cg_enemyColors` 把敌人（含机器人）统一成
  绿色；玩家描述为 *"all of the enemy bots are always colored green"*，
  **而这两个 cvar 是客户端设置** —— 服务端给机器人写 `color1/color2` 是没用的，
  因为 Q3 玩家配色是贴图染色，`Default.tga` 那种近黑蒙版乘上绿色仍然是暗的。
- Q3 配色码是 3 位掩码（1=红 2=绿 4=蓝 3=黄 5=品红 6=青 7=白），`2` 即纯绿。
- 恢复原版外观：`set cg_enemyModel ""; set cg_enemyColors ""`。

### 变更 — 切枪速度对齐 CPMA（450 ms → 瞬时）

- `code/game/bg_pmove.c` 的 `PM_BeginWeaponChange()`：把恒定的 `weaponTime += 200`
  改成**只有从出生武器（`WP_GAUNTLET` / `WP_MACHINEGUN`）切出时才 +200**；
  `PM_FinishWeaponChange()` 的举枪延迟 `+250` 改为 `+0`（宏 `ZZQ_WEAPON_RAISE_TIME`）。
  于是普通切枪只花 1~2 帧，原版是 200 + 250 = **450 ms**。
- 依据（多个独立来源一致）：CPMA *"取消了原版 450 ms 的武器切换延迟"*；
  *"instant weapon switching… 0 ms for the first weapon picked up after spawning,
  while imposing a minor 200 ms penalty only for transitions from starting weapons
  like the gauntlet or machinegun"*。
- `bg_pmove.c` 是 game / cgame **共用**的 bg 层，因此 `qagame.qvm` 与 `cgame.qvm`
  **两个 VM 都已同步重编** —— 只改一边会导致客户端预测与服务器不一致（切枪回弹）。
- 人和机器人同样生效（引擎物理层行为，不是 bot 专属）。

### 打包

- `tools/pack_mod.sh` 默认版本号提升到 `1.3.0`。
- 独立包现在打进去的 `cgame.qvm` 也含切枪改动（此前只含伤害数字）。

### 文档

- 新增 `docs/07-CPMA对齐说明.md`：写清 CPMA 亮皮的原理（`*_pm.skin` → shader →
  `rgbGen entity` × 白蒙版 `color.tga`）、本模组的实现，以及**为什么"服务端给机器人
  写 `color1/color2`"行不通**（那条弯路也记在案）。
- `packaging/README.txt`：把机器人外观一节改写成客户端 `cg_enemyModel` /
  `cg_enemyColors` 的说法，并给出回退命令。
- `README.md` / `release/README.md` 同步新版本号、新 md5 与外观 cvar 说明。
- 删除 1.3 开发期围绕旧方案做的临时对照 cfg（`config/cpma-test.cfg`、
  `config/cpma-control.cfg`）。

### 验证

- `build/build_qagame.sh` 与 better-gibs 树的 `build_cgame.sh` 均编译通过：
  `qagame.qvm` 605,612 B、`cgame.qvm` 361,052 B，0 error。
- 独立包内 `vm/qagame.qvm`、`vm/cgame.qvm` 的 md5 与 `release/` 下的产物逐字节一致。
- 启动后控制台会打印 `zzqAI cgame 1.3 loaded | enemyModel=pm enemyColors=2222`
  —— 该横幅可用来确认外观两项已生效、且跑的是本模组的 cgame。
- 未做：绿皮与切枪手感的录像级对照（两者都属表现层/物理手感，不涉及 AI 决策，
  不影响 M1–M6 的验证结论）。

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
