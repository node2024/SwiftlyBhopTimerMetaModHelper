// Replay bot command handlers and replay convars.
// Included by swiftlybhoptimer_helper.cpp to keep helper features grouped without changing linkage.

void SwiftlyBhopTimerHelper::RequestReplayBotAdd(const char *reason, const char *displayName, bool oneShot)
{
	char botName[128] = {};
	if (displayName && displayName[0])
	{
		V_strncpy(botName, displayName, sizeof(botName));
	}
	else
	{
		V_strncpy(botName, "SwiftlyBhopTimerReplay", sizeof(botName));
	}

	SbtMessage("Replay bot add requested. Reason=%s Name=%s OneShot=%d\n", reason ? reason : "unknown", botName, oneShot ? 1 : 0);
	ApplyReplayBotConvars();
	PruneStaleReplayBotRecords(true);
	if (HasReplayBotName(botName))
	{
		if (!oneShot)
		{
			for (ReplayBotRecord &record : _replayBots)
			{
				if (record.name == botName)
				{
					FinalizeReplayBotRecord(record, true);
					ApplyReplayBotPlaybackFixesToRecord(record);
					SbtMessage("Replay bot add request kept existing persistent bot: %s\n", botName);
					return;
				}
			}

			SbtMessage("Replay bot add found existing persistent name but no live record was refreshed: %s\n", botName);
			return;
		}
		else
		{
			SbtMessage("Replay bot add is replacing existing one-shot bot: %s\n", botName);
			RemoveReplayBotByName(botName, true);
		}
	}
	else if (!oneShot && IsPersistentReplayBotName(botName))
	{
		RemovePersistentReplayBotsForReplacement(botName);
	}

	if (!PruneReplayBotsForNewBot())
	{
		SbtMessage("Replay bot add aborted because the helper quota is full.\n");
		return;
	}

	if (CreateReplayBot(botName, oneShot))
	{
		ApplyReplayBotConvars(true);
		ApplyReplayBotPlaybackFixes(true);
		return;
	}

	SbtMessage("Direct CreateBot failed. No bot_add fallback is used; replay bots are managed only by this helper.\n");
}

void SwiftlyBhopTimerHelper::RequestReplayBotKick(const char *reason, const char *displayName)
{
	ApplyReplayBotConvars(true);
	SbtMessage("Replay bot kick requested. Reason=%s\n", reason ? reason : "unknown");
	if (displayName && displayName[0])
	{
		if (!RemoveReplayBotByName(displayName, true) && IsPersistentReplayBotName(displayName))
		{
			SbtMessage("Replay bot kick fallback exact persistent name: %s\n", displayName);
			KickReplayBotName(displayName);
		}

		return;
	}

	while (!_replayBots.empty())
	{
		RemoveReplayBotAt(_replayBots.size() - 1, true);
	}

	ServerCommand("bot_kick SwiftlyBhopTimerReplay");
	ServerCommand("bot_kick SBT_SR");
	ServerCommand("bot_kick SBT_PB_");
	ServerCommand("bot_kick SBT_TOP_");
	ServerCommand("bot_kick \"[SR]\"");
	ServerCommand("bot_kick \"[SR-ST]\"");
	ServerCommand("bot_kick \"[SR-CL]\"");
	ServerCommand("bot_kick \"[PB]\"");
	ServerCommand("bot_kick \"[1]\"");
	ServerCommand("bot_kick \"[2]\"");
	ServerCommand("bot_kick \"[3]\"");
	ServerCommand("bot_kick \"[SR Replay]\"");
	ServerCommand("bot_kick \"[PB Replay]\"");
	ServerCommand("bot_kick \"[Top Replay]\"");
}

void SwiftlyBhopTimerHelper::RequestReplayBotComplete(const char *reason, const char *displayName)
{
	ApplyReplayBotConvars(true);
	SbtMessage("Replay bot complete requested. Reason=%s Name=%s\n", reason ? reason : "unknown", displayName && displayName[0] ? displayName : "all");
	if (!displayName || !displayName[0])
	{
		return;
	}

	bool removed = false;
	bool protectedMatch = false;
	for (size_t i = 0; i < _replayBots.size();)
	{
		if (ReplayBotNameMatches(_replayBots[i].name, displayName))
		{
			if (!_replayBots[i].persistent)
			{
				RemoveReplayBotAt(i, true);
				removed = true;
				continue;
			}
			else
			{
				protectedMatch = true;
			}
		}

		i++;
	}

	if (removed)
	{
		return;
	}

	if (protectedMatch)
	{
		SbtMessage("Replay bot complete ignored for persistent/loop bot: %s\n", displayName);
		return;
	}

	if (RemoveReplayBotByNameOrTag(displayName, true))
	{
		return;
	}

	if (!IsProtectedReplayBotKickTarget(displayName))
	{
		SbtMessage("Replay bot complete fallback kicking unregistered one-shot bot: %s\n", displayName);
		KickReplayBotNameOrTag(displayName);
		return;
	}

	SbtMessage("Replay bot complete ignored; bot is not registered or is protected: %s\n", displayName);
}

void SwiftlyBhopTimerHelper::ApplyReplayBotConvars(bool quiet)
{
	SetConVar("bot_quota_mode", "normal", quiet);
	SetConVar("bot_quota", "5", quiet);
	SetConVar("bot_prefix", "", quiet);
	SetConVar("bot_join_after_player", "0", quiet);
	SetConVar("bot_auto_vacate", "0", quiet);
	SetConVar("bot_controllable", "0", quiet);
	SetConVar("bot_stop", "0", quiet);
	SetConVar("bot_freeze", "0", quiet);
	SetConVar("bot_zombie", "0", quiet);
	SetConVar("bot_mimic", "0", quiet);
	SetConVar("bot_dont_shoot", "1", quiet);
	SetConVar("mp_limitteams", "0", quiet);
	SetConVar("mp_autoteambalance", "0", quiet);
	SetConVar("mp_humanteam", "any", quiet);
	SetConVar("mp_force_pick_time", "0", quiet);
	SetConVar("mp_ignore_round_win_conditions", "1", quiet);
	SetConVar("mp_disconnect_kills_bots", "0", quiet);
	SetConVar("mp_disconnect_kills_players", "0", quiet);
	SetConVar("mp_warmup_pausetimer", "0", quiet);
	SetConVar("mp_warmup_offline_enabled", "0", quiet);
	SetConVar("mp_warmup_online_enabled", "0", quiet);
	SetConVar("mp_warmuptime", "0", quiet);
	ServerCommand("mp_unpause_match", quiet);
	ServerCommand("mp_warmup_end", quiet);

	if (!quiet)
	{
		SbtMessage("Replay bot convars applied.\n");
	}
}

void SwiftlyBhopTimerHelper::ApplyReplayBotPlaybackFixes(bool quiet)
{
	size_t appliedCount = 0;
	for (ReplayBotRecord &record : _replayBots)
	{
		if (ApplyReplayBotPlaybackFixesToRecord(record))
		{
			appliedCount++;
		}
	}

	if (!quiet)
	{
		SbtMessage("Replay bot playback flags applied. Bots=%zu Applied=%zu\n", _replayBots.size(), appliedCount);
	}
}

bool SwiftlyBhopTimerHelper::ApplyReplayBotPlaybackFixesToRecord(ReplayBotRecord &record)
{
	CEntityInstance *controller = ResolveReplayBotController(record);
	if (!controller)
	{
		return false;
	}

	CEntityInstance *pawn = GetPlayerPawnFromController(controller);
	if (!pawn)
	{
		return false;
	}

	bool applied = false;
	applied |= SetEntityBoolField(pawn, "CCSPlayerPawn", "m_bSimulatedEveryTick", true);
	applied |= SetEntityBoolField(pawn, "CCSPlayerPawn", "m_bAnimatedEveryTick", true);
	applied |= SetEntityBoolField(pawn, "CBaseEntity", "m_bSimulatedEveryTick", true);
	applied |= SetEntityBoolField(pawn, "CBaseEntity", "m_bAnimatedEveryTick", true);
	return applied;
}

