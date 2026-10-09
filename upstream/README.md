# upstream —— 上游原始文件

保留在这里是为了**对比、回退与授权追溯**，不参与构建。

| 文件 | 来源 | 说明 |
|---|---|---|
| `brainworks-1.0.1-original.pk3` | brainworks 1.0.1 官方发行包 | 214 KB。内含 `vm/qagame.qvm` + 5 个 `botfiles/*.c`。**纯游戏逻辑，不含 cgame** |
| `pre-zzqai-brainworks.pk3` | 安装 zzqAI 之前的游戏目录状态 | 324 KB。= 上面的 214 KB + 原版 better-gibs `vm/cgame.qvm`，是伤害数字改造的基线 |
| `brainworks-readme.txt` | brainworks 原作者 readme | 原始编译说明（用 `game.bat` + lcc） |
| `description.txt` | brainworks 发行包 | 模组说明 |
| `QIIIA-Game-Source-License.txt` | id Software | Quake 3 源码的 2000 年版有限授权文本（后被 GPLv2 取代） |

## 与当前 release 的差异

```
brainworks-1.0.1-original.pk3   7 个条目
    vm/qagame.qvm
    botfiles/{ichat.h, match.c, rnd.c, ...}

release/brainworks.pk3          51 个条目
    vm/qagame.qvm            ← zzqAI 编译产物
    vm/cgame.qvm             ← better-gibs + 伤害数字
    botfiles/                ← 新增 zzqbot_*.c 等
    scripts/zzqbot.bot       ← 新增
    models/players/War/      ← zzqBOT 使用的模型（22 个文件）
    sound/player/War/        ← zzqBOT 使用的音效（15 个文件）
```

## 授权

`brainworks-1.0.1-original.pk3` 及其 `botfiles/` 归 **Ted Vessenes** 所有，
按 GPL-2.0 分发。详见 [`../NOTICE.md`](../NOTICE.md)。
