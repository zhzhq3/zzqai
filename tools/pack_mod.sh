#!/usr/bin/env bash
#
# 组装 zzqAI 独立 mod 包（解压即用，不需要任何命令行参数）
#
# 目标形态：
#   <Quake3>/zzqai/              ← 解压出来就是这么一个目录
#   ├── zzqai.pk3                ← mod 本体
#   ├── description.txt          ← MODS 菜单里显示的说明
#   └── README.txt               ← 中文安装说明
#
# 装好后启动游戏 → MODS 菜单里会多出一项 "zzqai" → 选中 → 开图即可。
# 机器人由玩家自己在游戏内菜单里添加：ESC → ADD BOTS，菜单自带难度选择。
#
# 用法（Windows + Git Bash）：
#   bash tools/pack_mod.sh [War素材目录]
#
# 素材目录默认 /z/q3zzqbot，里面应有 models/ 与 sound/（zzqBOT 用的 War 皮肤）。
# 该目录不存在时，自动从 release/zzqai.pk3 或 release/brainworks.pk3 里就地取用。
#
# 依赖产物：
#   release/qagame.qvm   ← bash build/build_qagame.sh
#   release/cgame.qvm    ← better-gibs 伤害数字 cgame（随仓库提供）
#
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ASSETS="${1:-/z/q3zzqbot}"
VERSION="${ZZQAI_VERSION:-1.3.0}"
GAMEDIR="zzqai"

BASE="$ROOT/upstream/brainworks-1.0.1-original.pk3"
OUT_PK3="$ROOT/release/${GAMEDIR}.pk3"
OUT_ZIP="$ROOT/release/${GAMEDIR}-${VERSION}-standalone.zip"

[ -f "$ROOT/release/qagame.qvm" ] || { echo "缺少 release/qagame.qvm，先跑 bash build/build_qagame.sh"; exit 1; }
[ -f "$ROOT/release/cgame.qvm" ]  || { echo "缺少 release/cgame.qvm"; exit 1; }
[ -f "$BASE" ]                    || { echo "缺少上游基底 $BASE"; exit 1; }
[ -f "$ROOT/packaging/README.txt" ]      || { echo "缺少 packaging/README.txt"; exit 1; }
[ -f "$ROOT/packaging/description.txt" ] || { echo "缺少 packaging/description.txt"; exit 1; }

python - "$ROOT" "$ASSETS" "$BASE" "$OUT_PK3" "$OUT_ZIP" "$GAMEDIR" "$VERSION" <<'PY'
import sys, os, zipfile

root, assets, base, out_pk3, out_zip, gamedir, version = sys.argv[1:8]

# 条目索引： 小写路径 -> (pk3 内真实路径, 数据来源)
# 数据来源有两种：磁盘文件路径，或 ("@pk3", pk3路径, 内部路径)
overlay = {}

def add(pk3path, src):
    overlay[pk3path.lower()] = (pk3path, src)

def disk(full, pk3path):
    add(pk3path, ("@disk", full))

# ---- 1. 两个 VM ----
disk(os.path.join(root, "release", "qagame.qvm"), "vm/qagame.qvm")
disk(os.path.join(root, "release", "cgame.qvm"),  "vm/cgame.qvm")

# ---- 2. 本仓库的 bot 角色数据 ----
for base_dir, prefix in ((os.path.join(root, "botfiles"), "botfiles"),
                         (os.path.join(root, "scripts"),  "scripts")):
    assert os.path.isdir(base_dir), base_dir
    for dirpath, _, files in os.walk(base_dir):
        for fn in files:
            if fn.lower().endswith(".md"):
                continue                      # README 之类不进 pk3
            full = os.path.join(dirpath, fn)
            rel  = os.path.relpath(full, base_dir).replace("\\", "/")
            disk(full, prefix + "/" + rel)

# ---- 3. 素材：只吸收 models/ 与 sound/ ----
def absorb(src):
    if not src:
        return 0
    n = 0
    if os.path.isdir(src):
        for dirpath, _, files in os.walk(src):
            for fn in files:
                full = os.path.join(dirpath, fn)
                rel  = os.path.relpath(full, src).replace("\\", "/")
                if rel.split("/")[0] in ("models", "sound"):
                    disk(full, rel)
                    n += 1
    elif os.path.isfile(src) and src.lower().endswith((".pk3", ".zip")):
        z = zipfile.ZipFile(src)
        for name in z.namelist():
            if name.endswith("/"):
                continue
            if name.split("/")[0] in ("models", "sound"):
                add(name, ("@pk3", src, name))
                n += 1
        z.close()
    return n

n_assets = absorb(assets)
if n_assets == 0:
    for cand in (os.path.join(root, "release", gamedir + ".pk3"),
                 os.path.join(root, "release", "brainworks.pk3")):
        if os.path.isfile(cand):
            n_assets = absorb(cand)
            if n_assets:
                print("素材回退取自 :", cand)
                break
if n_assets == 0:
    sys.exit("找不到 models/ 与 sound/ 素材（试过 %s 及 release/*.pk3）" % assets)

def payload(src):
    if src[0] == "@disk":
        return open(src[1], "rb").read()
    _, pk, inner = src
    z = zipfile.ZipFile(pk)
    data = z.read(inner)
    z.close()
    return data

print("qagame.qvm   :", os.path.getsize(os.path.join(root, "release", "qagame.qvm")), "bytes")
print("素材条目     :", n_assets)
print("overlay 合计 :", len(overlay))

# ---- 4. 写 pk3：以上游 pk3 为骨架，逐条覆盖，再追加新条目 ----
written = set()
os.makedirs(os.path.dirname(os.path.abspath(out_pk3)), exist_ok=True)

zin = zipfile.ZipFile(base, "r")
with zipfile.ZipFile(out_pk3, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as zout:
    for info in zin.infolist():
        name = info.filename
        key  = name.lower()
        if key in overlay:
            canon, src = overlay[key]
            zi = zipfile.ZipInfo(name, date_time=info.date_time)
            zi.compress_type = zipfile.ZIP_DEFLATED
            zi.external_attr = info.external_attr
            zout.writestr(zi, payload(src))
            written.add(key)
        else:
            zout.writestr(info, zin.read(name))
    zin.close()

    for key in sorted(overlay):
        canon, src = overlay[key]
        if key in written:
            continue
        zout.writestr(canon, payload(src))
        written.add(key)

print("pk3 条目     :", len(written))
print("pk3 产物     :", out_pk3, os.path.getsize(out_pk3), "bytes")

# ---- 5. 打独立包 zip：顶层就是 zzqai/ 目录 ----
readme_data = open(os.path.join(root, "packaging", "README.txt"), "rb").read()
if not readme_data.startswith(b"\xef\xbb\xbf"):
    readme_data = b"\xef\xbb\xbf" + readme_data     # 给 Windows 记事本一个 BOM

with zipfile.ZipFile(out_zip, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    z.write(out_pk3, "%s/%s.pk3" % (gamedir, gamedir))
    z.writestr("%s/description.txt" % gamedir,
               open(os.path.join(root, "packaging", "description.txt"), "rb").read())
    z.writestr("%s/README.txt" % gamedir, readme_data)

print("独立包       :", out_zip, os.path.getsize(out_zip), "bytes")
print("包内结构     : %s/{ %s.pk3, description.txt, README.txt }" % (gamedir, gamedir))
PY

echo
echo "===== 独立包目录树 ====="
python - "$OUT_ZIP" <<'PY'
import sys, zipfile
z = zipfile.ZipFile(sys.argv[1], "r")
for n in z.namelist():
    print("  %9d  %s" % (z.getinfo(n).file_size, n))
PY
