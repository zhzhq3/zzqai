#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
从 baseq3/pak8a.pk3 抽取 CPMA 风格亮皮（ProMode brightskin）资源，
落到 assets/brightskin/，供 pack_mod.sh 打进 zzqai.pk3。

抽什么：
  1. models/players/<model>/{head,lower,upper}_pm.skin   —— 皮肤映射（69 个）
  2. models/players/<model>/icon_pm.tga                 —— 记分板头像（23 个）
  3. models/players/<model>/pm 系列 shader 块 → scripts/zzqai_brightskin.shader
  4. 上述 shader 引用的全部贴图（color.tga / color_h.tga / ...）

为什么要自带：干净安装的 Q3 里根本没有 color.tga，也没有 pm shader，
亮皮完全依赖第三方 HD 包（pak8a.pk3）。自带后本 mod 不再依赖任何外部包。

用法：
  python tools/extract_brightskin.py [源 pak8a.pk3] [输出目录]
"""
import os
import re
import sys
import zipfile

SRC = sys.argv[1] if len(sys.argv) > 1 else r"D:/Quake 3 Arena-vulkan/baseq3/pak8a.pk3"
DST = sys.argv[2] if len(sys.argv) > 2 else os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "assets", "brightskin")

DST = os.path.abspath(DST)
z = zipfile.ZipFile(SRC)
names = z.namelist()

# ---------------------------------------------------------------- 1. skin 文件
skins = sorted(n for n in names if n.lower().endswith("_pm.skin"))
assert skins, "源包里没有 *_pm.skin"

# ------------------------------------------------- 2. skin 里直接引用的贴图路径
direct_tex = set()
# ------------------------------------------------- 3. skin 引用的 shader 名
shader_refs = set()
for s in skins:
    txt = z.read(s).decode("latin-1")
    for line in txt.splitlines():
        line = line.strip()
        if not line or line.startswith("//") or "," not in line:
            continue
        mesh, target = line.split(",", 1)
        target = target.strip()
        if not target:
            continue
        if target.lower().endswith((".tga", ".jpg", ".png")):
            direct_tex.add(target)
        else:
            shader_refs.add(target)

# ---------------------------------------------------------------- 4. 解析 shader
shader_txt = z.read("scripts/players.shader").decode("latin-1")
lines = shader_txt.splitlines()

blocks = {}          # 小写 shader 名 -> (原始名, 块文本)
i = 0
while i < len(lines):
    raw = lines[i].strip()
    # shader 头：单独一行、非空白、不以 { } 开头
    if raw and raw[0] not in "{}" and not raw.startswith("//"):
        name = raw
        j = i + 1
        while j < len(lines) and not lines[j].strip():
            j += 1
        if j < len(lines) and lines[j].strip() == "{":
            depth = 0
            k = j
            while k < len(lines):
                st = lines[k].strip()
                depth += st.count("{") - st.count("}")
                if depth == 0 and st == "}":
                    break
                k += 1
            blocks[name.lower()] = (name, "\n".join(lines[i:k + 1]))
            i = k + 1
            continue
    i += 1

wanted = sorted(shader_refs, key=str.lower)
missing = [w for w in wanted if w.lower() not in blocks]
if missing:
    sys.exit("源 shader 里缺少定义: %s" % missing)

# ------------------------------------------- 5. shader 块引用的贴图（map 行）
for w in wanted:
    body = blocks[w.lower()][1]
    for m in re.finditer(r"^\s*map\s+(\S+)", body, re.M):
        direct_tex.add(m.group(1))

# ---------------------------------------------------------------- 6. 头像图标
icons = sorted(n for n in names if n.lower().endswith("/icon_pm.tga"))

# ---------------------------------------------------------------- 7. 复制落地
def put(src_entry, dst_rel):
    dst = os.path.join(DST, dst_rel.replace("/", os.sep))
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with open(dst, "wb") as f:
        f.write(z.read(src_entry))
    return len(z.read(src_entry))

total = 0
copied = 0

# 7a. 贴图（大小写按 skin/shader 里写的原样保留；源包里按名字解析）
resolved = {n.lower(): n for n in names}
for t in sorted(direct_tex, key=str.lower):
    if t.lower() not in resolved:
        print("  !! 源包缺少贴图:", t)
        continue
    total += put(resolved[t.lower()], t)
    copied += 1

# 7b. skin
for s in skins:
    total += put(s, s)
    copied += 1

# 7c. 图标
for c in icons:
    total += put(c, c)
    copied += 1

# 7d. shader（自己生成一个独立文件，不覆盖 pak8a 的 players.shader）
shader_out = ["// zzqAI - CPMA ProMode brightskin shaders",
              "// extracted from pak8a.pk3 scripts/players.shader by tools/extract_brightskin.py",
              "// stage1: color.tga mask, rgbGen entity  -> tinted by client color1/color2",
              "// stage2: same mask, rgbGen identity     -> keeps the dark base detail",
              ""]
for w in wanted:
    shader_out.append(blocks[w.lower()][1])
    shader_out.append("")
shader_data = ("\n".join(shader_out) + "\n").encode("latin-1")
shader_rel = "scripts/zzqai_brightskin.shader"
dst = os.path.join(DST, shader_rel.replace("/", os.sep))
os.makedirs(os.path.dirname(dst), exist_ok=True)
with open(dst, "wb") as f:
    f.write(shader_data)
copied += 1
total += len(shader_data)

print("源包        :", SRC)
print("skin        :", len(skins))
print("icon_pm.tga :", len(icons))
print("shader 块   :", len(wanted), wanted)
print("其中 map 用贴图 :", len(direct_tex))
print("落地文件    :", copied)
print("落地字节    :", total)
print("输出目录    :", DST)
