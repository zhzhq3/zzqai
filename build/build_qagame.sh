#!/usr/bin/env bash
#
# 编译 zzqAI 的 qagame.qvm（Quake 3 VM 字节码）
#
# 用法：
#   bash build/build_qagame.sh
#
# 依赖：
#   - 仓库自带 bin/ 下的 Windows 版 q3lcc / q3cpp / q3rcc / q3asm
#     （在 Windows + Git Bash 下可直接运行；Linux/macOS 需要 wine）
#
# 产物：
#   code/game/vm/qagame.qvm
#
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GAME="$ROOT/code/game"
BIN="$ROOT/bin"

# q3lcc 需要能在 PATH 里找到同目录的 q3cpp / q3rcc
export PATH="$BIN:$PATH"

cd "$GAME"
mkdir -p vm
cd vm

# q3asm 是原生 Windows 程序，不认识 MSYS 风格的 /z/... 路径，
# 必须把输出路径转成 Windows 形式（Z:/...），否则它会写到别的地方去。
WIN_GAME="$(cd "$GAME" && pwd -W 2>/dev/null || pwd)"

# 源文件清单——必须与 brainworks 原版一致，只多了 ai_duel
FILES="
g_main
bg_misc
bg_lib
bg_pmove
bg_slidemove
q_math
q_shared
ai_accuracy
ai_action
ai_adaptive
ai_aim
ai_attack
ai_aware
ai_chat
ai_client
ai_command
ai_dodge
ai_duel
ai_entity
ai_goal
ai_item
ai_level
ai_lib
ai_main
ai_maingoal
ai_motion
ai_move
ai_order
ai_path
ai_pickup
ai_predict
ai_profile
ai_region
ai_resource
ai_scan
ai_self
ai_share
ai_subteam
ai_team
ai_use
ai_vars
ai_view
ai_visible
ai_waypoint
ai_weapon
g_active
g_arenas
g_bot
g_client
g_cmds
g_combat
g_items
g_mem
g_misc
g_missile
g_mover
g_session
g_spawn
g_svcmds
g_target
g_team
g_trigger
g_utils
g_weapon
"

echo "===== q3lcc（逐文件生成 .asm） ====="
for f in $FILES; do
  "$BIN/lcc.exe" -DQ3_VM -S -Wf-target=bytecode -Wf-g -I../../game "../$f.c" \
    || { echo "LCC FAIL: $f"; exit 1; }
done

echo "===== 生成 qagame.list ====="
{
  echo "-o \"$WIN_GAME/vm/qagame\""
  for f in $FILES; do echo "$f"; done
  # trap_* 系统调用表（VM 版，equ trap_X -N）
  echo "../g_syscalls.asm"
} > qagame.list

echo "===== q3asm（链接成 qvm） ====="
"$BIN/q3asm.exe" -f qagame.list

echo "===== 产物 ====="
ls -la "$GAME/vm/qagame.qvm"
