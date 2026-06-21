// Direct CreateBot replay bot implementation and pruning.
// Included by swiftlybhoptimer_helper.cpp to keep helper features grouped without changing linkage.

bool SwiftlyBhopTimerHelper::ResolveCreateBot()
{
	if (_createBot)
	{
		return true;
	}

	_createBot = reinterpret_cast<CreateBot_t *>(FindCreateBotSignature());
	if (!_createBot)
	{
		SbtMessage("CreateBot signature was not found. Replay bot creation is unavailable.\n");
		return false;
	}

	SbtMessage("CreateBot signature resolved at %p.\n", reinterpret_cast<void *>(_createBot));
	return true;
}

bool SwiftlyBhopTimerHelper::ResolveSwitchTeam()
{
	if (_switchTeam)
	{
		return true;
	}

	_switchTeam = reinterpret_cast<SwitchTeam_t *>(FindSwitchTeamSignature());
	if (!_switchTeam)
	{
		SbtMessage("SwitchTeam signature was not found. Replay bot team finalization will use schema fallback.\n");
		return false;
	}

	SbtMessage("SwitchTeam signature resolved at %p.\n", reinterpret_cast<void *>(_switchTeam));
	return true;
}

CCSPlayerController *SwiftlyBhopTimerHelper::CreateReplayBot(const char *displayName, bool oneShot)
{
	if (!ResolveCreateBot())
	{
		return nullptr;
	}

	// Match cs2kz's safer path: create the fake client on T first, then move it
	// through the controller team code. Creating directly as CT can leave later
	// replay bots half-attached on some workshop maps.
	SbtBotProfile *profile = SbtBotProfile::PersistentProfile(CS_TEAM_T, displayName);
	CCSPlayerController *bot = _createBot(profile, CS_TEAM_T);
	if (!bot)
	{
		SbtMessage("CreateBot returned null.\n");
		return nullptr;
	}

	ReplayBotRecord record;
	record.controller = bot;
	record.name = displayName && displayName[0] ? displayName : "SwiftlyBhopTimerReplay";
	record.slot = FindControllerSlot(reinterpret_cast<CEntityInstance *>(bot));
	record.oneShot = oneShot;
	record.persistent = IsPersistentReplayBotName(record.name.c_str());
	FinalizeReplayBotRecord(record, false);
	_replayBots.push_back(record);
	SbtMessage("Replay bot created via direct CreateBot at %p. Slot=%d Count=%zu OneShot=%d Persistent=%d\n",
		bot,
		record.slot,
		_replayBots.size(),
		oneShot ? 1 : 0,
		record.persistent ? 1 : 0);
	return bot;
}

CEntityInstance *SwiftlyBhopTimerHelper::ResolveReplayBotController(ReplayBotRecord &record)
{
	if (!record.name.empty() && record.slot >= 0 && record.slot < 64)
	{
		CEntityInstance *controller = GetControllerBySlot(record.slot);
		if (ControllerNameMatches(controller, record.name))
		{
			record.controller = reinterpret_cast<CCSPlayerController *>(controller);
			return controller;
		}
	}

	if (record.name.empty())
	{
		return nullptr;
	}

	for (int slot = 0; slot < 64; slot++)
	{
		CEntityInstance *controller = GetControllerBySlot(slot);
		if (ControllerNameMatches(controller, record.name))
		{
			record.slot = slot;
			record.controller = reinterpret_cast<CCSPlayerController *>(controller);
			return controller;
		}
	}

	record.slot = -1;
	record.controller = nullptr;
	return nullptr;
}

bool SwiftlyBhopTimerHelper::IsReplayBotController(CEntityInstance *controller)
{
	if (!controller)
	{
		return false;
	}

	const int slot = FindControllerSlot(controller);
	for (ReplayBotRecord &record : _replayBots)
	{
		if (reinterpret_cast<CEntityInstance *>(record.controller) == controller)
		{
			record.slot = slot;
			return true;
		}

		if (ControllerNameMatches(controller, record.name))
		{
			record.slot = slot;
			record.controller = reinterpret_cast<CCSPlayerController *>(controller);
			return true;
		}
	}

	return false;
}

bool SwiftlyBhopTimerHelper::FinalizeReplayBotRecord(ReplayBotRecord &record, bool quiet)
{
	if (!record.controller)
	{
		return false;
	}

	auto *controller = reinterpret_cast<CEntityInstance *>(record.controller);
	record.slot = FindControllerSlot(controller);

	bool applied = false;
	applied |= SetControllerPlayerName(controller, record.name.c_str());
	applied |= SetControllerConnectedState(controller);

	if (!record.teamFinalized && ResolveSwitchTeam())
	{
		_switchTeam(record.controller, CS_TEAM_CT);
		record.teamFinalized = true;
		applied = true;
	}
	else if (!record.teamFinalized)
	{
		if (SetEntityInt32Field(controller, "CCSPlayerController", "m_iTeamNum", CS_TEAM_CT))
		{
			record.teamFinalized = true;
			applied = true;
		}
	}

	// CS2 can auto-kick bots that do not have spectator as pending team when
	// real players join. cs2kz applies the same pending-team guard.
	applied |= SetEntityUInt8Field(controller, "CCSPlayerController", "m_iPendingTeamNum", 1);

	CEntityInstance *pawn = GetPlayerPawnFromController(controller);
	if (pawn)
	{
		applied |= SetEntityInt32Field(pawn, "CCSPlayerPawn", "m_iTeamNum", CS_TEAM_CT);
	}

	if (!quiet)
	{
		SbtMessage("Replay bot finalized. Name=%s Slot=%d Applied=%d\n", record.name.c_str(), record.slot, applied ? 1 : 0);
	}

	return applied;
}

void SwiftlyBhopTimerHelper::PruneStaleReplayBotRecords(bool quiet)
{
	for (size_t i = 0; i < _replayBots.size();)
	{
		if (ResolveReplayBotController(_replayBots[i]))
		{
			i++;
			continue;
		}

		if (!quiet)
		{
			SbtMessage("Pruning stale replay bot record: %s\n", _replayBots[i].name.c_str());
		}

		_replayBots.erase(_replayBots.begin() + i);
	}
}

bool SwiftlyBhopTimerHelper::PruneReplayBotsForNewBot()
{
	PruneStaleReplayBotRecords(true);

	while (_replayBots.size() >= MAX_REPLAY_BOTS)
	{
		auto victim = std::find_if(_replayBots.begin(), _replayBots.end(), [](const ReplayBotRecord &record) {
			return !record.persistent;
		});

		if (victim == _replayBots.end())
		{
			SbtMessage("Replay bot quota is full and no non-persistent replay bot can be pruned.\n");
			return false;
		}

		size_t index = static_cast<size_t>(std::distance(_replayBots.begin(), victim));
		SbtMessage("Pruning replay bot to keep quota %zu: %s\n", MAX_REPLAY_BOTS, _replayBots[index].name.c_str());
		RemoveReplayBotAt(index, true);
	}

	return true;
}

bool SwiftlyBhopTimerHelper::HasReplayBotName(const char *displayName) const
{
	if (!displayName || !displayName[0])
	{
		return false;
	}

	return std::any_of(_replayBots.begin(), _replayBots.end(), [displayName](const ReplayBotRecord &record) {
		return record.name == displayName;
	});
}

bool SwiftlyBhopTimerHelper::ReplayBotNameMatches(const std::string &recordName, const char *displayName) const
{
	if (!displayName || !displayName[0])
	{
		return false;
	}

	if (recordName == displayName)
	{
		return true;
	}

	std::string requested(displayName);
	auto requestedBracket = requested.find(']');
	if (requestedBracket == std::string::npos || requestedBracket == 0)
	{
		return false;
	}

	auto recordBracket = recordName.find(']');
	if (recordBracket == std::string::npos || recordBracket == 0)
	{
		return false;
	}

	return recordName.compare(0, recordBracket + 1, requested, 0, requestedBracket + 1) == 0;
}

bool SwiftlyBhopTimerHelper::RemoveReplayBotByName(const char *displayName, bool kick)
{
	if (!displayName || !displayName[0])
	{
		return false;
	}

	bool removed = false;
	for (size_t i = 0; i < _replayBots.size();)
	{
		if (_replayBots[i].name == displayName)
		{
			RemoveReplayBotAt(i, kick);
			removed = true;
			continue;
		}

		i++;
	}

	if (kick && !removed)
	{
		KickReplayBotNameOrTag(displayName);
	}

	return removed;
}

void SwiftlyBhopTimerHelper::RemovePersistentReplayBotsForReplacement(const char *displayName)
{
	if (!displayName || !displayName[0] || !IsPersistentReplayBotName(displayName))
	{
		return;
	}

	auto replacementKey = GetPersistentReplayReplacementKey(displayName);
	if (replacementKey.empty())
	{
		return;
	}

	for (size_t i = 0; i < _replayBots.size();)
	{
		auto existingKey = GetPersistentReplayReplacementKey(_replayBots[i].name.c_str());
		if (_replayBots[i].persistent && existingKey == replacementKey && _replayBots[i].name != displayName)
		{
			SbtMessage("Replacing persistent replay bot: %s -> %s\n", _replayBots[i].name.c_str(), displayName);
			RemoveReplayBotAt(i, true);
			continue;
		}

		i++;
	}
}

bool SwiftlyBhopTimerHelper::RemoveReplayBotByNameOrTag(const char *displayName, bool kick)
{
	if (!displayName || !displayName[0])
	{
		return false;
	}

	for (size_t i = 0; i < _replayBots.size();)
	{
		if (!_replayBots[i].persistent && ReplayBotNameMatches(_replayBots[i].name, displayName))
		{
			RemoveReplayBotAt(i, kick);
			return true;
		}

		i++;
	}

	return false;
}

void SwiftlyBhopTimerHelper::RemoveReplayBotAt(size_t index, bool kick)
{
	if (index >= _replayBots.size())
	{
		return;
	}

	std::string botName = _replayBots[index].name;
	bool persistent = _replayBots[index].persistent;
	_replayBots.erase(_replayBots.begin() + index);
	if (kick)
	{
		if (persistent)
		{
			KickReplayBotName(botName.c_str());
		}
		else
		{
			KickReplayBotNameOrTag(botName.c_str());
		}
	}
}

void SwiftlyBhopTimerHelper::KickReplayBotName(const char *displayName)
{
	if (!displayName || !displayName[0])
	{
		return;
	}

	char command[192];
	V_snprintf(command, sizeof(command), "bot_kick \"%s\"", displayName);
	ServerCommand(command);
}

void SwiftlyBhopTimerHelper::KickReplayBotNameOrTag(const char *displayName)
{
	if (IsProtectedReplayBotKickTarget(displayName))
	{
		SbtMessage("Replay bot kick fallback skipped for protected target: %s\n", displayName && displayName[0] ? displayName : "empty");
		return;
	}

	KickReplayBotName(displayName);

	auto tag = GetReplayBotNameTag(displayName);
	if (!tag.empty() && tag != displayName && !IsProtectedReplayBotKickTarget(tag.c_str()))
	{
		KickReplayBotName(tag.c_str());
	}
}

