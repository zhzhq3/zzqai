#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""校验打包后的 zzqai.pk3 里亮皮资源是否自洽（不依赖 baseq3/pak8a.pk3）。

检查三件事：
  A. 每个 *_pm.skin 引用的 shader 名，在我们自带的 shader 文件里有定义；
  B. 每个 *_pm.skin 引用的贴图（.tga），在 pk3 或 baseq3 里能解析到；
  C. 我们 shader 文件里每条 map 的贴图都能解析到；
  D. pk3 里没有意外覆盖 scripts/players.shader（必须不存在，否则会盖掉别人的）。

用法: python tools/check_brightskin.py [pk3] [baseq3目录]
"""
import os
import re
import sys
import zipfile

PK3 = sys.argv[1] if len(sys.argv) > 1 else r"Z:/zzqai/release/zzqai.pk3"
BASEQ3 = sys.argv[2] if len(sys.argv) > 2 else r"D:/Quake 3 Arena-vulkan/baseq3"

z = zipfile.ZipFile(PK3)
names = z.namelist()
have = set(n.lower() for n in names)

baseq3_files = set()
baseq3_shaders = set()
SKIP = [s.strip().lower() for s in os.environ.get("ZZQAI_SKIP_PK3", "").split(",") if s.strip()]
for fn in sorted(os.listdir(BASEQ3)):
    if fn.lower().endswith(".pk3"):
        if fn.lower() in SKIP:
            print("  (按 ZZQAI_SKIP_PK3 排除)", fn)
            continue
        try:
            bz = zipfile.ZipFile(os.path.join(BASEQ3, fn))
            baseq3_files |= set(n.lower() for n in bz.namelist())
            for n in bz.namelist():
                if n.lower().endswith(".shader"):
                    txt = bz.read(n).decode("latin-1")
                    for m in re.finditer(r"(?m)^(\S[^\s{]*)\s*$", txt):
                        baseq3_shaders.add(m.group(1).lower())
        except Exception as e:
            print("  跳过", fn, e)


def resolvable_tex(path):
    p = path.lower()
    if p in have or p in baseq3_files:
        return True
    # Q3 皮肤可引用"带扩展名的 shader 名"（如 models/players/crash/crash_f.tga
    # 实际对应 shader models/players/crash/crash_f），去掉扩展名再查一次。
    if p.endswith((".tga", ".jpg", ".png")):
        stem = p.rsplit(".", 1)[0]
        if stem in baseq3_shaders or stem in defined or stem in have or stem in baseq3_files:
            return True
        return False
    return False


# ---- 解析自带 shader 文件 ----
shader_src = None
for n in names:
    if n.lower() == "scripts/zzqai_brightskin.shader":
        shader_src = z.read(n).decode("latin-1")
if shader_src is None:
    sys.exit("pk3 里没有 scripts/zzqai_brightskin.shader")

lines = shader_src.splitlines()
defined = {}
i = 0
while i < len(lines):
    raw = lines[i].strip()
    if raw and raw[0] not in "{}" and not raw.startswith("//"):
        j = i + 1
        while j < len(lines) and not lines[j].strip():
            j += 1
        if j < len(lines) and lines[j].strip() == "{":
            # 找块尾
            k, depth = j, 0
            while k < len(lines):
                st = lines[k].strip()
                depth += st.count("{") - st.count("}")
                if depth == 0 and st == "}":
                    break
                k += 1
            defined[raw.lower()] = "\n".join(lines[j:k + 1])
            i = k + 1
            continue
    i += 1

print("自带 shader 定义 :", len(defined))

err = 0
# ---- A / B ----
skins = [n for n in names if n.lower().endswith("_pm.skin")]
tex_refs = set()
for s in skins:
    for line in z.read(s).decode("latin-1").splitlines():
        line = line.strip()
        if not line or line.startswith("//") or "," not in line:
            continue
        mesh, target = line.split(",", 1)
        target = target.strip()
        if not target:
            continue
        if target.lower().endswith((".tga", ".jpg", ".png")):
            if not resolvable_tex(target):
                print("  [B] 贴图找不到 :", target, "(来自 %s)" % s)
                err += 1
        elif target.lower() not in defined:
            print("  [A] shader 未定义:", target, "(来自 %s)" % s)
            err += 1

# ---- C ----
for name, body in defined.items():
    for m in re.finditer(r"^\s*map\s+(\S+)", body, re.M):
        t = m.group(1)
        tex_refs.add(t)
        if not resolvable_tex(t):
            print("  [C] shader 贴图找不到:", t, "(shader %s)" % name)
            err += 1

# ---- D ----
for bad in ("scripts/players.shader",):
    if bad in have:
        print("  [D] 不应包含:", bad)
        err += 1

print("_pm.skin        :", len(skins))
print("color.tga       :", len([n for n in names if n.lower().endswith("/color.tga")]))
print("icon_pm.tga     :", len([n for n in names if n.lower().endswith("/icon_pm.tga")]))
print("shader 引用贴图 :", len(tex_refs))
print("结果            :", "全部通过" if err == 0 else "%d 处问题" % err)
sys.exit(1 if err else 0)
