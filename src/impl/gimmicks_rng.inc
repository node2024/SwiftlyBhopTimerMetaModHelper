// Bhop helper defaults, gimmick scanners, RNGFix, surf helpers.
// Included by swiftlybhoptimer_helper.cpp to keep helper features grouped without changing linkage.

void SwiftlyBhopTimerHelper::ApplyDefaultBhopHelperSettings(bool quiet)
{
	_forceAutoBhop = true;
	_triggerPushFixEnabled = true;
	_disableTelehop = false;
	_maxBhopBlockTicks = 64;
	_killPointServerCommandEntities = false;
	_rngFixDownhill = true;
	_rngFixUphill = 1;
	_rngFixEdge = true;
	_rngFixTriggerJump = true;
	_rngFixTelehop = true;
	_rngFixStairs = true;
	_rngFixUseOldSlopeFixLogic = true;
	_surfFixEnabled = true;
	_rngFixDebug = 0;

	_lastTriggerPushTriggers.fill(CEntityHandle());
	_lastTeleportTriggers.fill(CEntityHandle());
	_lastBhopBlocks.fill(CEntityHandle());
	_bhopBlockTicks.fill(0);
	_surfSuppressTicks.fill(0);
	_gimmickSlots.fill(RngFixSlotState());
	_rngFixSlots.fill(RngFixSlotState());
	_surfSlots.fill(RngFixSlotState());

	ApplyAutoBhopConvars(true);
	if (_killPointServerCommandEntities)
	{
		KillPointServerCommandEntities(true);
	}
	RescanGimmickEntities(true);
	_gimmickRescanFrames = 64;

	if (!quiet)
	{
		SbtMessage("Default bhop helper settings applied: trigger_push=1 telehop_rngfix=1 bhop_block_ticks=%d rngfix=1 autobhop=1\n",
			_maxBhopBlockTicks);
	}
}

void SwiftlyBhopTimerHelper::SetTriggerPushFix(bool enabled)
{
	if (_triggerPushFixEnabled == enabled)
	{
		if (enabled)
		{
			RescanGimmickEntities(true);
		}
		return;
	}

	if (!enabled)
	{
		CGameEntitySystem *entitySystem = GetGameEntitySystem();
		for (const TriggerPushRecord &record : _triggerPushRecords)
		{
			if (!record.speedZeroed || !entitySystem)
			{
				continue;
			}

			CEntityInstance *entity = GetEntityInstanceByHandle(entitySystem, record.bounds.handle);
			SetEntityFloatAny(entity, { "CTriggerPush", "CBaseTrigger", "CBaseEntity" }, { "m_flSpeed", "m_speed", "m_flPushSpeed" }, record.speed);
		}
	}

	_triggerPushFixEnabled = enabled;
	_lastTriggerPushTriggers.fill(CEntityHandle());
	if (enabled)
	{
		RescanGimmickEntities(false);
	}
	else if (_rngFixTriggerJump)
	{
		RescanGimmickEntities(false);
	}
	else
	{
		_triggerPushRecords.clear();
	}

	SbtMessage("trigger_push fix %s\n", enabled ? "enabled" : "disabled");
}

void SwiftlyBhopTimerHelper::SetDisableTelehop(bool enabled)
{
	_disableTelehop = enabled;
	if (enabled && _rngFixTelehop)
	{
		_rngFixTelehop = false;
		SbtMessage("rngfix_telehop disabled because sharptimer_disable_telehop was enabled\n");
	}

	_lastTeleportTriggers.fill(CEntityHandle());
	if (enabled)
	{
		RescanGimmickEntities(false);
	}

	SbtMessage("telehop speed reset %s\n", enabled ? "enabled" : "disabled");
}

void SwiftlyBhopTimerHelper::SetMaxBhopBlockTime(float seconds)
{
	if (!std::isfinite(seconds) || seconds <= 0.0f)
	{
		_maxBhopBlockTicks = 0;
		_bhopBlockRecords.clear();
		_bhopBlockTicks.fill(0);
		_lastBhopBlocks.fill(CEntityHandle());
		SbtMessage("bhop block time limit disabled\n");
		return;
	}

	_maxBhopBlockTicks = (std::max)(1, static_cast<int>(std::ceil(seconds * 64.0f)));
	_bhopBlockTicks.fill(0);
	_lastBhopBlocks.fill(CEntityHandle());
	RescanGimmickEntities(false);
	SbtMessage("bhop block time limit set: %.3f seconds (%d ticks)\n", seconds, _maxBhopBlockTicks);
}

void SwiftlyBhopTimerHelper::SetKillPointServerCommandEntities(bool enabled)
{
	_killPointServerCommandEntities = enabled;
	SbtMessage("point_servercommand removal %s\n", enabled ? "enabled" : "disabled");
	if (enabled)
	{
		KillPointServerCommandEntities(false);
	}
}

void SwiftlyBhopTimerHelper::PrintGimmickStatus()
{
	SbtMessage("gimmick status: trigger_push=%d records=%zu skipped_bounds=%d skipped_speed=%d skipped_direction=%d telehop=%d teleports=%zu bhop_block_ticks=%d blocks=%zu kill_pointservercommand=%d\n",
		_triggerPushFixEnabled ? 1 : 0,
		_triggerPushRecords.size(),
		_lastTriggerPushSkippedNoBounds,
		_lastTriggerPushSkippedNoSpeed,
		_lastTriggerPushSkippedNoDirection,
		_disableTelehop ? 1 : 0,
		_triggerTeleportRecords.size(),
		_maxBhopBlockTicks,
		_bhopBlockRecords.size(),
		_killPointServerCommandEntities ? 1 : 0);
}

void SwiftlyBhopTimerHelper::SetRngFixDownhill(bool enabled)
{
	_rngFixDownhill = enabled;
	SbtMessage("rngfix_downhill %s\n", enabled ? "enabled" : "disabled");
}

void SwiftlyBhopTimerHelper::SetRngFixUphill(int mode)
{
	_rngFixUphill = (std::max)(-1, (std::min)(1, mode));
	SbtMessage("rngfix_uphill set: %d\n", _rngFixUphill);
}

void SwiftlyBhopTimerHelper::SetRngFixEdge(bool enabled)
{
	_rngFixEdge = enabled;
	SbtMessage("rngfix_edge %s\n", enabled ? "enabled" : "disabled");
}

void SwiftlyBhopTimerHelper::SetRngFixTriggerJump(bool enabled)
{
	_rngFixTriggerJump = enabled;
	if (enabled)
	{
		RescanGimmickEntities(false);
	}

	SbtMessage("rngfix_triggerjump %s\n", enabled ? "enabled" : "disabled");
}

void SwiftlyBhopTimerHelper::SetRngFixTelehop(bool enabled)
{
	_rngFixTelehop = enabled;
	if (enabled && _disableTelehop)
	{
		_disableTelehop = false;
		_lastTeleportTriggers.fill(CEntityHandle());
		SbtMessage("telehop speed reset disabled because rngfix_telehop was enabled\n");
	}

	if (enabled)
	{
		_gimmickSlots.fill(RngFixSlotState());
		_rngFixSlots.fill(RngFixSlotState());
		_surfSlots.fill(RngFixSlotState());
		_surfSuppressTicks.fill(0);
		_lastTeleportTriggers.fill(CEntityHandle());
		RescanGimmickEntities(false);
	}

	SbtMessage("rngfix_telehop %s\n", enabled ? "enabled" : "disabled");
}

void SwiftlyBhopTimerHelper::SetRngFixStairs(bool enabled)
{
	_rngFixStairs = enabled;
	SbtMessage("rngfix_stairs %s (CS2 helper keeps this as a no-op safety toggle)\n", enabled ? "enabled" : "disabled");
}

void SwiftlyBhopTimerHelper::SetRngFixUseOldSlopeFixLogic(bool enabled)
{
	_rngFixUseOldSlopeFixLogic = enabled;
	SbtMessage("rngfix_useoldslopefixlogic %s\n", enabled ? "enabled" : "disabled");
}

void SwiftlyBhopTimerHelper::SetRngFixDebug(int level)
{
	_rngFixDebug = (std::max)(0, (std::min)(2, level));
	SbtMessage("rngfix_debug set: %d\n", _rngFixDebug);
}

void SwiftlyBhopTimerHelper::PrintRngFixStatus()
{
	SbtMessage("rngfix status: downhill=%d uphill=%d edge=%d triggerjump=%d telehop=%d stairs=%d old_slope=%d debug=%d frame=%d\n",
		_rngFixDownhill ? 1 : 0,
		_rngFixUphill,
		_rngFixEdge ? 1 : 0,
		_rngFixTriggerJump ? 1 : 0,
		_rngFixTelehop ? 1 : 0,
		_rngFixStairs ? 1 : 0,
		_rngFixUseOldSlopeFixLogic ? 1 : 0,
		_rngFixDebug,
		_rngFixFrame);
}

void SwiftlyBhopTimerHelper::SetSurfFix(bool enabled)
{
	_surfFixEnabled = enabled;
	_surfSuppressTicks.fill(0);
	_gimmickSlots.fill(RngFixSlotState());
	_surfSlots.fill(RngFixSlotState());
	SbtMessage("surf fix %s\n", enabled ? "enabled" : "disabled");
}

void SwiftlyBhopTimerHelper::PrintSurfStatus()
{
	SbtMessage("surf status: enabled=%d suppress_ticks=%d max_z_gain=%.2f max_extra_gravity=%.2f\n",
		_surfFixEnabled ? 1 : 0,
		SBT_SURF_SUPPRESS_TICKS,
		SBT_SURF_MAX_Z_GAIN_PER_TICK,
		SBT_SURF_MAX_EXTRA_ABOVE_GRAVITY);
}

void SwiftlyBhopTimerHelper::RescanGimmickEntities(bool quiet)
{
	_gimmickRescanFrames = 320;

	if (!_triggerPushFixEnabled && !_rngFixTriggerJump && !_disableTelehop && !_rngFixTelehop && _maxBhopBlockTicks <= 0 && !_killPointServerCommandEntities)
	{
		_priorityTriggerRecords.clear();
		_triggerPushRecords.clear();
		_triggerTeleportRecords.clear();
		_bhopBlockRecords.clear();
		return;
	}

	CGameEntitySystem *entitySystem = GetGameEntitySystem();
	if (!entitySystem)
	{
		if (!_gimmickScanWarned)
		{
			SbtMessage("gimmick scan skipped: entity system unavailable\n");
			_gimmickScanWarned = true;
		}
		return;
	}

	std::vector<TriggerPushRecord> previousPushRecords = _triggerPushRecords;
	_priorityTriggerRecords.clear();
	_triggerPushRecords.clear();
	_triggerTeleportRecords.clear();
	_bhopBlockRecords.clear();

	auto toGimmickVector = [](const SbtVector3 &value) {
		GimmickVector result;
		result.x = value.x;
		result.y = value.y;
		result.z = value.z;
		return result;
	};

	_lastTriggerPushSkippedNoBounds = 0;
	_lastTriggerPushSkippedNoSpeed = 0;
	_lastTriggerPushSkippedNoDirection = 0;

	auto findPreviousPushSpeed = [&previousPushRecords](const CEntityHandle &handle, float &speed) {
		for (const TriggerPushRecord &record : previousPushRecords)
		{
			if (EntityHandleEquals(record.bounds.handle, handle) && record.speed > 0.0f)
			{
				speed = record.speed;
				return true;
			}
		}

		return false;
	};

	for (int entityIndex = 1; entityIndex < MAX_TOTAL_ENTITIES; entityIndex++)
	{
		CEntityInstance *entity = GetEntityInstanceByIndex(entitySystem, entityIndex);
		if (!entity)
		{
			continue;
		}

		const char *className = entity->GetClassname();
		if (!className || !className[0])
		{
			continue;
		}

		const char *targetName = GetEntityTargetName(entity);
		SbtVector3 mins {};
		SbtVector3 maxs {};
		const bool hasBounds = GetEntityWorldBounds(entity, mins, maxs);

		if (hasBounds && ContainsCaseInsensitive(className, "trigger"))
		{
			GimmickBounds bounds;
			bounds.handle = entity->GetRefEHandle();
			bounds.entityIndex = entityIndex;
			bounds.mins = toGimmickVector(mins);
			bounds.maxs = toGimmickVector(maxs);
			bounds.name = targetName ? targetName : "";
			_priorityTriggerRecords.push_back(bounds);
		}

		if ((_triggerPushFixEnabled || _rngFixTriggerJump) && ContainsCaseInsensitive(className, "trigger_push"))
		{
			if (!hasBounds)
			{
				_lastTriggerPushSkippedNoBounds++;
				continue;
			}

			float speed = 0.0f;
			SbtVector3 direction {};
			const bool hasSpeed = ReadEntityFloatAny(entity, { "CTriggerPush", "CBaseTrigger", "CBaseEntity" }, { "m_flSpeed", "m_speed", "m_flPushSpeed" }, speed);
			float previousSpeed = 0.0f;
			const bool hasPreviousSpeed = findPreviousPushSpeed(entity->GetRefEHandle(), previousSpeed);
			if (hasPreviousSpeed)
			{
				speed = previousSpeed;
			}

			if ((!hasSpeed && !hasPreviousSpeed) || speed <= 0.0f)
			{
				_lastTriggerPushSkippedNoSpeed++;
				continue;
			}

			if (!ReadTriggerPushDirection(entity, direction))
			{
				_lastTriggerPushSkippedNoDirection++;
				continue;
			}

			{
				TriggerPushRecord record;
				record.bounds.handle = entity->GetRefEHandle();
				record.bounds.entityIndex = entityIndex;
				record.bounds.mins = toGimmickVector(mins);
				record.bounds.maxs = toGimmickVector(maxs);
				record.bounds.name = targetName;
				record.speed = speed;
				record.direction = toGimmickVector(direction);
				record.speedZeroed = _triggerPushFixEnabled
					&& SetEntityFloatAny(entity, { "CTriggerPush", "CBaseTrigger", "CBaseEntity" }, { "m_flSpeed", "m_speed", "m_flPushSpeed" }, 0.0f);
				_triggerPushRecords.push_back(record);
			}
		}

		if ((_disableTelehop || _rngFixTelehop) && ContainsCaseInsensitive(className, "trigger_teleport") && hasBounds)
		{
			GimmickBounds bounds;
			bounds.handle = entity->GetRefEHandle();
			bounds.entityIndex = entityIndex;
			bounds.mins = toGimmickVector(mins);
			bounds.maxs = toGimmickVector(maxs);
			bounds.name = targetName;
			_triggerTeleportRecords.push_back(bounds);
		}

		if (_maxBhopBlockTicks > 0
			&& hasBounds
			&& (ContainsCaseInsensitive(className, "trigger_multiple") || ContainsCaseInsensitive(className, "trigger_once") || ContainsCaseInsensitive(className, "trigger"))
			&& (ContainsCaseInsensitive(targetName, "bhop_block") || ContainsCaseInsensitive(targetName, "bhopblock")))
		{
			GimmickBounds bounds;
			bounds.handle = entity->GetRefEHandle();
			bounds.entityIndex = entityIndex;
			bounds.mins = toGimmickVector(mins);
			bounds.maxs = toGimmickVector(maxs);
			bounds.name = targetName;
			_bhopBlockRecords.push_back(bounds);
		}
	}

	if (_killPointServerCommandEntities)
	{
		KillPointServerCommandEntities(true);
	}

	if (!quiet)
	{
		SbtMessage("gimmick scan complete: trigger_push=%zu skipped_bounds=%d skipped_speed=%d skipped_direction=%d trigger_teleport=%zu bhop_block=%zu\n",
			_triggerPushRecords.size(),
			_lastTriggerPushSkippedNoBounds,
			_lastTriggerPushSkippedNoSpeed,
			_lastTriggerPushSkippedNoDirection,
			_triggerTeleportRecords.size(),
			_bhopBlockRecords.size());
	}
}

void SwiftlyBhopTimerHelper::ProcessGimmickFixes()
{
	if (!_triggerPushFixEnabled && !_rngFixTriggerJump && !_disableTelehop && !_rngFixTelehop && _maxBhopBlockTicks <= 0)
	{
		return;
	}

	for (int slot = 0; slot < 64; slot++)
	{
		CEntityInstance *controller = GetControllerBySlot(slot);
		if (IsReplayBotController(controller))
		{
			_gimmickSlots[slot].valid = false;
			continue;
		}

		CEntityInstance *pawn = GetPlayerPawnFromController(controller);
		if (!pawn)
		{
			_gimmickSlots[slot].valid = false;
			continue;
		}

		if (IsNoclipActiveForPawn(slot, pawn))
		{
			_gimmickSlots[slot].valid = false;
			_lastTriggerPushTriggers[slot] = CEntityHandle();
			_lastTeleportTriggers[slot] = CEntityHandle();
			_lastBhopBlocks[slot] = CEntityHandle();
			_bhopBlockTicks[slot] = 0;
			continue;
		}

		SbtVector3 position {};
		if (!GetEntityOrigin(pawn, position))
		{
			_gimmickSlots[slot].valid = false;
			continue;
		}

		const bool hasPreviousOrigin = _gimmickSlots[slot].valid;
		const SbtVector3 previousPosition = hasPreviousOrigin
			? SbtVector3 { _gimmickSlots[slot].origin.x, _gimmickSlots[slot].origin.y, _gimmickSlots[slot].origin.z }
			: position;
		const GimmickVector currentOrigin { position.x, position.y, position.z };
		const GimmickVector previousOrigin { previousPosition.x, previousPosition.y, previousPosition.z };
		const bool priorityTriggerContact = IsInsidePriorityGimmickContact(previousOrigin, currentOrigin);
		if (priorityTriggerContact)
		{
			_gimmickPriorityTicks[slot] = (std::max)(_gimmickPriorityTicks[slot], 2);
		}

		if (_surfSuppressTicks[slot] > 0)
		{
			if (priorityTriggerContact)
			{
				_surfSuppressTicks[slot] = 0;
			}
			else
			{
				_lastTriggerPushTriggers[slot] = CEntityHandle();
				_lastTeleportTriggers[slot] = CEntityHandle();
				_lastBhopBlocks[slot] = CEntityHandle();
				_bhopBlockTicks[slot] = 0;
				_gimmickSlots[slot].valid = true;
				_gimmickSlots[slot].origin = currentOrigin;
				continue;
			}
		}

		if (_triggerPushFixEnabled)
		{
			ProcessTriggerPushFix(pawn, slot, currentOrigin, previousOrigin, hasPreviousOrigin);
		}

		if (_disableTelehop)
		{
			ProcessTeleportSpeedReset(pawn, slot, currentOrigin, previousOrigin, hasPreviousOrigin);
		}

		if (_maxBhopBlockTicks > 0)
		{
			ProcessBhopBlockLimit(pawn, slot, currentOrigin, previousOrigin, hasPreviousOrigin);
		}

		_gimmickSlots[slot].valid = true;
		_gimmickSlots[slot].origin = currentOrigin;
	}
}

void SwiftlyBhopTimerHelper::ProcessTriggerPushFix(CEntityInstance *pawn, int slot, const GimmickVector &origin, const GimmickVector &previousOrigin, bool hasPreviousOrigin)
{
	if (_triggerPushRecords.empty() || slot < 0 || slot >= static_cast<int>(_lastTriggerPushTriggers.size()))
	{
		return;
	}

	const SbtVector3 position { origin.x, origin.y, origin.z };
	const SbtVector3 previousPosition { previousOrigin.x, previousOrigin.y, previousOrigin.z };
	SbtVector3 velocity {};
	if (!ReadEntityVector3Any(pawn, { "CBaseEntity", "CBasePlayerPawn", "CCSPlayerPawn" }, { "m_vecAbsVelocity", "m_vecVelocity" }, velocity))
	{
		return;
	}

	for (const TriggerPushRecord &record : _triggerPushRecords)
	{
		const SbtVector3 mins { record.bounds.mins.x, record.bounds.mins.y, record.bounds.mins.z };
		const SbtVector3 maxs { record.bounds.maxs.x, record.bounds.maxs.y, record.bounds.maxs.z };
		if (!PointInsideBounds(position, mins, maxs, SBT_TRIGGER_TOUCH_PADDING)
			&& (!hasPreviousOrigin || !SegmentIntersectsBounds(previousPosition, position, mins, maxs, SBT_TRIGGER_SWEEP_PADDING)))
		{
			continue;
		}

		const bool isNewTouch = !EntityHandleEquals(_lastTriggerPushTriggers[slot], record.bounds.handle);
		const SbtVector3 direction { record.direction.x, record.direction.y, record.direction.z };
		const float currentAlongPush = (velocity.x * direction.x) + (velocity.y * direction.y) + (velocity.z * direction.z);
		float boost = 0.0f;
		if (isNewTouch)
		{
			boost = record.speed;
		}
		else if (currentAlongPush < record.speed - 0.5f)
		{
			boost = record.speed - currentAlongPush;
		}

		_lastTriggerPushTriggers[slot] = record.bounds.handle;
		if (boost <= 0.0f)
		{
			return;
		}

		const SbtVector3 adjusted {
			velocity.x + (direction.x * boost),
			velocity.y + (direction.y * boost),
			velocity.z + (direction.z * boost),
		};

		SetEntityVector3Any(pawn, { "CBaseEntity", "CBasePlayerPawn", "CCSPlayerPawn" }, { "m_vecAbsVelocity", "m_vecVelocity" }, adjusted);
		_gimmickPriorityTicks[slot] = (std::max)(_gimmickPriorityTicks[slot], 2);
		return;
	}

	_lastTriggerPushTriggers[slot] = CEntityHandle();
}

void SwiftlyBhopTimerHelper::ProcessTeleportSpeedReset(CEntityInstance *pawn, int slot, const GimmickVector &origin, const GimmickVector &previousOrigin, bool hasPreviousOrigin)
{
	if (_triggerTeleportRecords.empty() || slot < 0 || slot >= static_cast<int>(_lastTeleportTriggers.size()))
	{
		return;
	}

	const SbtVector3 position { origin.x, origin.y, origin.z };
	const SbtVector3 previousPosition { previousOrigin.x, previousOrigin.y, previousOrigin.z };
	for (const GimmickBounds &bounds : _triggerTeleportRecords)
	{
		const SbtVector3 mins { bounds.mins.x, bounds.mins.y, bounds.mins.z };
		const SbtVector3 maxs { bounds.maxs.x, bounds.maxs.y, bounds.maxs.z };
		if (!PointInsideBounds(position, mins, maxs, SBT_TRIGGER_TOUCH_PADDING)
			&& (!hasPreviousOrigin || !SegmentIntersectsBounds(previousPosition, position, mins, maxs, SBT_TRIGGER_SWEEP_PADDING)))
		{
			continue;
		}

		if (!EntityHandleEquals(_lastTeleportTriggers[slot], bounds.handle))
		{
			const SbtVector3 zero {};
			SetEntityVector3Any(pawn, { "CBaseEntity", "CBasePlayerPawn", "CCSPlayerPawn" }, { "m_vecAbsVelocity", "m_vecVelocity", "m_vecBaseVelocity" }, zero);
			_lastTeleportTriggers[slot] = bounds.handle;
		}

		return;
	}

	_lastTeleportTriggers[slot] = CEntityHandle();
}

void SwiftlyBhopTimerHelper::ProcessBhopBlockLimit(CEntityInstance *pawn, int slot, const GimmickVector &origin, const GimmickVector &previousOrigin, bool hasPreviousOrigin)
{
	if (_bhopBlockRecords.empty() || slot < 0 || slot >= static_cast<int>(_bhopBlockTicks.size()))
	{
		return;
	}

	const SbtVector3 position { origin.x, origin.y, origin.z };
	const SbtVector3 previousPosition { previousOrigin.x, previousOrigin.y, previousOrigin.z };
	for (const GimmickBounds &bounds : _bhopBlockRecords)
	{
		const SbtVector3 mins { bounds.mins.x, bounds.mins.y, bounds.mins.z };
		const SbtVector3 maxs { bounds.maxs.x, bounds.maxs.y, bounds.maxs.z };
		if (!PointInsideBounds(position, mins, maxs, SBT_TRIGGER_TOUCH_PADDING)
			&& (!hasPreviousOrigin || !SegmentIntersectsBounds(previousPosition, position, mins, maxs, SBT_TRIGGER_SWEEP_PADDING)))
		{
			continue;
		}

		if (EntityHandleEquals(_lastBhopBlocks[slot], bounds.handle))
		{
			_bhopBlockTicks[slot]++;
		}
		else
		{
			_lastBhopBlocks[slot] = bounds.handle;
			_bhopBlockTicks[slot] = 1;
		}

		if (_bhopBlockTicks[slot] > _maxBhopBlockTicks)
		{
			const SbtVector3 dropVelocity { 0.0f, 0.0f, -250.0f };
			SetEntityVector3Any(pawn, { "CBaseEntity", "CBasePlayerPawn", "CCSPlayerPawn" }, { "m_vecAbsVelocity", "m_vecVelocity", "m_vecBaseVelocity" }, dropVelocity);
		}

		return;
	}

	_lastBhopBlocks[slot] = CEntityHandle();
	_bhopBlockTicks[slot] = 0;
}

void SwiftlyBhopTimerHelper::ProcessRngFixes()
{
	const bool enabled = _rngFixDownhill
		|| _rngFixUphill != 0
		|| _rngFixEdge
		|| _rngFixTriggerJump
		|| _rngFixTelehop;
	if (!enabled)
	{
		return;
	}

	_rngFixFrame++;

	for (int slot = 0; slot < 64; slot++)
	{
		CEntityInstance *controller = GetControllerBySlot(slot);
		if (IsReplayBotController(controller))
		{
			_rngFixSlots[slot].valid = false;
			_gimmickPriorityTicks[slot] = 0;
			continue;
		}

		CEntityInstance *pawn = GetPlayerPawnFromController(controller);
		if (!pawn)
		{
			_rngFixSlots[slot].valid = false;
			_gimmickPriorityTicks[slot] = 0;
			continue;
		}

		if (IsNoclipActiveForPawn(slot, pawn))
		{
			_rngFixSlots[slot].valid = false;
			_gimmickPriorityTicks[slot] = 0;
			continue;
		}

		SbtVector3 origin {};
		SbtVector3 velocity {};
		int32_t flags = 0;
		if (!GetEntityOrigin(pawn, origin)
			|| !ReadEntityVector3Any(pawn, { "CBaseEntity", "CBasePlayerPawn", "CCSPlayerPawn" }, { "m_vecAbsVelocity", "m_vecVelocity" }, velocity)
		|| !ReadEntityInt32Any(pawn, { "CBaseEntity", "CBasePlayerPawn", "CCSPlayerPawn" }, { "m_fFlags" }, flags))
		{
			_rngFixSlots[slot].valid = false;
			_gimmickPriorityTicks[slot] = 0;
			continue;
		}

		const bool onGround = (flags & SBT_FL_ONGROUND) != 0;
		const GimmickVector currentOrigin { origin.x, origin.y, origin.z };
		const GimmickVector currentVelocity { velocity.x, velocity.y, velocity.z };
		GimmickVector effectiveVelocity = currentVelocity;
		const RngFixSlotState previous = _rngFixSlots[slot];
		bool appliedPriorityGimmick = false;

		if (previous.valid)
		{
			appliedPriorityGimmick = ProcessRngFixLandingSpeed(pawn, slot, previous, currentOrigin, currentVelocity, onGround, effectiveVelocity) || appliedPriorityGimmick;
			appliedPriorityGimmick = ProcessRngFixTriggerJump(pawn, slot, previous, currentOrigin, currentVelocity, onGround, effectiveVelocity) || appliedPriorityGimmick;
			const GimmickVector preTelehopVelocity = effectiveVelocity;
			appliedPriorityGimmick = ProcessRngFixTelehop(pawn, slot, previous, currentOrigin, preTelehopVelocity, effectiveVelocity) || appliedPriorityGimmick;
		}

		RngFixSlotState &state = _rngFixSlots[slot];
		state.valid = true;
		state.onGround = onGround;
		state.origin = currentOrigin;
		state.velocity = effectiveVelocity;

		const SbtVector3 effectiveVelocityVector { effectiveVelocity.x, effectiveVelocity.y, effectiveVelocity.z };
		if (VectorLength2D(effectiveVelocityVector) > 80.0f)
		{
			state.lastMoveVelocity = effectiveVelocity;
			state.lastMoveFrame = _rngFixFrame;
		}

		if (appliedPriorityGimmick)
		{
			_gimmickPriorityTicks[slot] = (std::max)(_gimmickPriorityTicks[slot], 3);
			_surfSuppressTicks[slot] = 0;
			_surfSlots[slot].valid = false;
		}
	}
}

bool SwiftlyBhopTimerHelper::IsInsidePriorityGimmickContact(const GimmickVector &previousOrigin, const GimmickVector &origin) const
{
	const SbtVector3 start { previousOrigin.x, previousOrigin.y, previousOrigin.z };
	const SbtVector3 end { origin.x, origin.y, origin.z };

	auto touchesBounds = [&](const GimmickBounds &bounds, float padding) {
		const SbtVector3 mins { bounds.mins.x, bounds.mins.y, bounds.mins.z };
		const SbtVector3 maxs { bounds.maxs.x, bounds.maxs.y, bounds.maxs.z };
		return PointInsideBounds(end, mins, maxs, padding)
			|| SegmentIntersectsBounds(start, end, mins, maxs, padding);
	};

	for (const GimmickBounds &bounds : _priorityTriggerRecords)
	{
		if (touchesBounds(bounds, SBT_TRIGGER_SWEEP_PADDING))
		{
			return true;
		}
	}

	for (const TriggerPushRecord &record : _triggerPushRecords)
	{
		if (touchesBounds(record.bounds, SBT_TRIGGER_SWEEP_PADDING))
		{
			return true;
		}
	}

	for (const GimmickBounds &bounds : _triggerTeleportRecords)
	{
		if (touchesBounds(bounds, SBT_TRIGGER_SWEEP_PADDING))
		{
			return true;
		}
	}

	for (const GimmickBounds &bounds : _bhopBlockRecords)
	{
		if (touchesBounds(bounds, SBT_TRIGGER_SWEEP_PADDING))
		{
			return true;
		}
	}

	return false;
}

bool SwiftlyBhopTimerHelper::ProcessRngFixLandingSpeed(CEntityInstance *pawn, int slot, const RngFixSlotState &previous, const GimmickVector &origin, const GimmickVector &velocity, bool onGround, GimmickVector &effectiveVelocity)
{
	if (!_rngFixDownhill && _rngFixUphill == 0 && !_rngFixEdge)
	{
		return false;
	}

	const bool justLanded = !previous.onGround && onGround;
	if (!justLanded)
	{
		return false;
	}

	const SbtVector3 previousVelocity { previous.velocity.x, previous.velocity.y, previous.velocity.z };
	const SbtVector3 currentVelocity { velocity.x, velocity.y, velocity.z };
	if (previousVelocity.z > RNGFIX_NON_JUMP_VELOCITY)
	{
		return false;
	}
	if (std::fabs(currentVelocity.z) > RNGFIX_SURF_VERTICAL_VELOCITY
		&& !IsInsidePriorityGimmickContact(previous.origin, origin))
	{
		if (_rngFixDebug > 0)
		{
			SbtMessage("rngfix landing speed guard skipped surf-like contact: slot=%d current_z=%.2f\n", slot, currentVelocity.z);
		}
		return false;
	}

	const float previousSpeed2D = VectorLength2D(previousVelocity);
	const float currentSpeed2D = VectorLength2D(currentVelocity);
	const float speedLoss = previousSpeed2D - currentSpeed2D;
	if (previousSpeed2D < 180.0f || speedLoss < 12.0f)
	{
		return false;
	}

	const bool downhillCandidate = _rngFixDownhill && previousVelocity.z < -5.0f;
	const bool uphillCandidate = _rngFixUphill != 0 && speedLoss <= 96.0f;
	const bool edgeCandidate = _rngFixEdge && speedLoss <= 128.0f;
	if (!downhillCandidate && !uphillCandidate && !edgeCandidate)
	{
		return false;
	}

	const float maxRestoredLoss = _rngFixUseOldSlopeFixLogic ? 320.0f : 160.0f;
	if (speedLoss > maxRestoredLoss)
	{
		return false;
	}

	SbtVector3 restored { previousVelocity.x, previousVelocity.y, currentVelocity.z };
	SetEntityVector3Any(pawn, { "CBaseEntity", "CBasePlayerPawn", "CCSPlayerPawn" }, { "m_vecAbsVelocity", "m_vecVelocity" }, restored);
	effectiveVelocity = { restored.x, restored.y, restored.z };

	if (_rngFixDebug > 0)
	{
		SbtMessage("rngfix landing speed guard: slot=%d loss=%.2f prev=%.2f current=%.2f\n", slot, speedLoss, previousSpeed2D, currentSpeed2D);
	}

	return true;
}

bool SwiftlyBhopTimerHelper::ProcessRngFixTriggerJump(CEntityInstance *pawn, int slot, const RngFixSlotState &previous, const GimmickVector &origin, const GimmickVector &, bool onGround, GimmickVector &effectiveVelocity)
{
	if (!_rngFixTriggerJump || _triggerPushRecords.empty() || previous.onGround || !onGround)
	{
		return false;
	}

	const SbtVector3 previousPosition { previous.origin.x, previous.origin.y, previous.origin.z };
	const SbtVector3 position { origin.x, origin.y, origin.z };
	const SbtVector3 currentVelocity { effectiveVelocity.x, effectiveVelocity.y, effectiveVelocity.z };

	for (const TriggerPushRecord &record : _triggerPushRecords)
	{
		SbtVector3 mins { record.bounds.mins.x, record.bounds.mins.y, record.bounds.mins.z };
		SbtVector3 maxs { record.bounds.maxs.x, record.bounds.maxs.y, record.bounds.maxs.z };
		maxs.z += SBT_TRIGGER_TOUCH_PADDING;
		mins.z -= 2.0f;
		if (!PointInsideBounds(position, mins, maxs, SBT_TRIGGER_TOUCH_PADDING)
			&& !SegmentIntersectsBounds(previousPosition, position, mins, maxs, SBT_TRIGGER_SWEEP_PADDING))
		{
			continue;
		}

		const SbtVector3 direction { record.direction.x, record.direction.y, record.direction.z };
		const float currentAlongPush = (currentVelocity.x * direction.x) + (currentVelocity.y * direction.y) + (currentVelocity.z * direction.z);
		float boost = record.speed;
		if (currentAlongPush > record.speed + 0.5f)
		{
			boost = record.speed;
		}
		else if (currentAlongPush > 0.0f)
		{
			boost = record.speed - currentAlongPush;
		}

		if (boost <= 0.0f)
		{
			return false;
		}

		const SbtVector3 adjusted {
			currentVelocity.x + (direction.x * boost),
			currentVelocity.y + (direction.y * boost),
			currentVelocity.z + (direction.z * boost),
		};
		SetEntityVector3Any(pawn, { "CBaseEntity", "CBasePlayerPawn", "CCSPlayerPawn" }, { "m_vecAbsVelocity", "m_vecVelocity" }, adjusted);
		effectiveVelocity = { adjusted.x, adjusted.y, adjusted.z };
		_lastTriggerPushTriggers[slot] = record.bounds.handle;

		if (_rngFixDebug > 0)
		{
			SbtMessage("rngfix triggerjump push: slot=%d trigger=%d boost=%.2f\n", slot, record.bounds.entityIndex, boost);
		}
		return true;
	}

	return false;
}

bool SwiftlyBhopTimerHelper::ProcessRngFixTelehop(CEntityInstance *pawn, int slot, const RngFixSlotState &previous, const GimmickVector &origin, const GimmickVector &velocity, GimmickVector &effectiveVelocity)
{
	if (!_rngFixTelehop || slot < 0 || slot >= static_cast<int>(_rngFixSlots.size()))
	{
		return false;
	}

	const SbtVector3 previousOrigin { previous.origin.x, previous.origin.y, previous.origin.z };
	const SbtVector3 currentOrigin { origin.x, origin.y, origin.z };
	const SbtVector3 previousVelocity { previous.velocity.x, previous.velocity.y, previous.velocity.z };
	const SbtVector3 currentVelocity { velocity.x, velocity.y, velocity.z };
	const SbtVector3 lastMoveVelocity { previous.lastMoveVelocity.x, previous.lastMoveVelocity.y, previous.lastMoveVelocity.z };
	const SbtVector3 pendingVelocity { previous.pendingTelehopVelocity.x, previous.pendingTelehopVelocity.y, previous.pendingTelehopVelocity.z };
	const float deltaX = currentOrigin.x - previousOrigin.x;
	const float deltaY = currentOrigin.y - previousOrigin.y;
	const float deltaZ = currentOrigin.z - previousOrigin.z;
	const float delta2D = std::sqrt((deltaX * deltaX) + (deltaY * deltaY));
	const float previousSpeed2D = VectorLength2D(previousVelocity);
	const float currentSpeed2D = VectorLength2D(currentVelocity);
	const float lastMoveSpeed2D = VectorLength2D(lastMoveVelocity);
	const float pendingSpeed2D = VectorLength2D(pendingVelocity);
	const bool hasRecentMoveVelocity = previous.lastMoveFrame > -100000
		&& (_rngFixFrame - previous.lastMoveFrame) <= 16
		&& lastMoveSpeed2D > 80.0f;
	const bool hasPendingTelehop = previous.pendingTelehopRestore
		&& (_rngFixFrame - previous.pendingTelehopFrame) <= 12
		&& pendingSpeed2D > 80.0f;

	auto pointInsideTeleport = [this](const SbtVector3 &point) {
		for (const GimmickBounds &bounds : _triggerTeleportRecords)
		{
			const SbtVector3 mins { bounds.mins.x, bounds.mins.y, bounds.mins.z };
			const SbtVector3 maxs { bounds.maxs.x, bounds.maxs.y, bounds.maxs.z };
			if (PointInsideBounds(point, mins, maxs, 8.0f))
			{
				return true;
			}
		}

		return false;
	};

	const bool previousInsideTeleport = !_triggerTeleportRecords.empty() && pointInsideTeleport(previousOrigin);
	const bool currentInsideTeleport = !_triggerTeleportRecords.empty() && pointInsideTeleport(currentOrigin);
	RngFixSlotState &state = _rngFixSlots[slot];
	if (currentInsideTeleport && currentSpeed2D > 80.0f)
	{
		state.pendingTelehopRestore = true;
		state.pendingTelehopFrame = _rngFixFrame;
		state.pendingTelehopVelocity = velocity;
	}
	else if (currentInsideTeleport && hasRecentMoveVelocity)
	{
		state.pendingTelehopRestore = true;
		state.pendingTelehopFrame = _rngFixFrame;
		state.pendingTelehopVelocity = previous.lastMoveVelocity;
	}

	const float referenceSpeed2D = (std::max)(previousSpeed2D, (std::max)(lastMoveSpeed2D, pendingSpeed2D));
	const bool teleportLikeMove = delta2D > (std::max)(96.0f, referenceSpeed2D * 0.05f) || std::fabs(deltaZ) > 96.0f;
	if (!teleportLikeMove)
	{
		if ((previousInsideTeleport || hasPendingTelehop) && referenceSpeed2D > currentSpeed2D + 24.0f)
		{
			const SbtVector3 restored = pendingSpeed2D >= lastMoveSpeed2D && hasPendingTelehop
				? SbtVector3 { pendingVelocity.x, pendingVelocity.y, currentVelocity.z }
				: SbtVector3 { lastMoveVelocity.x, lastMoveVelocity.y, currentVelocity.z };

			SetEntityVector3Any(pawn, { "CBaseEntity", "CBasePlayerPawn", "CCSPlayerPawn" }, { "m_vecAbsVelocity", "m_vecVelocity", "m_vecBaseVelocity" }, restored);
			effectiveVelocity = { restored.x, restored.y, restored.z };
			state.pendingTelehopRestore = false;
			state.lastTeleportDetected = true;
			state.lastTeleportFrame = _rngFixFrame;
			state.lastMoveVelocity = effectiveVelocity;
			state.lastMoveFrame = _rngFixFrame;

			if (_rngFixDebug > 0)
			{
				SbtMessage("rngfix telehop trigger restore: slot=%d ref=%.2f current=%.2f pending=%d prevInside=%d\n",
					slot,
					referenceSpeed2D,
					currentSpeed2D,
					hasPendingTelehop ? 1 : 0,
					previousInsideTeleport ? 1 : 0);
			}
			return true;
		}

		state.lastTeleportDetected = false;
		return false;
	}

	state.lastTeleportDetected = true;
	state.lastTeleportFrame = _rngFixFrame;
	if (currentSpeed2D >= referenceSpeed2D - 24.0f || referenceSpeed2D <= 80.0f)
	{
		return false;
	}

	SbtVector3 restored { previousVelocity.x, previousVelocity.y, currentVelocity.z };
	if (hasPendingTelehop && pendingSpeed2D >= previousSpeed2D && pendingSpeed2D >= lastMoveSpeed2D)
	{
		restored = { pendingVelocity.x, pendingVelocity.y, currentVelocity.z };
	}
	else if (hasRecentMoveVelocity && lastMoveSpeed2D >= previousSpeed2D)
	{
		restored = { lastMoveVelocity.x, lastMoveVelocity.y, currentVelocity.z };
	}

	SetEntityVector3Any(pawn, { "CBaseEntity", "CBasePlayerPawn", "CCSPlayerPawn" }, { "m_vecAbsVelocity", "m_vecVelocity", "m_vecBaseVelocity" }, restored);
	effectiveVelocity = { restored.x, restored.y, restored.z };
	state.pendingTelehopRestore = false;
	state.lastMoveVelocity = effectiveVelocity;
	state.lastMoveFrame = _rngFixFrame;

	if (_rngFixDebug > 0)
	{
		SbtMessage("rngfix telehop restore: slot=%d ref=%.2f prev=%.2f last=%.2f pending=%.2f current=%.2f delta=%.2f %.2f\n",
			slot,
			referenceSpeed2D,
			previousSpeed2D,
			lastMoveSpeed2D,
			pendingSpeed2D,
			currentSpeed2D,
			delta2D,
			deltaZ);
	}
	return true;
}

void SwiftlyBhopTimerHelper::KillPointServerCommandEntities(bool quiet)
{
	if (!quiet)
	{
		SbtMessage("requesting point_servercommand removal\n");
	}

	ServerCommand("ent_fire point_servercommand kill", true);
}

