# zzqBOT —— 自定义机器人模式

## 一、它是什么（以及不是什么）

**zzqBOT 是一个纯粹的运行时数据包，不含任何 AI 逻辑。**

brainworks 的机器人角色是**运行时从 `botfiles/` 读进来的**，不是编进 QVM 的。
所以"造一个新机器人"= 写几份角色数据文件 + 一个索引条目，**不需要重编译 QVM**。

```
scripts/zzqbot.bot          ← 索引：名字、显示名、模型、指向哪个角色文件
    ↓  aifile（路径相对 botfiles/）
botfiles/bots/zzqbot_c.c    ← 角色：性格 + 瞄准/反应/警觉特征（按 skill 分档）
    ├── CHARACTERISTIC_WEAPONWEIGHTS  → bots/zzqbot_w.c   武器选择倾向
    ├── CHARACTERISTIC_ITEMWEIGHTS    → bots/zzqbot_i.c   物品选择倾向
    └── CHARACTERISTIC_CHAT_FILE      → bots/zzqbot_t.c   聊天语料
```

> ⚠️ **两个目录不能搞混**：引擎扫的是 **`scripts/*.bot`** 来发现机器人，
> 而 `.bot` 里的 `aifile` 路径是**相对 `botfiles/`** 的。
> 本仓库因此把 `zzqbot.bot` 放在仓库根目录的 `scripts/`，角色文件放在 `botfiles/`。

它的"智能"完全等于 brainworks 原版 —— 一个字节的 AI 逻辑都没改。
真正让机器人变强的是本仓库的 M1–M4，而 M1–M4 对所有机器人一视同仁，
zzqBOT 只是换了个名字、皮肤和一份调好的角色参数。

## 二、文件清单

| 文件 | 作用 |
|---|---|
| `scripts/zzqbot.bot` | 机器人索引条目 |
| `botfiles/bots/zzqbot_c.c` | 角色定义，含 skill 1 / 4 / 5 三档 |
| `botfiles/bots/zzqbot_w.c` | 武器权重 |
| `botfiles/bots/zzqbot_i.c` | 物品权重 |
| `botfiles/bots/zzqbot_t.c` | 聊天语料 |
| `botfiles/bots/zzqbot/fw_weap.c` | 自定义武器权重（`zzqbot_w.c` 里引用） |

## 三、角色参数设计意图

以 **skill 5** 档为例：

| 特征 | 值 | 意图 |
|---|---|---|
| `AIM_ACCURACY` | 1.00 | 满瞄准精度 |
| `AIM_SKILL` | 1.00 | 满提前量修正 |
| `REACTIONTIME` | 0.00 | 无反应延迟 |
| `VIEW_MAXCHANGE` | 720 | 可以快速转身 |
| `ALERTNESS` | 1.00 | 满警觉 |
| `AGGRESSION` | 1.00 | 好战 |
| `SELFPRESERVATION` | 1.00 | 惜命 —— **配合 M3，会主动脱战补血** |
| `CAMPER` | 0.00 | 不蹲点 |
| `JUMPER` | 0.95 | 爱跳（配合 M4 的绕圈走位） |
| `WEAPONJUMPING` | 0.80 | 愿意火箭跳 |
| `FIRETHROTTLE` | 1.00 | 满射速 |

`SELFPRESERVATION` 拉满是刻意的：brainworks 只有在
`BotShouldRocketJump()` 之类的少数地方读它，而 M3 的脱离判定是独立算的；
但把它拉满能让角色整体倾向"活着比换命重要"，与 M3 的设计方向一致。

> ⚠️ 注意 `AIM_ACCURACY = 1.0` 不等于"100% 命中"。brainworks 会把
> `CHARACTERISTIC_AIM_ACCURACY` 与 `weapon_char_acc[]`、交火距离、
> 目标移动速度等一起算，实际命中率仍受这些因素限制。
> **不要**把它当成"调成 1.0 就是神枪手"的开关 —— 那样只会做出一个让人恶心的 bot。

## 四、安装

### 4.1 只需要角色文件

把本仓库的 `botfiles/` 与 `scripts/` 放进游戏目录：

```
<Quake3>/brainworks/
├── botfiles/
│   └── bots/
│       ├── zzqbot_c.c
│       ├── zzqbot_w.c
│       ├── zzqbot_i.c
│       ├── zzqbot_t.c
│       └── zzqbot/fw_weap.c
└── scripts/
    └── zzqbot.bot
```

**但 gamedir 的 pk3 优先于松散文件** —— 如果 `brainworks.pk3` 里也有 `botfiles/`，
那松散目录里的文件可能被忽略。所以推荐直接打进 pk3：

```bash
bash tools/pack_release.sh release/brainworks.pk3 release/brainworks-new.pk3
```

### 4.2 模型与音效（可选）

`scripts/zzqbot.bot` 里写的是 `model War/Default`。
**本仓库不附带 War 模型与音效**（第三方美术素材，见 `NOTICE.md`）。

三个选择：

1. 自备 War 模型，放到 `models/players/War/` 与 `sound/player/War/`；
2. 把 `scripts/zzqbot.bot` 的 `model` 改成任意已有模型（如 `sarge/Default`、`ranger/Default`）；
3. `pack_release.sh` 的第三个参数指向一个含 `models/players/War` 的目录，自动打进去。

## 五、使用

```
addbot zzqBOT 5
```

参数：`addbot <名字> <技能 0-5> <皮肤> <队伍> <显示名>`

```
addbot zzqBOT 5 "" 0 ^1zzqBOT^7-^4Alpha
addbot zzqBOT 5 "" 0 ^1zzqBOT^7-^4Beta
addbot zzqBOT 5 "" 0 ^1zzqBOT^7-^4Gamma
```

第 5 个参数是显示用别名，避免三个机器人都叫 "zzqBOT" 分不清谁在干什么。

> **CTF 模式下参数会变**：`addbot <队名> <名字> <皮肤>`。
> 如果服务器默认开着 `ctf`，先 `set ctf 0`，否则机器人会全被叫成 "male"。

一键脚本：

```
exec zzqbot.cfg
```

## 六、自己调角色

`bots/zzqbot_c.c` 里按 `skill N { ... }` 分档写。想加一档就再加一个块；
想改性格直接改数值即可，**改完不需要重编译任何东西**，重开一局就生效。

主要可调特征：

| 特征 | 含义 |
|---|---|
| `AIM_SKILL` / `AIM_ACCURACY` | 提前量修正能力 / 瞄准精度 |
| `REACTIONTIME` | 发现敌人到开火的延迟（秒） |
| `ALERTNESS` | 警觉度（影响感知半径） |
| `AGGRESSION` | 好战程度 |
| `SELFPRESERVATION` | 惜命程度 |
| `CAMPER` | 蹲点倾向 |
| `EASY_FRAGGER` | 面对弱者的手软程度 |
| `WEAPONJUMPING` | 火箭跳意愿 |
| `JUMPER` | 跳跃频率 |
| `VIEW_FACTOR` / `VIEW_MAXCHANGE` | 转头平滑度 / 最大角速度 |
