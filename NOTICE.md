# 第三方组件与授权归属

zzqAI 是以下项目的**衍生作品**。遵守 GPL-2.0 的传染性要求，本项目同样以 GPL-2.0 发布，
并在此列明全部上游来源。

## 1. Quake 3 Arena 游戏代码

- 来源：id Software 于 2005 年以 **GPL-2.0** 发布的 Quake 3 Arena 源代码
- 版权：Copyright (C) 1999-2000 id Software, Inc.
- 说明：`code/game/` 下的 `g_*.c`、`bg_*.c`、`q_*.c` 及大部分头文件均源自此。
  部分文件头部保留的 "QIIIA Game Source License" 是 2000 年的早期有限授权，
  之后 id 已统一改以 GPLv2 发布；本仓库以 GPLv2 为准。
  原始文本见 `upstream/QIIIA-Game-Source-License.txt`。

## 2. brainworks Bot AI

- 来源：Ted Vessenes（"Aeye"）的 brainworks 模组，版本 1.0.1
- 版权：Copyright (C) 2002-2007 Ted Vessenes，文件头保留原声明
- 上游原始 pk3：`upstream/brainworks-1.0.1-original.pk3`
- 说明：`code/game/ai_*.c`（除 `ai_duel.c/.h`、`ai_profile.c/.h`）为原样或小幅修改。
  **本项目的价值建立在 brainworks 之上**，请优先支持原作者。

## 3. baseq3a / better-gibs（cgame 基础）

- 来源：WofWca 的 `better-gibs` 项目 `baseq3a` 分支
- 版权：Copyright (C) 1999-2000 id Software, Inc.（cgame 部分）
- 说明：伤害数字功能改自该分支的 cgame，overlay 文件见 `cgame/overlay/`。

## 4. zzqBOT 角色所使用的 War 模型与音效

- 来源：`Quake3bot` 项目（3rd-party bot mod）携带的 War 角色素材
- 说明：**本仓库不附带** `models/players/War/` 与 `sound/player/War/` 等二进制素材
  （体积大且属第三方美术资产）。仓库只含 `botfiles/` 下的角色**定义文件**。
  使用时请自备 War 模型（放到 `models/players/War/`、`sound/player/War/`），
  或把 `botfiles/bots/zzqbot_c.c` 里的 `model` 字段改成任意已有模型。

## 5. lcc / q3asm 工具链

- 来源：Quake 3 SDK 附带的 `lcc` 分支（Jacob Navia 的 lcc 编译器 + id 的 q3asm）
- 说明：`bin/` 下的 `lcc.exe` / `q3cpp.exe` / `q3rcc.exe` / `q3asm.exe` 为原样分发，
  仅用于编译 QVM。
