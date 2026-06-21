// Console command entry points.
// Included by swiftlybhoptimer_helper.cpp to keep helper features grouped without changing linkage.

CON_COMMAND_F(sbt_replaybot_add, "Requests a SwiftlyBhopTimer replay bot.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.RequestReplayBotAdd("server_command", args.ArgC() > 1 ? args.Arg(1) : nullptr, false);
}

CON_COMMAND_F(sbt_replaybot_add_once, "Requests a one-shot SwiftlyBhopTimer replay bot.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.RequestReplayBotAdd("server_command_once", args.ArgC() > 1 ? args.Arg(1) : nullptr, true);
}

CON_COMMAND_F(sbt_replaybot_kick, "Kicks SwiftlyBhopTimer replay bots.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.RequestReplayBotKick("server_command", args.ArgC() > 1 ? args.Arg(1) : nullptr);
}

CON_COMMAND_F(sbt_replaybot_complete, "Completes and kicks a one-shot SwiftlyBhopTimer replay bot.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.RequestReplayBotComplete("server_command_complete", args.ArgC() > 1 ? args.Arg(1) : nullptr);
}

CON_COMMAND_F(sbt_replaybot_convars_apply, "Forces SwiftlyBhopTimer replay bot convars.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.ApplyReplayBotConvars(true);
}

CON_COMMAND_F(sbt_replaybot_optimize, "Applies SwiftlyBhopTimer replay bot playback network flags.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.ApplyReplayBotPlaybackFixes(args.ArgC() > 1 ? std::atoi(args.Arg(1)) != 0 : true);
}

CON_COMMAND_F(sbt_force_team, "Forces a real player onto a team without using the normal team slot gate.", FCVAR_NONE)
{
	if (args.ArgC() < 3)
	{
		SbtMessage("sbt_force_team usage: sbt_force_team <slot> <team>\n");
		return;
	}

	g_SwiftlyBhopTimerHelper.ForcePlayerTeam(std::atoi(args.Arg(1)), std::atoi(args.Arg(2)));
}

CON_COMMAND_F(sbt_bhop_physics_apply, "No-op legacy command; SwiftlyBhopTimer no longer applies bhop movement convars.", FCVAR_NONE)
{
	const char *gravity = args.ArgC() > 1 ? args.Arg(1) : "800";
	const char *jumpImpulse = args.ArgC() > 2 ? args.Arg(2) : "301.993378";
	const char *airAccelerate = args.ArgC() > 3 ? args.Arg(3) : "1000";
	const char *airMaxWishSpeed = args.ArgC() > 4 ? args.Arg(4) : "35";
	const char *friction = args.ArgC() > 5 ? args.Arg(5) : "4";

	g_SwiftlyBhopTimerHelper.ApplyBhopPhysicsConvars(gravity, jumpImpulse, airAccelerate, airMaxWishSpeed, friction);
}

CON_COMMAND_F(sbt_roundflow_apply, "Applies SwiftlyBhopTimer round and warmup convars.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.ApplyRoundFlowConvars();
}

CON_COMMAND_F(sbt_movement_compat_apply, "Applies SwiftlyBhopTimer jump/subtick compatibility convars.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.ApplyMovementCompatibilityConvars(false);
}

CON_COMMAND_F(sbt_movement_compat_burst, "Schedules repeated SwiftlyBhopTimer jump/subtick compatibility convar application.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.ScheduleMovementCompatibilityBurst();
}

CON_COMMAND_F(sbt_roundtimer_sync, "Synchronizes the current CS2 round timer UI.", FCVAR_NONE)
{
	if (args.ArgC() < 4)
	{
		SbtMessage("sbt_roundtimer_sync usage: sbt_roundtimer_sync <totalSeconds> <elapsedSeconds> <currentTime>\n");
		return;
	}

	g_SwiftlyBhopTimerHelper.ApplyRoundTimerSync(
		static_cast<float>(std::atof(args.Arg(1))),
		static_cast<float>(std::atof(args.Arg(2))),
		static_cast<float>(std::atof(args.Arg(3))));
}

CON_COMMAND_F(sbt_htmlhud_fix, "Applies SwiftlyBhopTimer HTML HUD flashing workaround.", FCVAR_NONE)
{
	if (args.ArgC() < 2)
	{
		SbtMessage("sbt_htmlhud_fix usage: sbt_htmlhud_fix <currentTime>\n");
		return;
	}

	g_SwiftlyBhopTimerHelper.ApplyHtmlHudFlashFix(static_cast<float>(std::atof(args.Arg(1))), true);
}

CON_COMMAND_F(sbt_timelimit_init, "Reapplies SwiftlyBhopTimer initial map time limit.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.InitTimeLimit();
	g_SwiftlyBhopTimerHelper.ScheduleInitialTimeLimitApply();
}

CON_COMMAND_F(sbt_timelimit_set, "Sets SwiftlyBhopTimer map time limit in minutes.", FCVAR_NONE)
{
	if (args.ArgC() < 2)
	{
		SbtMessage("sbt_timelimit_set usage: sbt_timelimit_set <minutes>\n");
		return;
	}

	g_SwiftlyBhopTimerHelper.SetTimeLimit(static_cast<float>(std::atof(args.Arg(1))), "command");
}

CON_COMMAND_F(sbt_bhop_physics_status, "Prints disabled status for legacy SwiftlyBhopTimer bhop movement convars.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.PrintBhopPhysicsStatus();
}

CON_COMMAND_F(sbt_bhop_mode, "Sets SwiftlyBhopTimer per-player bhop mode.", FCVAR_NONE)
{
	if (args.ArgC() < 3)
	{
		SbtMessage("sbt_bhop_mode usage: sbt_bhop_mode <slot> <standard|classic>\n");
		return;
	}

	g_SwiftlyBhopTimerHelper.SetBhopMode(std::atoi(args.Arg(1)), args.Arg(2));
}

CON_COMMAND_F(sbt_bhop_mode_status, "Prints SwiftlyBhopTimer per-player bhop mode status.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.PrintBhopModeStatus(args.ArgC() > 1 ? std::atoi(args.Arg(1)) : -1);
}

CON_COMMAND_F(sbt_bhop_helper_defaults, "Applies SwiftlyBhopTimer helper bhop defaults.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.ApplyDefaultBhopHelperSettings(false);
}

CON_COMMAND_F(sbt_hidefps, "Applies SwiftlyBhopTimer first-person viewmodel offsets.", FCVAR_NONE)
{
	if (args.ArgC() < 6)
	{
		SbtMessage("sbt_hidefps usage: sbt_hidefps <slot> <x> <y> <z> <fov>\n");
		return;
	}

	g_SwiftlyBhopTimerHelper.ApplyHideFps(
		std::atoi(args.Arg(1)),
		static_cast<float>(std::atof(args.Arg(2))),
		static_cast<float>(std::atof(args.Arg(3))),
		static_cast<float>(std::atof(args.Arg(4))),
		static_cast<float>(std::atof(args.Arg(5))));
}

CON_COMMAND_F(sbt_hidefps_toggle, "Toggles SwiftlyBhopTimer first-person viewmodel hiding.", FCVAR_NONE)
{
	if (args.ArgC() < 3)
	{
		SbtMessage("sbt_hidefps_toggle usage: sbt_hidefps_toggle <slot> <0|1>\n");
		return;
	}

	g_SwiftlyBhopTimerHelper.SetHideFps(std::atoi(args.Arg(1)), std::atoi(args.Arg(2)) != 0);
}

CON_COMMAND_F(sbt_hidefps_debug, "Prints SwiftlyBhopTimer first-person viewmodel debug information.", FCVAR_NONE)
{
	if (args.ArgC() < 2)
	{
		SbtMessage("sbt_hidefps_debug usage: sbt_hidefps_debug <slot>\n");
		return;
	}

	g_SwiftlyBhopTimerHelper.DebugHideFps(std::atoi(args.Arg(1)));
}

CON_COMMAND_F(sbt_hideweapon, "Toggles SwiftlyBhopTimer first-person weapon transmit hiding.", FCVAR_NONE)
{
	if (args.ArgC() < 3)
	{
		SbtMessage("sbt_hideweapon usage: sbt_hideweapon <slot> <0|1>\n");
		return;
	}

	g_SwiftlyBhopTimerHelper.SetHideWeapon(std::atoi(args.Arg(1)), std::atoi(args.Arg(2)) != 0);
}

CON_COMMAND_F(sbt_change_map, "Changes map through SwiftlyBhopTimer MetaMod helper.", FCVAR_NONE)
{
	if (args.ArgC() < 2)
	{
		SbtMessage("sbt_change_map usage: sbt_change_map <map> [workshopId]\n");
		return;
	}

	g_SwiftlyBhopTimerHelper.RequestMapChange(args.Arg(1), args.ArgC() > 2 ? args.Arg(2) : nullptr);
}

CON_COMMAND_F(sbt_change_map_command, "Executes a configured map change command through SwiftlyBhopTimer MetaMod helper.", FCVAR_NONE)
{
	if (args.ArgC() < 2)
	{
		SbtMessage("sbt_change_map_command usage: sbt_change_map_command <command>\n");
		return;
	}

	g_SwiftlyBhopTimerHelper.RequestMapChangeCommand(args.Arg(1));
}

CON_COMMAND_F(sbt_restart_player, "Queues a SwiftlyBhopTimer restart teleport for a player.", FCVAR_NONE)
{
	if (args.ArgC() < 5)
	{
		SbtMessage("sbt_restart_player usage: sbt_restart_player <slot> <x> <y> <z> [delayFrames]\n");
		return;
	}

	g_SwiftlyBhopTimerHelper.RequestPlayerRestart(
		std::atoi(args.Arg(1)),
		static_cast<float>(std::atof(args.Arg(2))),
		static_cast<float>(std::atof(args.Arg(3))),
		static_cast<float>(std::atof(args.Arg(4))),
		args.ArgC() > 5 ? std::atoi(args.Arg(5)) : 2);
}

CON_COMMAND_F(sbt_restart_player_map_start, "Queues a SwiftlyBhopTimer restart teleport to map_start/map_start1..20.", FCVAR_NONE)
{
	if (args.ArgC() < 3)
	{
		SbtMessage("sbt_restart_player_map_start usage: sbt_restart_player_map_start <slot> <delayFrames> [fallbackX fallbackY fallbackZ]\n");
		return;
	}

	const bool hasFallback = args.ArgC() >= 6;
	g_SwiftlyBhopTimerHelper.RequestPlayerRestartAtMapStart(
		std::atoi(args.Arg(1)),
		std::atoi(args.Arg(2)),
		hasFallback,
		hasFallback ? static_cast<float>(std::atof(args.Arg(3))) : 0.0f,
		hasFallback ? static_cast<float>(std::atof(args.Arg(4))) : 0.0f,
		hasFallback ? static_cast<float>(std::atof(args.Arg(5))) : 0.0f);
}

CON_COMMAND_F(sbt_noclip, "Sets a player's MoveType and ActualMoveType through SwiftlyBhopTimer MetaMod helper.", FCVAR_NONE)
{
	if (args.ArgC() < 3)
	{
		SbtMessage("sbt_noclip usage: sbt_noclip <slot> <0|1>\n");
		return;
	}

	g_SwiftlyBhopTimerHelper.SetPlayerNoclip(std::atoi(args.Arg(1)), ParseCommandBool(args, 2, false));
}

CON_COMMAND_F(sbt_spec_target, "Sets a player's spectator target through SwiftlyBhopTimer MetaMod helper.", FCVAR_NONE)
{
	if (args.ArgC() < 3)
	{
		SbtMessage("sbt_spec_target usage: sbt_spec_target <viewerSlot> <targetSlot>\n");
		return;
	}

	g_SwiftlyBhopTimerHelper.SetSpectateTarget(std::atoi(args.Arg(1)), std::atoi(args.Arg(2)));
}

CON_COMMAND_F(sharptimer_trigger_push_fix, "SharpTimer compatibility: fixes trigger_push acceleration.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.SetTriggerPushFix(ParseCommandBool(args, 1, true));
}

CON_COMMAND_F(sharptimer_disable_telehop, "SharpTimer compatibility: resets speed on trigger_teleport entry.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.SetDisableTelehop(ParseCommandBool(args, 1, true));
}

CON_COMMAND_F(sharptimer_max_bhop_block_time, "SharpTimer compatibility: limits time spent on bhop_block triggers.", FCVAR_NONE)
{
	if (args.ArgC() < 2)
	{
		g_SwiftlyBhopTimerHelper.SetMaxBhopBlockTime(0.0f);
		return;
	}

	g_SwiftlyBhopTimerHelper.SetMaxBhopBlockTime(static_cast<float>(std::atof(args.Arg(1))));
}

CON_COMMAND_F(sharptimer_kill_pointservercommand_entities, "SharpTimer compatibility: removes point_servercommand entities.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.SetKillPointServerCommandEntities(ParseCommandBool(args, 1, false));
}

CON_COMMAND_F(sbt_gimmick_status, "Prints SwiftlyBhopTimer helper gimmick-fix status.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.PrintGimmickStatus();
}

CON_COMMAND_F(sbt_autobhop_force, "Forces CS2 auto-bhop convars while enabled.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.SetForceAutoBhop(ParseCommandBool(args, 1, true));
}

CON_COMMAND_F(sbt_autobhop_status, "Prints SwiftlyBhopTimer auto-bhop force status.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.PrintAutoBhopStatus();
}

CON_COMMAND_F(sbt_surf_fix, "Toggles SwiftlyBhopTimer surf-only movement correction.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.SetSurfFix(ParseCommandBool(args, 1, true));
}

CON_COMMAND_F(sbt_surf_status, "Prints SwiftlyBhopTimer surf-only movement correction status.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.PrintSurfStatus();
}

CON_COMMAND_F(rngfix_downhill, "RNGFix compatibility: toggles downhill incline landing speed guard.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.SetRngFixDownhill(ParseCommandBool(args, 1, true));
}

CON_COMMAND_F(rngfix_uphill, "RNGFix compatibility: sets uphill incline mode (-1, 0, 1).", FCVAR_NONE)
{
	const int mode = args.ArgC() > 1 ? std::atoi(args.Arg(1)) : 1;
	g_SwiftlyBhopTimerHelper.SetRngFixUphill((std::max)(-1, (std::min)(1, mode)));
}

CON_COMMAND_F(rngfix_edge, "RNGFix compatibility: toggles edge-bug landing speed guard.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.SetRngFixEdge(ParseCommandBool(args, 1, true));
}

CON_COMMAND_F(rngfix_triggerjump, "RNGFix compatibility: toggles thin trigger jump assistance.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.SetRngFixTriggerJump(ParseCommandBool(args, 1, true));
}

CON_COMMAND_F(rngfix_telehop, "RNGFix compatibility: toggles conservative telehop speed restore.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.SetRngFixTelehop(ParseCommandBool(args, 1, true));
}

CON_COMMAND_F(rngfix_stairs, "RNGFix compatibility: toggles stair slide mode placeholder.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.SetRngFixStairs(ParseCommandBool(args, 1, true));
}

CON_COMMAND_F(rngfix_useoldslopefixlogic, "RNGFix compatibility: toggles old slopefix landing behavior.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.SetRngFixUseOldSlopeFixLogic(ParseCommandBool(args, 1, false));
}

CON_COMMAND_F(rngfix_debug, "RNGFix compatibility: sets debug level.", FCVAR_NONE)
{
	const int level = args.ArgC() > 1 ? std::atoi(args.Arg(1)) : 1;
	g_SwiftlyBhopTimerHelper.SetRngFixDebug((std::max)(0, (std::min)(2, level)));
}

CON_COMMAND_F(sbt_rngfix_status, "Prints SwiftlyBhopTimer RNGFix compatibility status.", FCVAR_NONE)
{
	g_SwiftlyBhopTimerHelper.PrintRngFixStatus();
}

