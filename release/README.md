# release —— 可直接使用的产物

## 内容

| 文件 | 说明 |
|---|---|
| `zzqai-1.3.0-standalone.zip` | **推荐**。独立 mod 包，解压到 Quake 3 根目录即可，游戏内 MODS 菜单选中即玩 |
| `zzqai.pk3` | 上面那个包里的 mod 本体（想单独取用） |
| `brainworks.pk3` | 覆盖式安装用的整包（丢进已有的 `brainworks/` 目录覆盖），旧流程保留 |
| `qagame.qvm` | 单独编译出的游戏逻辑 VM，供想自己打进 pk3 的人使用 |
| `cgame.qvm` | 伤害数字 cgame（better-gibs 基座） |

## 快速安装（推荐）

```
1. 解压 zzqai-1.3.0-standalone.zip 到 <Quake3>/（和 baseq3 同级）
2. 启动 → MODS 菜单 → 选中 zzqai
3. MULTIPLAYER → CREATE 开图 → ESC → ADD BOTS → 选难度 → FIGHT
```

不需要启动参数，也不需要控制台命令。

## 校验

`qagame.qvm` / `cgame.qvm` 与本仓库源码的编译产物逐字节一致
（`bash build/build_qagame.sh` 与 `bash build/build_cgame.sh` 可复现），
`zzqai.pk3` 内的两个 VM 与它们 md5 相同：

```
qagame.qvm   605,612 字节   md5 12f89905b980f9b5fbb27d6a8f5bfe75
cgame.qvm    361,052 字节   md5 6e7fc2f4bf53137bb463fb1920d87dc1
```

独立包内 `zzqai/zzqai.pk3`：

```
3,434,258 字节   md5 9b4ed059d47d0eba5add9d2688da543b
```

独立包 `zzqai-1.3.0-standalone.zip`：

```
3,421,868 字节   md5 732c259a0530eb96fb1abb06402e0f46
```

## `qagame.qvm` 手动放进 pk3

`.pk3` 就是 `.zip`：

1. 用任意 zip 工具打开 `zzqai.pk3`
2. 把 `vm/qagame.qvm` **替换**成这里的文件
3. **保持 zip 内的路径为 `vm/qagame.qvm`**（不能多一层目录）

> **不要**把 `qagame.qvm` 放在 `zzqai/vm/` 目录里 ——
> 引擎会优先使用 pk3 内的版本，松散文件会被完全忽略。

脚本版：

```bash
bash tools/pack_mod.sh                                                        # 独立包
bash tools/pack_release.sh upstream/brainworks-1.0.1-original.pk3 release/brainworks.pk3 Z:/q3zzqbot
```

## 回退

想退回原版行为有两种方式，任选其一：

**方式 A（推荐）—— 关 cvar，不换文件：**

```
set bot_profile 0
set bot_item_wait 0
set bot_duel 0
set bot_duel_move 0
set bot_adaptive 0
set bot_share 0
```

已在对照实验中验证：行为完全等同原版，零额外日志。

**方式 B —— 换回原版 pk3：**

```
upstream/brainworks-1.0.1-original.pk3     纯 brainworks 1.0.1（214 KB，无 cgame）
upstream/pre-zzqai-brainworks.pk3          brainworks + 原版 better-gibs cgame（324 KB）
```
