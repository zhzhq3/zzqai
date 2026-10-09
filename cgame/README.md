# 伤害数字（cgame 部分）

打中敌人时对方身上冒出伤害值 —— 和 CPMA 里那个效果一样。

它由**两个半边**组成：

```
服务端 qagame            客户端 cgame
─────────────           ─────────────
G_Damage()               CG_ServerCommand()
  ↓                        ↑
trap_SendServerCommand(  "dmg 3 47"
  attacker->s.number,      ↓
  "dmg <targ> <take>")   CG_DamagePlum_f()
                           ↓
                         CG_AddDamagePlum(targ, 47)   ← 记下"谁挨了多少血"
                           ↓
                         CG_DrawDamagePlums()         ← 每帧投影到世界坐标画出来
```

服务端那半边在 `code/game/g_combat.c`（已包含在本仓库的 qagame 源码里）：

```c
// g_combat.c，G_Damage() 里 targ->health 扣完之后
if ( attacker->client && targ->client && attacker != targ ) {
    trap_SendServerCommand( attacker->s.number,
                            va( "dmg %i %i", targ->s.number, take ) );
}
```

客户端这半边在 cgame 里，**不在本仓库的 `code/` 树中** ——
因为它来自一个不同的上游项目。

---

## 一、上游来源

cgame 是 `better-gibs`（WofWca）的 **baseq3a 分支**。
Q3 引擎分离 `vm/qagame.qvm` 与 `vm/cgame.qvm`，本项目的 qagame 改编自 brainworks，
cgame 则来自 better-gibs。

## 二、准备工作

1. 取 [better-gibs](https://github.com/WofWca/better-gibs) 的 `baseq3a` 分支源码。
2. 把 `overlay/` 覆盖到源码树上：

```bash
cp -r cgame/overlay/code/cgame/*        <better-gibs>/code/cgame/
cp    cgame/overlay/build/win32-qvm/cgame.q3asm  <better-gibs>/build/win32-qvm/
```

3. 把本仓库的 `build/build_cgame.sh` 拷进去（路径按你的目录改一下），然后：

```bash
bash build_cgame.sh
# 产物 <better-gibs>/build/win32-qvm/vm/cgame/cgame.qvm
```

## 三、改动清单

覆盖的文件共 6 个（1 个新增 + 5 个修改）：

| 文件 | 改动 |
|---|---|
| `code/cgame/cg_damagenums.c` | **新增**。全部实现 |
| `code/cgame/cg_cvar.h` | 新增 3 个 cvar 声明 |
| `code/cgame/cg_local.h` | 新增函数原型 |
| `code/cgame/cg_main.c` | `CG_Init()` 里调用 `CG_ClearDamagePlums()` |
| `code/cgame/cg_draw.c` | `CG_Draw2D()` 里（记分板之后）调用 `CG_DrawDamagePlums()` |
| `code/cgame/cg_servercmds.c` | 解析 `dmg` 服务端命令 → `CG_DamagePlum_f()` |
| `build/win32-qvm/cgame.q3asm` | 文件清单加入 `cg_damagenums` |

### `cg_damagenums.c` 的参数

```c
#define MAX_DAMAGE_PLUMS      16      // 同时最多 16 个数字
#define DAMAGE_PLUM_TIME      1000    // 数字存活时长 (ms)
#define DAMAGE_PLUM_RISE      40.0f   // 存活期内向上漂浮的世界单位
#define DAMAGE_PLUM_HEIGHT    46.0f   // 相对敌人原点的起始高度
#define DAMAGE_PLUM_MERGE_MS  400     // 400ms 内对同一敌人的连续命中合并成一个数字
#define DAMAGE_PLUM_FONT_SCALE 0.5f   // 字号相对 BIGCHAR(16px)：0.5 = 8px
```

`DAMAGE_PLUM_MERGE_MS` 是必须的：机枪一秒钟能打十几发，
不合并的话屏幕上会糊成一片，看不出实际伤害量。

字号按需求**缩到原来的一半**（16 px → 8 px），改 `DAMAGE_PLUM_FONT_SCALE` 即可。

## 四、cvar

| cvar | 默认 | 说明 |
|---|---|---|
| `cg_damageNumbers` | 1 | 总开关 |
| `cg_damageNumbersScale` | 1 | 整体缩放（叠加在 0.5 的基础字号上） |
| `cg_damageNumbersDebug` | 0 | 打印数字生成/绘制诊断（很刷屏） |

## 五、部署

把编译出的 `cgame.qvm` 放进 pk3 的 `vm/cgame.qvm`（和 `vm/qagame.qvm` 同一个 pk3 即可）：

```bash
python - <<'PY'
import zipfile, os
src = "brainworks.pk3"
qvm = open("cgame.qvm", "rb").read()
zin = zipfile.ZipFile(src); infos = zin.infolist()
with zipfile.ZipFile(src + ".tmp", "w", zipfile.ZIP_DEFLATED, 9) as zout:
    for i in infos:
        if i.filename.lower() == "vm/cgame.qvm":
            zi = zipfile.ZipInfo("vm/cgame.qvm", date_time=i.date_time)
            zi.compress_type = zipfile.ZIP_DEFLATED
            zout.writestr(zi, qvm)
        else:
            zout.writestr(i, zin.read(i.filename))
os.replace(src + ".tmp", src)
PY
```

---

## 六、已知限制

- 只有**本地玩家**打中敌人时才显示（服务端只发给攻击者）。他人互相打不显示，
  这是刻意的：避免变成"全图伤害透视"。
- 数字是屏幕空间投影到世界坐标，不做遮挡判断 —— 敌人躲墙后数字仍会飘。
  CPMA 也是这样，属于风格化选择。
- 同一敌人的多次命中会在 400 ms 内合并，所以看到的是"累计值"而不是每一发。
