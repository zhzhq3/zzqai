# assets/brightskin —— 自带的 CPMA ProMode 亮皮资源

这一目录由 `tools/extract_brightskin.py` 从 `baseq3/pak8a.pk3`
（better-gibs 系 HD 包，其 cgame 即本模组 cgame 的来源）里**原样抽出**，
再由 `tools/pack_mod.sh` 打进 `zzqai.pk3`。

## 为什么自带

干净安装的 Quake 3（只有 `pak0.pk3`）里：

- **没有** `models/players/*/color.tga` 这类亮皮蒙版
- **没有** `models/players/*/pm` 系列 shader
- **没有** 任何 `*_pm.skin`

也就是说 `/pm` 亮皮完全靠第三方包提供。自带之后，本 mod 在任何一台
干净的 Q3 上都能开出亮皮，**不再依赖 `pak8a.pk3` 或任何外部 HD 包**。

## 内容（136 个文件）

| 类别 | 数量 | 说明 |
|---|---|---|
| `models/players/<模型>/{head,lower,upper}_pm.skin` | 69 | 把模型各块 mesh 指向 `pm` / `pmh` / `pmg` … shader |
| `models/players/<模型>/color.tga` | 23 | 亮皮蒙版：alpha=255 处走 `rgbGen entity` 被染色，alpha=0 处保留底色 |
| `models/players/<模型>/color_h.tga` 等 | 20 | 头/附件用的配套蒙版（`color_h` / `color_x` / `color_t` / `color_skates`） |
| `models/players/<模型>/icon_pm.tga` | 23 | 记分板头像（缺了会导致该皮肤注册失败） |
| `scripts/zzqai_brightskin.shader` | 1 | 44 个 shader 定义，与 `pak8a.pk3` 的 `scripts/players.shader` **逐字一致** |

## 关于和 pak8a 的共存

`zzqai_brightskin.shader` **不叫** `scripts/players.shader`，所以：

- 装了 `pak8a.pk3`：两边都定义同一批 shader 名，内容完全相同 ⇒ 谁先解析都一样，无副作用；
- 没装 `pak8a.pk3`：只用我们这份，照常工作。

`zzqai.pk3` 里**故意不含** `scripts/players.shader`，
`tools/check_brightskin.py` 会把这条当成硬性约束来检查。

## 校验

```bash
bash tools/pack_mod.sh
# 常规自洽检查（皮肤→shader→贴图 全链路可解析）
python tools/check_brightskin.py release/zzqai.pk3

# 排除 pak8a.pk3，证明"自带资源单独就够用"
ZZQAI_SKIP_PK3=pak8a.pk3 python tools/check_brightskin.py release/zzqai.pk3
```

## 重新生成

源包路径不同的话：

```bash
python tools/extract_brightskin.py <某人的 baseq3/pak8a.pk3> assets/brightskin
```

## 许可

亮皮蒙版与 shader 源自 CPMA / OSP 一系的 "ProMode brightskin"，随 Q3 社区
惯例以 GPL-2.0 分发，与本仓库 `LICENSE` 一致。模型 `.md3`、其余原版贴图
仍取自 `baseq3/pak0.pk3`，不在此目录内、也不随本包分发。
