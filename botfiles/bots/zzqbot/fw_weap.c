//===========================================================================
//
// Name:			fw_weap.c
// Function:		zzqBOT 专用武器选择（模糊逻辑，2026-10-09 设计）
// Base:			参照 baseq3 的 fw_weap.c，并按 War bot 的写法加入距离分档
// Notes:			设计要点：
//					  - 偏好武器：轨道炮（远）、火箭筒（中）
//					  - 近身自伤风险武器（火箭/等离/榴弹/BFG）在贴脸时降权
//					  - 闪电枪只在 500 单位内有效（超出射程直接放弃）
//					  - 霰弹枪负责 150~400 的近中距离
//					ENEMY_HORIZONTAL_DIST 由引擎按"实际水平距离(单位)"填入。
// Tab Size:		4 (real tabs)
//===========================================================================


weight "Gauntlet"
{
	switch(INVENTORY_GAUNTLET)
	{
		case 1: return 0;
		default:
		{
			// 铁爪只在贴身时才有意义
			switch(ENEMY_HORIZONTAL_DIST)
			{
				case 40: return $evalint(W_GAUNTLET*8);
				case 80: return $evalint(W_GAUNTLET*3);
				default: return 1;
			} //end switch
		} //end default
	} //end switch
} //end weight

weight "Shotgun"
{
	switch(INVENTORY_SHOTGUN)
	{
		case 1: return 0;
		default:
		{
			switch(INVENTORY_SHELLS)
			{
				case 1: return 0;
				default:
				{
					// 近中距离主力
					switch(ENEMY_HORIZONTAL_DIST)
					{
						case 150: return $evalint(W_SHOTGUN*4);
						case 300: return $evalint(W_SHOTGUN*5);
						case 500: return $evalint(W_SHOTGUN);
						case 800: return $evalint(W_SHOTGUN/2);
						default: return $evalint(W_SHOTGUN/8);
					} //end switch
				} //end default
			} //end switch
		} //end default
	} //end switch
} //end weight

weight "Machinegun"
{
	switch(INVENTORY_MACHINEGUN)
	{
		case 1: return 0;
		default:
		{
			switch(INVENTORY_BULLETS)
			{
				case 1: return 0;
				default:
				{
					// 保底武器，距离越远越没用
					switch(ENEMY_HORIZONTAL_DIST)
					{
						case 400: return W_MACHINEGUN;
						case 900: return $evalint(W_MACHINEGUN/2);
						default: return $evalint(W_MACHINEGUN/4);
					} //end switch
				} //end default
			} //end switch
		} //end default
	} //end switch
} //end weight

weight "Grenade Launcher"
{
	switch(INVENTORY_GRENADELAUNCHER)
	{
		case 1: return 0;
		default:
		{
			switch(INVENTORY_GRENADES)
			{
				case 1: return 0;
				default:
				{
					// 中距离抛射，贴脸禁用（自伤）
					switch(ENEMY_HORIZONTAL_DIST)
					{
						case 120: return 1;
						case 300: return W_GRENADELAUNCHER;
						case 600: return W_GRENADELAUNCHER;
						case 900: return $evalint(W_GRENADELAUNCHER/2);
						default: return $evalint(W_GRENADELAUNCHER/8);
					} //end switch
				} //end default
			} //end switch
		} //end default
	} //end switch
} //end weight

weight "Rocket Launcher"
{
	switch(INVENTORY_ROCKETLAUNCHER)
	{
		case 1: return 0;
		default:
		{
			switch(INVENTORY_ROCKETS)
			{
				case 1: return 0;
				default:
				{
					// 偏好武器之一：中距离王者
					switch(ENEMY_HORIZONTAL_DIST)
					{
						case 120: return $evalint(W_ROCKETLAUNCHER/2);
						case 300: return $evalint(W_ROCKETLAUNCHER*2);
						case 600: return $evalint(W_ROCKETLAUNCHER*3/2);
						case 1000: return W_ROCKETLAUNCHER;
						default: return $evalint(W_ROCKETLAUNCHER/2);
					} //end switch
				} //end default
			} //end switch
		} //end default
	} //end switch
} //end weight

weight "Lightning Gun"
{
	switch(INVENTORY_LIGHTNING)
	{
		case 1: return 0;
		default:
		{
			switch(INVENTORY_LIGHTNINGAMMO)
			{
				case 1: return 0;
				default:
				{
					// 闪电枪射程极短，超出就放弃
					switch(ENEMY_HORIZONTAL_DIST)
					{
						case 250: return $evalint(W_LIGHTNING*2);
						case 500: return $evalint(W_LIGHTNING*3/2);
						default: return $evalint(W_LIGHTNING/10);
					} //end switch
				} //end default
			} //end switch
		} //end default
	} //end switch
} //end weight

weight "Railgun"
{
	switch(INVENTORY_RAILGUN)
	{
		case 1: return 0;
		default:
		{
			switch(INVENTORY_SLUGS)
			{
				case 1: return 0;
				default:
				{
					// 偏好武器之首：距离越远越吃香
					switch(ENEMY_HORIZONTAL_DIST)
					{
						case 200: return $evalint(W_RAILGUN/8);
						case 450: return $evalint(W_RAILGUN/2);
						case 700: return W_RAILGUN;
						case 1000: return $evalint(W_RAILGUN*3/2);
						default: return $evalint(W_RAILGUN*3);
					} //end switch
				} //end default
			} //end switch
		} //end default
	} //end switch
} //end weight

weight "Plasma Gun"
{
	switch(INVENTORY_PLASMAGUN)
	{
		case 1: return 0;
		default:
		{
			switch(INVENTORY_CELLS)
			{
				case 1: return 0;
				default:
				{
					// 弹量越多越愿意用，贴脸降权
					switch(ENEMY_HORIZONTAL_DIST)
					{
						case 100: return $evalint(W_PLASMAGUN/2);
						case 400: return $evalint(W_PLASMAGUN + INVENTORY_CELLS/4);
						case 700: return $evalint(W_PLASMAGUN*3/2 + INVENTORY_CELLS/4);
						default: return $evalint(W_PLASMAGUN*2);
					} //end switch
				} //end default
			} //end switch
		} //end default
	} //end switch
} //end weight

weight "BFG10K"
{
	switch(INVENTORY_BFG10K)
	{
		case 1: return 0;
		default:
		{
			switch(INVENTORY_BFGAMMO)
			{
				case 1: return 0;
				default:
				{
					switch(ENEMY_HORIZONTAL_DIST)
					{
						case 250: return $evalint(W_BFG10K/2);
						default: return W_BFG10K;
					} //end switch
				} //end default
			} //end switch
		} //end default
	} //end switch
} //end weight

weight "Grappling Hook"
{
	switch(INVENTORY_GRAPPLINGHOOK)
	{
		case 1: return 0;
		default:
		{
			return W_GRAPPLE;
		} //end default
	} //end switch
} //end weight
