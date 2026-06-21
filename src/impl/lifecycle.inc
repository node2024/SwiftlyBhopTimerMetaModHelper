// Plugin lifecycle, map changes, game-frame dispatch.
// Included by swiftlybhoptimer_helper.cpp to keep helper features grouped without changing linkage.

bool SwiftlyBhopTimerHelper::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, g_pEngineServer2, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pNetworkMessages, INetworkMessages, NETWORKMESSAGES_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pSbtGameEventSystem, IGameEventSystem, GAMEEVENTSYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pSbtGameResourceServiceServer, IGameResourceService, GAMERESOURCESERVICESERVER_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, g_pSbtServerGameDll, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_CURRENT(GetServerFactory, g_pSbtSource2GameEntities, ISource2GameEntities, SOURCE2GAMEENTITIES_INTERFACE_VERSION);

	g_SMAPI->AddListener(this, this);
	ConVar_Register();
	InstallTimeLimitMirror();
	if (late)
	{
		InitTimeLimit();
		ScheduleInitialTimeLimitApply();
	}
	if (g_pSbtServerGameDll)
	{
		SH_ADD_HOOK(IServerGameDLL, GameFrame, g_pSbtServerGameDll, SH_MEMBER(this, &SwiftlyBhopTimerHelper::HookGameFrame), true);
	}
	SH_ADD_HOOK(ISource2GameEntities, CheckTransmit, g_pSbtSource2GameEntities, SH_STATIC(SbtHookCheckTransmit), true);
	ResolveCreateBot();
	ResolveSwitchTeam();
	ApplyDefaultBhopHelperSettings(true);

	SbtMessage("Loaded. Late=%d\n", late ? 1 : 0);
	return true;
}

bool SwiftlyBhopTimerHelper::Unload(char *error, size_t maxlen)
{
	if (g_pSbtSource2GameEntities)
	{
		SH_REMOVE_HOOK(ISource2GameEntities, CheckTransmit, g_pSbtSource2GameEntities, SH_STATIC(SbtHookCheckTransmit), true);
	}
	if (g_pSbtServerGameDll)
	{
		SH_REMOVE_HOOK(IServerGameDLL, GameFrame, g_pSbtServerGameDll, SH_MEMBER(this, &SwiftlyBhopTimerHelper::HookGameFrame), true);
	}

	RemoveTimeLimitMirror();
	ConVar_Unregister();
	SbtMessage("Unloaded.\n");
	return true;
}

bool SwiftlyBhopTimerHelper::Pause(char *error, size_t maxlen)
{
	return true;
}

bool SwiftlyBhopTimerHelper::Unpause(char *error, size_t maxlen)
{
	return true;
}

void SwiftlyBhopTimerHelper::AllPluginsLoaded()
{
	SbtMessage("AllPluginsLoaded.\n");
}

void SwiftlyBhopTimerHelper::OnLevelInit(char const *pMapName,
	char const *pMapEntities,
	char const *pOldLevel,
	char const *pLandmarkName,
	bool loadGame,
	bool background)
{
	SbtMessage("OnLevelInit(%s)\n", pMapName ? pMapName : "unknown");
	_replayBots.clear();
	_pendingPlayerRestarts.clear();
	_pendingMapStartRestarts.clear();
	_noclipSlots.fill(false);
	_bhopModeSlots.fill(SBT_BHOP_MODE_STANDARD);
	_classicBhopSlots.fill(ClassicBhopSlotState());
	_priorityTriggerRecords.clear();
	_triggerPushRecords.clear();
	_triggerTeleportRecords.clear();
	_bhopBlockRecords.clear();
	_lastTriggerPushTriggers.fill(CEntityHandle());
	_lastTeleportTriggers.fill(CEntityHandle());
	_lastBhopBlocks.fill(CEntityHandle());
	_bhopBlockTicks.fill(0);
	_gimmickPriorityTicks.fill(0);
	_surfSuppressTicks.fill(0);
	_gimmickSlots.fill(RngFixSlotState());
	_rngFixSlots.fill(RngFixSlotState());
	_surfSlots.fill(RngFixSlotState());
	_rngFixFrame = 0;
	_forceAutoBhop = false;
	_gimmickRescanFrames = 64;
	_gimmickScanWarned = false;
	_movementCompatMaintainDelayFrames = 64;
	InstallTimeLimitMirror();
	InitTimeLimit();
	ScheduleInitialTimeLimitApply();
	ScheduleMovementCompatibilityBurst();
	ApplyRoundFlowConvars();
	ApplyReplayBotConvars();
	ApplyDefaultBhopHelperSettings(false);
}

void SwiftlyBhopTimerHelper::HookGameFrame(bool simulating, bool, bool)
{
	ProcessMovementCompatibilityBurst();
	ProcessMovementCompatibilityMaintenance();

	if (simulating)
	{
		ProcessPendingPlayerRestarts();
		ProcessPendingMapStartRestarts();
		ProcessAutoBhopForce();
		ProcessNoclipPlayers();
		if (_gimmickRescanFrames <= 0)
		{
			RescanGimmickEntities(true);
		}
		else
		{
			_gimmickRescanFrames--;
		}
		ProcessGimmickFixes();
		ProcessSurfPhysics();
		ProcessRngFixes();
		ProcessBhopModes();
	}

	if (_initialTimeLimitApplyFramesRemaining <= 0)
	{
		return;
	}

	_initialTimeLimitApplyFramesRemaining--;
	if (!simulating)
	{
		return;
	}

	if (_initialTimeLimitApplyDelayFrames > 0)
	{
		_initialTimeLimitApplyDelayFrames--;
		return;
	}

	_initialTimeLimitApplyPasses++;
	ApplyMirroredTimeLimit(_scheduledTimeLimitMinutes, "delayed-init");

	if (_initialTimeLimitApplyPasses >= 5)
	{
		_initialTimeLimitApplyFramesRemaining = 0;
		return;
	}

	_initialTimeLimitApplyDelayFrames = 64;
}

void SwiftlyBhopTimerHelper::RequestMapChange(const char *mapName, const char *workshopId)
{
	if (!IsSafeMapChangeArg(mapName))
	{
		SbtMessage("map change skipped: invalid map name\n");
		return;
	}

	char command[256] = {};
	if (workshopId && workshopId[0])
	{
		if (!IsSafeMapChangeArg(workshopId))
		{
			SbtMessage("map change skipped: invalid workshop id for map %s\n", mapName);
			return;
		}

		V_snprintf(command, sizeof(command), "host_workshop_map %s", workshopId);
		SbtMessage("map change requested: map=%s workshop=%s\n", mapName, workshopId);
	}
	else
	{
		V_snprintf(command, sizeof(command), "changelevel %s", mapName);
		SbtMessage("map change requested: map=%s\n", mapName);
	}

	ServerCommand(command);
}

void SwiftlyBhopTimerHelper::RequestMapChangeCommand(const char *command)
{
	if (!IsSafeMapChangeArg(command))
	{
		SbtMessage("map change command skipped: invalid command\n");
		return;
	}

	SbtMessage("map change command requested: %s\n", command);
	ServerCommand(command);
}

