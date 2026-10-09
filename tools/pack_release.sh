#!/usr/bin/env bash
#
# 把 zzqAI 的产物 + zzqBOT 角色文件打进一个可直接放进游戏目录的 brainworks.pk3
#
# 用法（Windows + Git Bash）：
#   bash tools/pack_release.sh <上游 brainworks.pk3> <输出 brainworks.pk3> \
#                              [zzqbot 资源目录] [cgame.qvm]
#
# 例：
#   bash tools/pack_release.sh upstream/pre-zzqai-brainworks.pk3 \
#                              release/brainworks-new.pk3 \
#                              Z:/q3zzqbot \
#                              release/cgame.qvm
#
# 会写入/覆盖 pk3 内的：
#   vm/qagame.qvm          <- release/qagame.qvm       （本仓库编译）
#   vm/cgame.qvm           <- [cgame.qvm] 或 release/cgame.qvm（若存在）
#   botfiles/bots/**       <- botfiles/bots/**
#   scripts/**.bot         <- scripts/**
#   models/… sound/…       <- [zzqbot 资源目录]（可选，War 素材属第三方，见 NOTICE.md）
#
# NOTE: 机器人列表文件必须落在 pk3 的 scripts/ 下（引擎扫的是 scripts/*.bot），
#       角色文件必须在 botfiles/ 下；.bot 里的 aifile 路径是相对 botfiles/ 的。
#
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_PK3="${1:?请给出上游 brainworks.pk3 路径}"
OUT_PK3="${2:?请给出输出 pk3 路径}"
ASSETS_DIR="${3:-}"
CGAME_QVM="${4:-$ROOT/release/cgame.qvm}"

python - "$ROOT" "$SRC_PK3" "$OUT_PK3" "$ASSETS_DIR" "$CGAME_QVM" <<'PY'
import sys, os, zipfile

root, src, out, assets, cgame = sys.argv[1:6]

# zip 内路径 -> 磁盘路径（None 表示"值为内联数据"）
overlay = {}

qagame = os.path.join(root, "release", "qagame.qvm")
assert os.path.isfile(qagame), "缺少 release/qagame.qvm，先跑 build/build_qagame.sh"
overlay["vm/qagame.qvm"] = qagame

if cgame and os.path.isfile(cgame):
    overlay["vm/cgame.qvm"] = cgame
    print("cgame.qvm   :", os.path.getsize(cgame), "bytes")
else:
    print("cgame.qvm   : 跳过（未提供，pk3 将沿用上游自带的）")

for base, prefix in (
    (os.path.join(root, "botfiles"), "botfiles"),
    (os.path.join(root, "scripts"), "scripts"),
):
    for dirpath, _, files in os.walk(base):
        for fn in files:
            if fn.lower().endswith(".md"):
                continue          # README 之类不进 pk3
            full = os.path.join(dirpath, fn)
            rel = os.path.relpath(full, base).replace("\\", "/")
            overlay[prefix + "/" + rel] = full

if assets and os.path.isdir(assets):
    for dirpath, _, files in os.walk(assets):
        for fn in files:
            full = os.path.join(dirpath, fn)
            # 只吸收美术/音频资源；角色定义与列表由本仓库提供
            rel = os.path.relpath(full, assets).replace("\\", "/")
            parts = rel.split("/")
            if parts[0] in ("models", "sound"):
                overlay[rel] = full

print("qagame.qvm  :", os.path.getsize(qagame), "bytes")
print("overlay 条目:", len(overlay))

zin = zipfile.ZipFile(src, "r")
infos = zin.infolist()
written = set()
out_dir = os.path.dirname(os.path.abspath(out))
if out_dir:
    os.makedirs(out_dir, exist_ok=True)

with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as zout:
    for info in infos:
        name = info.filename
        key = name.lower()
        if key in overlay:
            data = open(overlay[key], "rb").read()
            zi = zipfile.ZipInfo(name, date_time=info.date_time)
            zi.compress_type = zipfile.ZIP_DEFLATED
            zi.external_attr = info.external_attr
            zout.writestr(zi, data)
            written.add(key)
        else:
            zout.writestr(info, zin.read(name))

    # 追加 pk3 里原本没有的新条目
    for name, path in sorted(overlay.items()):
        if name.lower() in written:
            continue
        zout.writestr(name, open(path, "rb").read())
        written.add(name.lower())

zin.close()
print("写入条目    :", len(written))
print("输出        :", out, os.path.getsize(out), "bytes")
PY
