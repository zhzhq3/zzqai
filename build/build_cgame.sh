#!/bin/bash
# 用 baseq3a 自带工具链编译 cgame.qvm（对齐 build/win32-qvm/compile.bat，全部用相对路径）
set -e
ROOT="/z/q3gibsbq"
TOOL="$ROOT/build/win32-qvm/tools"
VM="$ROOT/build/win32-qvm/vm/cgame"

CGAME_SRCS="cg_main cg_consolecmds cg_draw cg_damagenums cg_drawtools cg_effects cg_ents cg_event cg_info cg_localents cg_marks cg_players cg_playerstate cg_predict cg_scoreboard cg_servercmds cg_snapshot cg_view cg_weapons"
SHARED_SRCS="bg_slidemove bg_pmove bg_lib bg_misc q_math q_shared"

rm -rf "$VM"; mkdir -p "$VM"; cd "$VM"

echo "===== q3lcc ====="
for f in $CGAME_SRCS; do
  "$TOOL/q3lcc.exe" -DQ3_VM -DCGAME -S -Wf-g -I../../../../code/cgame -I../../../../code/game \
      "../../../../code/cgame/$f.c" || { echo "FAIL $f"; exit 1; }
done
for f in $SHARED_SRCS; do
  "$TOOL/q3lcc.exe" -DQ3_VM -DCGAME -S -Wf-g -I../../../../code/cgame -I../../../../code/game \
      "../../../../code/game/$f.c" || { echo "FAIL $f"; exit 1; }
done
cp "$ROOT/code/cgame/cg_syscalls.asm" ./cg_syscalls.asm

echo "===== q3asm ====="
"$TOOL/q3asm.exe" -vq3 -r -m -v -o cgame -f ../../cgame.q3asm
ls -la cgame.qvm
