# release —— 可直接使用的产物

## 内容

| 文件 | 说明 |
|---|---|
| `brainworks.pk3` | **完整组装好的模组**，含新 qagame.qvm + 伤害数字 cgame.qvm + zzqBOT 角色与素材。丢进游戏就能玩 |
| `qagame.qvm` | 单独编译出的游戏逻辑 VM（594,524 字节），供想自己打进 pk3 的人使用 |

## 快速安装

```
1. 备份 <Quake3>/brainworks/brainworks.pk3
2. 用这里的 brainworks.pk3 覆盖它
3. 启动，控制台执行：  exec zzqbot.cfg
```

或命令行：

```
quake3e.x64.exe +set fs_game brainworks +devmap q3dm4 +exec zzqbot.cfg
```

## 校验

`qagame.qvm` 与本仓库 `code/game/` 源码的编译产物逐字节一致
（`bash build/build_qagame.sh` 可复现）：

```
大小    594,524 字节
md5     2edbfaf7799339059da19d19d0004386
```

`brainworks.pk3` 内 `vm/qagame.qvm` 与上面的 md5 相同。

## `qagame.qvm` 手动放进 pk3

`.pk3` 就是 `.zip`：

1. 用任意 zip 工具打开 `brainworks.pk3`
2. 把 `vm/qagame.qvm` **替换**成这里的文件
3. **保持 zip 内的路径为 `vm/qagame.qvm`**（不能多一层目录）

> **不要**把 `qagame.qvm` 放在 `brainworks/vm/` 目录里 ——
> 引擎会优先使用 pk3 内的版本，松散文件会被完全忽略。

脚本版：

```bash
bash tools/pack_release.sh upstream/brainworks-1.0.1-original.pk3 release/brainworks.pk3
```

## 回退

想退回原版行为有两种方式，任选其一：

**方式 A（推荐）—— 关 cvar，不换文件：**

```
set bot_profile 0
set bot_item_wait 0
set bot_duel 0
set bot_duel_move 0
```

已在对照实验中验证：行为完全等同原版，零额外日志。

**方式 B —— 换回原版 pk3：**

```
upstream/brainworks-1.0.1-original.pk3     纯 brainworks 1.0.1（214 KB，无 cgame）
upstream/pre-zzqai-brainworks.pk3          brainworks + 原版 better-gibs cgame（324 KB）
```
