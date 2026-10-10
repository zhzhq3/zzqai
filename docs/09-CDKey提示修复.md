# 09 · 为什么只有 zzqai 这个 mod 会问 CD key（1.3.2 修复）

> 现象：把本 mod 装好后，每次在 MODS 菜单里选中 **zzqai**，进主菜单前面
> 都会弹出一个要你填 CD key 的框；换回 baseq3 或别的 mod 就没有。
>
> 结论：**不是本 mod 的 bug，是"借来的界面 + 没有本 gamedir 的 key 文件"
> 这两件事凑到一起的必然结果。** 修法是往 mod 目录里补一个 `q3key`。

---

## 1. 弹框的真正触发条件

界面（`ui.qvm`）里的 `UI_MainMenu()`（ioquake3 `code/q3_ui/ui_menu.c:276`）：

```c
void UI_MainMenu( void ) {
	trap_Cvar_Set( "sv_killserver", "1" );

	if( !uis.demoversion && !ui_cdkeychecked.integer ) {
		char	key[17];
		trap_GetCDKey( key, sizeof(key) );
		if( trap_VerifyCDKey( key, NULL ) == qfalse ) {
			UI_CDKeyMenu();          // ← 就是它弹的框
			return;
		}
	}
	...
```

三个条件必须同时成立才会弹：

| 条件 | 本地实际情况 |
|---|---|
| `!uis.demoversion` | `demoversion` 只在 `ui_gameinfo.c:814` 被赋成 `qfalse`，全程恒为假 → 恒为真 |
| `!ui_cdkeychecked` | 该 cvar 是 `CVAR_ROM`（`ui_main.c:216`），不进配置文件 ⇒ 每次启动都是 `0` |
| `trap_VerifyCDKey(GetCDKey())` 失败 | **就是这一条在 zzqai 下失败** |

所以关键在于第三个条件：**取到的 key 是什么**。

---

## 2. key 是从哪取的：`cl_cdkey` 的两段结构

引擎里 CD key 是一个 34 字节的全局数组（`code/qcommon/common.c:2459`）：

```c
char	cl_cdkey[34] = "                                ";
                       ↑ [0..15]              ↑ [16..31]
                         baseq3 的 key          当前 gamedir 的 key
```

界面取 key（`code/client/cl_ui.c:640`）：

```c
static void CLUI_GetCDKey( char *buf, int buflen ) {
	gamedir = Cvar_VariableString( "fs_game" );
	if (UI_usesUniqueCDKey() && gamedir[0] != 0) {
		Com_Memcpy( buf, &cl_cdkey[16], 16);   // ← 用「本 gamedir 的 key」
	} else {
		Com_Memcpy( buf, cl_cdkey, 16);        // ← baseq3 的 key
	}
}
```

而 `UI_usesUniqueCDKey()` 是问界面模块要答案的（`cl_ui.c:1146`）：

```c
qboolean UI_usesUniqueCDKey( void ) {
		return (VM_Call( uivm, UI_HASUNIQUECDKEY) == qtrue);
}
```

**界面模块的答卷**（ioquake3 `code/q3_ui/ui_main.c:81`）：

```c
	case UI_HASUNIQUECDKEY:				// mod authors need to observe this
		return qtrue;  // change this to qfalse for mods!
	}
```

源码里那句注释就是全部答案：*"mod 作者要把它改成 qfalse！"*
ioquake3 系的界面一律 **qtrue**，也就是"**每个 gamedir 各自要一份 key**"。

---

## 3. 于是链条闭合了

启动 / 换 gamedir 时（`code/qcommon/files.c:3433`）：

```c
	if (!com_standalone->integer) {
		Com_ReadCDKey(BASEGAME);                 // 读 <root>/baseq3/q3key  → cl_cdkey[0..15]
		if (fs_gamedirvar->string[0]) {
			Com_AppendCDKey(fs_gamedirvar->string);  // 读 <root>/zzqai/q3key → cl_cdkey[16..31]
		}
	}
```

* `baseq3/q3key` 存在（正常装过游戏都会有）⇒ 前半段有值。
* **`zzqai/q3key` 不存在** ⇒ `Com_AppendCDKey` 取不到，把 `cl_cdkey[16..31]` 清空。
* 界面判定 `unique = qtrue` 且 `fs_game = "zzqai"` ⇒ 取的是 **空的** 后半段。
* `trap_VerifyCDKey("")` ⇒ 长度 0 ≠ 16 ⇒ **假** ⇒ **弹框**。

换回 baseq3 时 `fs_game` 为空 ⇒ 走 `else` 分支取前半段（有值）⇒ 不弹。
**这就是"只有这个 mod 会问"的全部原因。**

> 顺带：为什么别的 mod 大多不弹？因为成熟 mod 一般**自带 `vm/ui.qvm`**，
> 而自带界面的 mod 通常把 `UI_HASUNIQUECDKEY` 编译成 `qfalse`，
> 于是根本不走"逐 gamedir"这条路。本 mod 沿用了 baseq3 的界面，就把这个
> 行为一起继承过来了。

---

## 4. 修法：往 mod 目录里放一个 `q3key`

校验函数（`code/client/cl_main.c:4704`，注意最后那个 `if`）：

```c
qboolean CL_CDKeyValidate( const char *key, const char *checksum ) {
	len = strlen(key);
	if( len != CDKEY_LEN ) return qfalse;        // CDKEY_LEN = 16
	sum = 0;
	for (i = 0; i < len; i++) {
		switch(tolower(*key++)) {
		case '2': case '3': case '7':
		case 'a': ... case 'w':
			sum += ch; continue;
		default: return qfalse;              // 只允许这几个字母
		}
	}
	...
	if (!checksum) {
		return qtrue;                        // ← checksum 为 NULL 时直接放行
	}
```

`Com_ReadCDKey` / `trap_VerifyCDKey` 传的都是 `NULL`，所以规则简化成：

> **只要长度正好 16、字符全落在 `237abcdghjlprstw` 里，就算合法。**

本仓库 `packaging/q3key` 前 16 字节是 `2ab3cd7gahljprst`，满足条件。
（这个 key 只过本地这道格式检查，不参与任何联网验证；`cl_guid` 也不是从它
推导的 —— 那来自独立的 `qkey` 文件，见 `cl_main.c:3463 CL_GenerateQKey`。）

### 必须注意的两点

1. **必须是松散文件 `<Quake3>/zzqai/q3key`，打进 pk3 无效。**
   引擎读它走的是操作系统文件接口（`FS_BaseDir_FOpenFileRead` → `Sys_FOpen`），
   根本不查 pk3 压缩包。
2. **前 16 字节不能被改动。** 用记事本另存、加 UTF-8 BOM、被换行工具改写，
   都可能弄坏。所以这个文件故意不带 BOM。

---

## 5. 这个位置真的会被读到吗 —— 取证

会。用的是**同一个函数、同一种调用形状**，而那条路是**今天已经实测跑通**的。

### 5.1 源码：两个文件走的是同一条路

```c
// code/qcommon/files.c:2459  —— MODS 菜单读「模组说明」
void FS_GetModDescription( const char *modDir, char *description, int descriptionLen ) {
	Com_sprintf( descPath, sizeof(descPath), "%s%cdescription.txt", modDir, PATH_SEP );
	nDescLen = FS_BaseDir_FOpenFileRead( descPath, &descHandle );   // ← 同一个函数
```

```c
// code/qcommon/common.c:2477  —— CD key 读取
	Com_sprintf(fbuffer, sizeof(fbuffer), "%s/q3key", filename);
	FS_BaseDir_FOpenFileRead( fbuffer, &f );                        // ← 同一个函数
```

`description.txt` 的调用形状是 `"<gamedir>/description.txt"`，`q3key` 是
`"<gamedir>/q3key"` —— **完全同构**。

### 5.2 路径怎么拼出来的

```c
// code/qcommon/files.c:526
char *FS_BaseDir_BuildOSPath( const char *base, const char *qpath ) {
	char *ospath = FS_BuildOSPath(base, qpath, "");   // ← qpath 走 game 槽
	ospath[strlen(ospath) - 1] = '\0';                // 去掉末尾斜杠
	return ospath;
}
// code/qcommon/files.c:501
Com_sprintf( temp, sizeof(temp), "/%s/%s", game, qpath );
Com_sprintf( ospath[toggle], sizeof(ospath[0]), "%s%s", base, temp );
```

`FS_BaseDir_FOpenFileRead` 会依次拿 `fs_pathVars[]` 里的每一项当 `base`
（`files.c:3342 FS_InitPathVars`）：

```
fs_homeconfigpath, fs_homedatapath, fs_homestatepath,
fs_basepath, fs_apppath, fs_steampath, fs_gogpath, fs_microsoftstorepath
```

`fs_basepath` **就在里面**（第 4 项）。于是
`FS_BaseDir_FOpenFileRead("zzqai/q3key")` 必然尝试
`<fs_basepath>/zzqai/q3key`。

### 5.3 实测锚点：`fs_basepath` 就是游戏根目录

用引擎本体跑一次（专用服务器 + 日志）：

```bash
cd "D:/Quake 3 Arena-vulkan"
./quake3e.ded.x64.exe +set fs_game zzqai +set logfile 1 \
  +set net_port 28043 +set bot_enable 0 +map q3dm1 \
  +which description.txt +path +quit
```

日志原文：

```
File "description.txt" found at "D:\Quake 3 Arena-vulkan\zzqai"
Current search path:
D:\Quake 3 Arena-vulkan\zzqai\zzqai.pk3 (187 files)
D:\Quake 3 Arena-vulkan\zzqai
c:/program files (x86)/steam\SteamApps\common\Quake 3 Arena\zzqai
...
version\Q3 1.32e win_mingw-x86_64 May 26 2026
```

⇒ 安装根目录就是 `D:\Quake 3 Arena-vulkan`，也就是 `fs_basepath`。
独立包解压出来的 `zzqai/` 正好落在这里，于是：

```
<fs_basepath> / zzqai / q3key
= D:\Quake 3 Arena-vulkan\zzqai\q3key      ← 部署位置
```

### 5.4 ⚠️ 别用 `which` 去验证它

```
File not found: "zzqai/q3key"
```

这条是**正常且预期**的，不是反证 —— `which` 走的是另一条路：
`FS_Which_f` 用 `FS_FOpenFileReadDir()` 在**每条 search path 的根**上找文件
（`files.c:2809`），而 search path 已经是 `<root>/zzqai`，
所以它实际查的是 `<root>/zzqai/zzqai/q3key`（多了一层）。
CD key 走的 `FS_BaseDir_FOpenFileRead` 才把 `fs_basepath` 本身当根。
两个函数不同，别混。

---

## 6. 改动清单（1.3.2）

| 文件 | 改动 |
|---|---|
| `packaging/q3key` | 新增。前 16 字节 `2ab3cd7gahljprst` + 说明注释，CRLF，无 BOM |
| `tools/pack_mod.sh` | 独立包内增加 `zzqai/q3key`；打包时**强校验**前 16 字节合法，不合法直接报错退出；默认版本号 1.3.1 → 1.3.2 |
| `packaging/README.txt` | 安装树补 `q3key`；新增"六之二、它老是问我 CD key"一节 |
| `packaging/description.txt` | 版本号 1.3.2 |

`zzqai.pk3` **没有任何改动**（md5 与 1.3.1 相同）——
这个问题的修复完全落在独立包的那一个松散文件上。

---

## 7. 如果以后还是弹框，按这个顺序查

1. `zzqai/q3key` 在不在游戏目录里？
2. 它有没有被误打进 `zzqai.pk3`？（应**在包外**）
3. 用十六进制看一眼前 16 字节还是不是 `2ab3cd7gahljprst`
   （记事本另存 / BOM / 换行转换都可能弄坏）。
4. 引擎的 home 路径如果被设成了别处（`fs_homepath`），把同一份 `q3key`
   复制到 `<homepath>/zzqai/q3key` 也能生效 —— 因为 `fs_homedatapath` /
   `fs_homestatepath` 同样在 `fs_pathVars` 列表里。
