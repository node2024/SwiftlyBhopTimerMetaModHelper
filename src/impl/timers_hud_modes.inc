// Time limit, round timer, HTML HUD fix, legacy bhop status and mode commands.
// Included by swiftlybhoptimer_helper.cpp to keep helper features grouped without changing linkage.

void SwiftlyBhopTimerHelper::InitTimeLimit()
{
	InstallTimeLimitMirror();
	SetTimeLimit(DEFAULT_TIMER_CONVAR_MINUTES, "init");
}

void SwiftlyBhopTimerHelper::SetTimeLimit(float minutes, const char *reason)
{
	if (!std::isfinite(minutes) || minutes <= 0.0f)
	{
		SbtMessage("Time limit set skipped: invalid minutes %.3f. Reason=%s\n", minutes, reason ? reason : "unknown");
		return;
	}

	_scheduledTimeLimitMinutes = minutes;
	ApplyMirroredTimeLimit(minutes, reason ? reason : "set");
	ScheduleInitialTimeLimitApply();
}

void SwiftlyBhopTimerHelper::ScheduleInitialTimeLimitApply()
{
	_initialTimeLimitApplyFramesRemaining = 384;
	_initialTimeLimitApplyDelayFrames = 16;
	_initialTimeLimitApplyPasses = 0;
	SbtMessage("Initial time limit delayed apply scheduled.\n");
}

void SwiftlyBhopTimerHelper::ApplyRoundTimerSync(float totalSeconds, float elapsedSeconds, float currentTime)
{
	if (!std::isfinite(totalSeconds) || totalSeconds <= 0.0f)
	{
		SbtMessage("roundtimer sync skipped: invalid totalSeconds=%f\n", totalSeconds);
		return;
	}

	if (!std::isfinite(elapsedSeconds) || elapsedSeconds < 0.0f)
	{
		elapsedSeconds = 0.0f;
	}

	if (!std::isfinite(currentTime) || currentTime < 0.0f)
	{
		currentTime = 0.0f;
	}

	elapsedSeconds = (std::min)(elapsedSeconds, (std::max)(0.0f, totalSeconds - 1.0f));
	const float remainingSeconds = (std::max)(1.0f, totalSeconds - elapsedSeconds);
	const float roundStartTime = currentTime > 0.0f ? currentTime - elapsedSeconds : 0.0f;
	const float roundEndTime = currentTime > 0.0f ? currentTime + remainingSeconds : 0.0f;

	CEntityInstance *gameRulesProxy = FindGameRulesProxy();
	void *gameRules = GetGameRulesPointer(gameRulesProxy);
	if (!gameRules)
	{
		SbtMessage("roundtimer sync skipped: CCSGameRulesProxy/m_pGameRules unavailable\n");
		return;
	}

	int applied = 0;
	applied += SetRawInt32Field(gameRules, gameRulesProxy, "CCSGameRules", "m_iRoundTime", static_cast<int32_t>(std::ceil(totalSeconds))) ? 1 : 0;
	applied += SetRawBoolField(gameRules, gameRulesProxy, "CCSGameRules", "m_bRoundTimeWarningTriggered", false) ? 1 : 0;
	applied += SetRawBoolField(gameRules, gameRulesProxy, "CCSGameRules", "m_bGameRestart", false) ? 1 : 0;

	if (currentTime > 0.0f)
	{
		applied += SetRawFloatField(gameRules, gameRulesProxy, "CCSGameRules", "m_fRoundStartTime", roundStartTime) ? 1 : 0;
		applied += SetRawFloatField(gameRules, gameRulesProxy, "CCSGameRules", "m_flRestartRoundTime", roundEndTime) ? 1 : 0;
		applied += SetRawFloatField(gameRules, gameRulesProxy, "CCSGameRules", "m_timeUntilNextPhaseStarts", roundEndTime) ? 1 : 0;
		applied += SetRawFloatField(gameRules, gameRulesProxy, "CCSGameRules", "m_flGameStartTime", roundStartTime) ? 1 : 0;
		applied += SetRawFloatField(gameRules, gameRulesProxy, "CCSGameRules", "m_fMatchStartTime", roundStartTime) ? 1 : 0;
	}

	NotifyNetworkStateChanged(gameRulesProxy, {});
	SbtMessage("Round timer synced. total=%.3f elapsed=%.3f remaining=%.3f fields=%d\n",
		totalSeconds,
		elapsedSeconds,
		remainingSeconds,
		applied);
}

void SwiftlyBhopTimerHelper::ApplyHtmlHudFlashFix(float currentTime, bool quiet)
{
	if (!std::isfinite(currentTime) || currentTime <= 0.0f)
	{
		if (!quiet)
		{
			SbtMessage("HTML HUD flash fix skipped: invalid currentTime=%f\n", currentTime);
		}
		return;
	}

	CEntityInstance *gameRulesProxy = FindGameRulesProxy();
	void *gameRules = GetGameRulesPointer(gameRulesProxy);
	if (!gameRules)
	{
		if (!quiet)
		{
			SbtMessage("HTML HUD flash fix skipped: CCSGameRulesProxy/m_pGameRules unavailable\n");
		}
		return;
	}

	float restartRoundTime = 0.0f;
	if (!ReadRawFloatField(gameRules, "CCSGameRules", "m_flRestartRoundTime", restartRoundTime))
	{
		if (!quiet)
		{
			SbtMessage("HTML HUD flash fix skipped: m_flRestartRoundTime unavailable\n");
		}
		return;
	}

	if (restartRoundTime < currentTime)
	{
		const bool applied = SetRawBoolField(gameRules, gameRulesProxy, "CCSGameRules", "m_bGameRestart", true);
		if (!quiet)
		{
			SbtMessage("HTML HUD flash fix %s. restart=%.3f current=%.3f\n", applied ? "applied" : "failed", restartRoundTime, currentTime);
		}
	}
}

void SwiftlyBhopTimerHelper::ApplyBhopPhysicsConvars(const char *gravity,
	const char *jumpImpulse,
	const char *airAccelerate,
	const char *airMaxWishSpeed,
	const char *friction)
{
	SbtMessage("Bhop physics helper is disabled. Use cfg/map exec convars instead.\n");
}

void SwiftlyBhopTimerHelper::PrintBhopPhysicsStatus()
{
	SbtMessage("Bhop physics helper is disabled. Runtime movement convars are no longer modified by this helper.\n");
}

void SwiftlyBhopTimerHelper::SetBhopMode(int slot, const char *modeName)
{
	if (slot < 0 || slot >= static_cast<int>(_bhopModeSlots.size()))
	{
		SbtMessage("bhop mode skipped: invalid slot %d\n", slot);
		return;
	}

	int mode = SBT_BHOP_MODE_STANDARD;
	if (modeName && modeName[0])
	{
		if (V_stricmp(modeName, "classic") == 0 || V_stricmp(modeName, "css") == 0)
		{
			mode = SBT_BHOP_MODE_CLASSIC;
		}
		else if (V_stricmp(modeName, "standard") != 0 && V_stricmp(modeName, "std") != 0)
		{
			SbtMessage("bhop mode skipped: unknown mode '%s' for slot %d\n", modeName, slot);
			return;
		}
	}

	_bhopModeSlots[slot] = mode;
	_classicBhopSlots[slot] = ClassicBhopSlotState();
	SbtMessage("bhop mode slot %d = %s\n", slot, mode == SBT_BHOP_MODE_CLASSIC ? "classic" : "standard");
}

void SwiftlyBhopTimerHelper::PrintBhopModeStatus(int slot)
{
	if (slot >= 0 && slot < static_cast<int>(_bhopModeSlots.size()))
	{
		SbtMessage("bhop mode slot %d = %s classic_valid=%d\n",
			slot,
			_bhopModeSlots[slot] == SBT_BHOP_MODE_CLASSIC ? "classic" : "standard",
			_classicBhopSlots[slot].valid ? 1 : 0);
		return;
	}

	int classicCount = 0;
	for (int value : _bhopModeSlots)
	{
		if (value == SBT_BHOP_MODE_CLASSIC)
		{
			classicCount++;
		}
	}

	SbtMessage("bhop mode status: classic_slots=%d standard_slots=%zu\n", classicCount, _bhopModeSlots.size() - classicCount);
}

