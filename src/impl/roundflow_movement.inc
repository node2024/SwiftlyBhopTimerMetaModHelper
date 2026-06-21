// Round-flow convars, movement compatibility, autobhop, surf and classic movement.
// Included by swiftlybhoptimer_helper.cpp to keep helper features grouped without changing linkage.

void SwiftlyBhopTimerHelper::ApplyRoundFlowConvars()
{
	SetConVar("game_type", "3");
	SetConVar("game_mode", "0");
	SetConVar("mp_maxrounds", "0");
	SetConVar("mp_freezetime", "0");
	SetConVar("mp_team_intro_time", "0");
	SetConVar("mp_round_restart_delay", "0");
	SetConVar("mp_halftime", "0");
	SetConVar("mp_overtime_enable", "0");
	SetConVar("mp_ignore_round_win_conditions", "1");
	SetConVar("mp_endmatch_votenextmap", "0");
	SetConVar("mp_endmatch_votenextleveltime", "0");
	SetConVar("mp_endmatch_votenextmap_keepcurrent", "0");
	SetConVar("mp_match_end_changelevel", "1");
	SetConVar("mp_match_end_restart", "0");
	SetConVar("mp_match_restart_delay", "0");
	SetConVar("mp_warmup_pausetimer", "0");
	SetConVar("mp_warmup_offline_enabled", "0");
	SetConVar("mp_warmup_online_enabled", "0");
	SetConVar("mp_warmuptime", "0");
	SetConVar("mp_warmuptime_all_players_connected", "0");
	SetConVar("sv_warmup_to_freezetime_delay", "0");
	SetConVar("mp_respawn_on_death_ct", "0");
	SetConVar("mp_respawn_on_death_t", "0");
	SetConVar("mp_respawn_immunitytime", "-1");
	SetConVar("mp_autokick", "0");
	SetConVar("mp_autoteambalance", "0");
	SetConVar("mp_limitteams", "0");
	SetConVar("mp_humanteam", "any");
	SetConVar("mp_force_pick_time", "0");
	SetConVar("bot_quota_mode", "normal");
	SetConVar("bot_quota", "5");
	SetConVar("bot_prefix", "");
	SetConVar("bot_join_after_player", "0");
	SetConVar("bot_auto_vacate", "0");
	SetConVar("bot_controllable", "0");
	SetConVar("mp_friendlyfire", "0");
	SetConVar("mp_solid_teammates", "0");
	SetConVar("mp_solid_enemies", "0");
	SetConVar("mp_disconnect_kills_players", "0");
	SetConVar("mp_disconnect_kills_bots", "0");
	SetConVar("mp_death_drop_gun", "0");
	SetConVar("mp_death_drop_grenade", "0");
	SetConVar("mp_death_drop_c4", "0");
	SetConVar("mp_death_drop_defuser", "0");
	SetConVar("mp_drop_knife_enable", "0");
	SetConVar("mp_weapons_allow_map_placed", "0");
	SetConVar("sv_infinite_ammo", "2");
	SetConVar("sv_noclipspeed", "5");
	SetConVar("sv_noclipaccelerate", "5");
	SetConVar("sv_falldamage_scale", "0");
	SetConVar("sv_staminajumpcost", "0");
	SetConVar("sv_staminalandcost", "0");
	SetConVar("sv_staminarecoveryrate", "60");
	SetConVar("sv_staminamax", "0");
	ApplyMovementCompatibilityConvars(true);
	SetConVar("sv_vote_allow_in_warmup", "0");
	ServerCommand("mp_unpause_match");
	ServerCommand("mp_warmup_end");

	SbtMessage("Round flow convars applied.\n");
}

void SwiftlyBhopTimerHelper::ApplyMovementCompatibilityConvars(bool quiet)
{
	SetConVar("sv_jump_precision_enable", "0", quiet);
	SetConVar("sv_legacy_jump", "1", quiet);
	SetConVar("sv_subtick_movement_view_angles", "0", quiet);
	SetConVar("sv_staminajumpcost", "0", quiet);
	SetConVar("sv_staminalandcost", "0", quiet);
	SetConVar("sv_staminarecoveryrate", "60", quiet);
	SetConVar("sv_staminamax", "0", quiet);

	if (!quiet)
	{
		SbtMessage("Movement compatibility convars applied: sv_jump_precision_enable 0, sv_legacy_jump 1, sv_subtick_movement_view_angles 0, stamina disabled\n");
	}
}

void SwiftlyBhopTimerHelper::ScheduleMovementCompatibilityBurst()
{
	_movementCompatBurstPassesRemaining = 20;
	_movementCompatBurstDelayFrames = 1;
	SbtMessage("Movement compatibility burst scheduled. Passes=%d intervalFrames=%d\n", _movementCompatBurstPassesRemaining, 32);
}

void SwiftlyBhopTimerHelper::ProcessMovementCompatibilityBurst()
{
	if (_movementCompatBurstPassesRemaining <= 0)
	{
		return;
	}

	if (_movementCompatBurstDelayFrames > 0)
	{
		_movementCompatBurstDelayFrames--;
		return;
	}

	ApplyMovementCompatibilityConvars(true);
	_movementCompatBurstPassesRemaining--;
	_movementCompatBurstDelayFrames = 32;
}

static bool EnsureBoolConVarValue(CConVarRef<bool> &convar, const char *name, bool desired, bool quiet)
{
	if (!convar.IsValidRef() || !convar.IsConVarDataAvailable())
	{
		return false;
	}

	const bool current = convar.Get();
	if (current == desired)
	{
		return true;
	}

	convar.Set(desired);
	if (!quiet)
	{
		SbtMessage("Corrected movement compatibility convar: %s %d -> %d\n", name, current ? 1 : 0, desired ? 1 : 0);
	}

	return true;
}

bool SwiftlyBhopTimerHelper::EnsureMovementCompatibilityConvars(bool quiet)
{
	const bool precisionAvailable = EnsureBoolConVarValue(g_sbtSvJumpPrecisionEnable, "sv_jump_precision_enable", false, quiet);
	const bool legacyAvailable = EnsureBoolConVarValue(g_sbtSvLegacyJump, "sv_legacy_jump", true, quiet);
	const bool viewAnglesAvailable = EnsureBoolConVarValue(g_sbtSvSubtickMovementViewAngles, "sv_subtick_movement_view_angles", false, quiet);

	if (precisionAvailable && legacyAvailable && viewAnglesAvailable)
	{
		return true;
	}

	ApplyMovementCompatibilityConvars(true);
	return false;
}

void SwiftlyBhopTimerHelper::ProcessMovementCompatibilityMaintenance()
{
	if (_movementCompatMaintainDelayFrames > 0)
	{
		_movementCompatMaintainDelayFrames--;
		return;
	}

	_movementCompatMaintainDelayFrames = 320;
	EnsureMovementCompatibilityConvars(false);
}

void SwiftlyBhopTimerHelper::SetForceAutoBhop(bool enabled)
{
	_forceAutoBhop = enabled;
	if (enabled)
	{
		ApplyAutoBhopConvars(false);
	}

	SbtMessage("auto-bhop force %s\n", enabled ? "enabled" : "disabled");
}

void SwiftlyBhopTimerHelper::ApplyAutoBhopConvars(bool quiet)
{
	SetConVar("sv_enablebunnyhopping", "1", quiet);
	SetConVar("sv_autobunnyhopping", "1", quiet);
	ApplyMovementCompatibilityConvars(quiet);
}

void SwiftlyBhopTimerHelper::PrintAutoBhopStatus()
{
	SbtMessage("auto-bhop status: force=%d convars=sv_enablebunnyhopping 1, sv_autobunnyhopping 1\n",
		_forceAutoBhop ? 1 : 0);
}

void SwiftlyBhopTimerHelper::ProcessAutoBhopForce()
{
	if (!_forceAutoBhop)
	{
		return;
	}

	ApplyAutoBhopConvars(true);
}

bool SwiftlyBhopTimerHelper::IsSurfLikeContact(const RngFixSlotState &previous, const GimmickVector &origin, const GimmickVector &velocity, bool onGround) const
{
	if (!previous.valid || previous.onGround || onGround)
	{
		return false;
	}

	const SbtVector3 previousVelocity { previous.velocity.x, previous.velocity.y, previous.velocity.z };
	const SbtVector3 currentVelocity { velocity.x, velocity.y, velocity.z };
	const float speed2D = VectorLength2D(currentVelocity);
	if (speed2D < SBT_SURF_MIN_SPEED)
	{
		return false;
	}

	if (IsInsidePriorityGimmickContact(previous.origin, origin))
	{
		return false;
	}

	const float verticalDelta = currentVelocity.z - previousVelocity.z;
	const float originDeltaZ = origin.z - previous.origin.z;
	const bool verticalSupported = std::fabs(currentVelocity.z) >= SBT_SURF_MIN_VERTICAL_SPEED
		&& verticalDelta > SBT_SURF_VERTICAL_DROP_GRACE;
	const bool upwardSurf = currentVelocity.z > RNGFIX_SURF_VERTICAL_VELOCITY
		&& verticalSupported
		&& originDeltaZ > -2.0f;
	const bool redirectedFall = previousVelocity.z < SBT_SURF_REDIRECT_FALL_SPEED
		&& verticalDelta > SBT_SURF_REDIRECT_MIN_Z_GAIN;

	return upwardSurf || redirectedFall;
}

void SwiftlyBhopTimerHelper::ProcessSurfPhysics()
{
	if (!_surfFixEnabled)
	{
		return;
	}

	for (int slot = 0; slot < 64; slot++)
	{
		CEntityInstance *controller = GetControllerBySlot(slot);
		if (IsReplayBotController(controller))
		{
			_surfSlots[slot].valid = false;
			_surfSuppressTicks[slot] = 0;
			continue;
		}

		CEntityInstance *pawn = GetPlayerPawnFromController(controller);
		if (!pawn || IsNoclipActiveForPawn(slot, pawn))
		{
			_surfSlots[slot].valid = false;
			_surfSuppressTicks[slot] = 0;
			continue;
		}

		SbtVector3 origin {};
		SbtVector3 velocity {};
		int32_t flags = 0;
		if (!GetEntityOrigin(pawn, origin)
			|| !ReadEntityVector3Any(pawn, { "CBaseEntity", "CBasePlayerPawn", "CCSPlayerPawn" }, { "m_vecAbsVelocity", "m_vecVelocity" }, velocity)
			|| !ReadEntityInt32Any(pawn, { "CBaseEntity", "CBasePlayerPawn", "CCSPlayerPawn" }, { "m_fFlags" }, flags))
		{
			_surfSlots[slot].valid = false;
			_surfSuppressTicks[slot] = 0;
			continue;
		}

		const bool onGround = (flags & SBT_FL_ONGROUND) != 0;
		const GimmickVector currentOrigin { origin.x, origin.y, origin.z };
		GimmickVector currentVelocity { velocity.x, velocity.y, velocity.z };
		const RngFixSlotState previous = _surfSlots[slot];

		if (_gimmickPriorityTicks[slot] > 0)
		{
			_surfSuppressTicks[slot] = 0;
			RngFixSlotState &state = _surfSlots[slot];
			state.valid = true;
			state.onGround = onGround;
			state.origin = currentOrigin;
			state.velocity = currentVelocity;
			continue;
		}

		const GimmickVector priorityStart = previous.valid ? previous.origin : currentOrigin;
		if (IsInsidePriorityGimmickContact(priorityStart, currentOrigin))
		{
			_surfSuppressTicks[slot] = 0;
			RngFixSlotState &state = _surfSlots[slot];
			state.valid = true;
			state.onGround = onGround;
			state.origin = currentOrigin;
			state.velocity = currentVelocity;
			continue;
		}

		const bool surfLikeContact = IsSurfLikeContact(previous, currentOrigin, currentVelocity, onGround);
		if (surfLikeContact)
		{
			_surfSuppressTicks[slot] = (std::max)(_surfSuppressTicks[slot], SBT_SURF_SUPPRESS_TICKS);
		}
		else if (_surfSuppressTicks[slot] > 0)
		{
			_surfSuppressTicks[slot]--;
		}

		const bool surfActive = surfLikeContact || _surfSuppressTicks[slot] > 0;
		if (surfActive && previous.valid)
		{
			const float gravityStep = SBT_SURF_GRAVITY * GetServerFrameTime();
			const float previousZ = previous.velocity.z;
			const float maxAllowedZ = previousZ >= 0.0f
				? previousZ + SBT_SURF_MAX_Z_GAIN_PER_TICK
				: previousZ - gravityStep + SBT_SURF_MAX_EXTRA_ABOVE_GRAVITY;

			if (currentVelocity.z > maxAllowedZ)
			{
				SbtVector3 adjusted { currentVelocity.x, currentVelocity.y, maxAllowedZ };
				if (SetEntityVector3Any(pawn, { "CBaseEntity", "CBasePlayerPawn", "CCSPlayerPawn" }, { "m_vecAbsVelocity", "m_vecVelocity" }, adjusted))
				{
					currentVelocity.z = maxAllowedZ;
					if (_rngFixDebug > 0)
					{
						SbtMessage("surf fix z clamp: slot=%d prev_z=%.2f current_z=%.2f clamped_z=%.2f\n",
							slot,
							previousZ,
							velocity.z,
							maxAllowedZ);
					}
				}
			}
		}

		RngFixSlotState &state = _surfSlots[slot];
		state.valid = true;
		state.onGround = onGround;
		state.origin = currentOrigin;
		state.velocity = currentVelocity;
	}
}

void SwiftlyBhopTimerHelper::ProcessBhopModes()
{
	for (int slot = 0; slot < 64; slot++)
	{
		if (_bhopModeSlots[slot] != SBT_BHOP_MODE_CLASSIC)
		{
			_classicBhopSlots[slot].valid = false;
			continue;
		}

		CEntityInstance *controller = GetControllerBySlot(slot);
		if (IsReplayBotController(controller))
		{
			_classicBhopSlots[slot].valid = false;
			continue;
		}

		CEntityInstance *pawn = GetPlayerPawnFromController(controller);
		if (!pawn || IsNoclipActiveForPawn(slot, pawn))
		{
			_classicBhopSlots[slot].valid = false;
			continue;
		}

		if (_surfSuppressTicks[slot] > 0)
		{
			SbtVector3 velocity {};
			if (ReadEntityVector3Any(pawn, { "CBaseEntity", "CBasePlayerPawn", "CCSPlayerPawn" }, { "m_vecAbsVelocity", "m_vecVelocity" }, velocity))
			{
				ClassicBhopSlotState &state = _classicBhopSlots[slot];
				GimmickVector angleValues {};
				if (TryReadNoclipAngles(controller, pawn, angleValues))
				{
					state.lastYaw = angleValues.y;
				}

				state.valid = true;
				state.hasMoveInput = false;
				state.stepRemainder = 0.0f;
				state.lastMoveButtons = 0;
				state.tinyYawFrames = 0;
				state.lastVelocity = { velocity.x, velocity.y, velocity.z };
			}
			continue;
		}

		if (_gimmickPriorityTicks[slot] > 0)
		{
			_gimmickPriorityTicks[slot]--;

			SbtVector3 velocity {};
			if (ReadEntityVector3Any(pawn, { "CBaseEntity", "CBasePlayerPawn", "CCSPlayerPawn" }, { "m_vecAbsVelocity", "m_vecVelocity" }, velocity))
			{
				ClassicBhopSlotState &state = _classicBhopSlots[slot];
				GimmickVector angleValues {};
				if (TryReadNoclipAngles(controller, pawn, angleValues))
				{
					state.lastYaw = angleValues.y;
				}

				state.valid = true;
				state.hasMoveInput = false;
				state.stepRemainder = 0.0f;
				state.lastMoveButtons = 0;
				state.tinyYawFrames = 0;
				state.lastVelocity = { velocity.x, velocity.y, velocity.z };
			}

			continue;
		}

		ProcessClassicBhopMovement(slot, controller, pawn);
	}
}

bool SwiftlyBhopTimerHelper::ProcessClassicBhopMovement(int slot, CEntityInstance *controller, CEntityInstance *pawn)
{
	if (!pawn || slot < 0 || slot >= static_cast<int>(_classicBhopSlots.size()))
	{
		return false;
	}

	SbtVector3 velocity {};
	int32_t flags = 0;
	if (!ReadEntityVector3Any(pawn, { "CBaseEntity", "CBasePlayerPawn", "CCSPlayerPawn" }, { "m_vecAbsVelocity", "m_vecVelocity" }, velocity)
		|| !ReadEntityInt32Any(pawn, { "CBaseEntity", "CBasePlayerPawn", "CCSPlayerPawn" }, { "m_fFlags" }, flags))
	{
		_classicBhopSlots[slot].valid = false;
		return false;
	}

	GimmickVector angleValues {};
	if (!TryReadNoclipAngles(controller, pawn, angleValues))
	{
		_classicBhopSlots[slot].valid = false;
		return false;
	}

	ClassicBhopSlotState &state = _classicBhopSlots[slot];
	const bool onGround = (flags & SBT_FL_ONGROUND) != 0;
	const float speed2D = VectorLength2D(velocity);
	const float previousStoredSpeed2D = state.valid
		? std::sqrt((state.lastVelocity.x * state.lastVelocity.x) + (state.lastVelocity.y * state.lastVelocity.y))
		: 0.0f;
	if (onGround || (speed2D < SBT_CLASSIC_MIN_STRAFE_SPEED && previousStoredSpeed2D < SBT_CLASSIC_MIN_STRAFE_SPEED))
	{
		state.valid = true;
		state.hasMoveInput = false;
		state.lastYaw = angleValues.y;
		state.stepRemainder = 0.0f;
		state.lastMoveButtons = 0;
		state.tinyYawFrames = 0;
		state.lastVelocity = { velocity.x, velocity.y, velocity.z };
		return false;
	}

	int32_t buttons = 0;
	if (!TryReadNoclipButtons(controller, pawn, buttons))
	{
		state.valid = false;
		state.hasMoveInput = false;
		return false;
	}

	const int32_t movementButtons = buttons & (SBT_IN_FORWARD | SBT_IN_BACK | SBT_IN_MOVELEFT | SBT_IN_MOVERIGHT);
	const int32_t sideButtons = movementButtons & (SBT_IN_MOVELEFT | SBT_IN_MOVERIGHT);
	const int32_t lastSideButtons = state.lastMoveButtons & (SBT_IN_MOVELEFT | SBT_IN_MOVERIGHT);
	const bool movementButtonsChanged = state.hasMoveInput && state.lastMoveButtons != movementButtons;
	const bool sideButtonsChanged = state.hasMoveInput && lastSideButtons != sideButtons;

	float forwardMove = 0.0f;
	float sideMove = 0.0f;
	if ((buttons & SBT_IN_FORWARD) != 0)
	{
		forwardMove += SBT_CLASSIC_CSS_MAX_WISH_SPEED;
	}
	if ((buttons & SBT_IN_BACK) != 0)
	{
		forwardMove -= SBT_CLASSIC_CSS_MAX_WISH_SPEED;
	}
	if ((buttons & SBT_IN_MOVERIGHT) != 0)
	{
		sideMove += SBT_CLASSIC_CSS_MAX_WISH_SPEED;
	}
	if ((buttons & SBT_IN_MOVELEFT) != 0)
	{
		sideMove -= SBT_CLASSIC_CSS_MAX_WISH_SPEED;
	}

	if (std::fabs(forwardMove) <= 0.001f && std::fabs(sideMove) <= 0.001f)
	{
		state.valid = true;
		state.hasMoveInput = false;
		state.lastYaw = angleValues.y;
		state.stepRemainder = 0.0f;
		state.lastMoveButtons = 0;
		state.tinyYawFrames = 0;
		state.lastVelocity = { velocity.x, velocity.y, velocity.z };
		return false;
	}

	if (!state.valid)
	{
		state.valid = true;
		state.hasMoveInput = true;
		state.lastYaw = angleValues.y;
		state.stepRemainder = 0.0f;
		state.lastMoveButtons = movementButtons;
		state.tinyYawFrames = 0;
		state.lastVelocity = { velocity.x, velocity.y, velocity.z };
		return false;
	}

	const float previousSpeed2D = previousStoredSpeed2D;
	const float yawDelta = NormalizeAngleDelta(angleValues.y - state.lastYaw);
	const float absYawDelta = std::fabs(yawDelta);
	const bool sideTurnAligned = (sideMove < 0.0f && yawDelta > 0.0f) || (sideMove > 0.0f && yawDelta < 0.0f);
	const bool microStrafeAligned = sideTurnAligned
		&& std::fabs(sideMove) > 0.001f
		&& absYawDelta >= SBT_CLASSIC_MICRO_STRAFE_MIN_YAW_DELTA;
	if (absYawDelta < SBT_CLASSIC_MIN_YAW_DELTA && !microStrafeAligned)
	{
		state.tinyYawFrames++;
		if (std::fabs(sideMove) > 0.001f
			&& state.tinyYawFrames <= SBT_CLASSIC_TINY_YAW_GRACE_FRAMES
			&& speed2D + SBT_CLASSIC_MAX_REASONABLE_SPEED_DROP >= previousSpeed2D)
		{
			if (speed2D > previousSpeed2D)
			{
				state.lastVelocity = { velocity.x, velocity.y, velocity.z };
			}
			state.hasMoveInput = true;
			state.lastMoveButtons = movementButtons;
			return false;
		}

		state.lastYaw = angleValues.y;
		state.stepRemainder = 0.0f;
		state.hasMoveInput = true;
		state.lastMoveButtons = movementButtons;
		state.tinyYawFrames = 0;
		state.lastVelocity = { velocity.x, velocity.y, velocity.z };
		return false;
	}
	state.tinyYawFrames = 0;

	SbtVector3 previousDirection { state.lastVelocity.x, state.lastVelocity.y, 0.0f };
	SbtVector3 currentDirection { velocity.x, velocity.y, 0.0f };
	const bool hasPreviousDirection = NormalizeVector3(previousDirection);
	const bool hasCurrentDirection = NormalizeVector3(currentDirection);
	const float directionDot = hasPreviousDirection && hasCurrentDirection
		? (previousDirection.x * currentDirection.x) + (previousDirection.y * currentDirection.y)
		: 1.0f;
	const float speedDrop = previousSpeed2D - speed2D;
	const bool collisionLikeSpeedLoss = previousSpeed2D >= SBT_CLASSIC_MIN_STRAFE_SPEED
		&& speedDrop >= (std::max)(SBT_CLASSIC_COLLISION_SPEED_DROP_MIN, previousSpeed2D * SBT_CLASSIC_COLLISION_SPEED_DROP_RATIO)
		&& (!hasCurrentDirection || speed2D < SBT_CLASSIC_RESTORE_CURRENT_DIRECTION_MIN_SPEED || directionDot < SBT_CLASSIC_COLLISION_DIRECTION_DOT_MAX);
	if (collisionLikeSpeedLoss)
	{
		state.lastYaw = angleValues.y;
		state.stepRemainder = 0.0f;
		state.hasMoveInput = true;
		state.lastMoveButtons = movementButtons;
		state.tinyYawFrames = 0;
		state.lastVelocity = { velocity.x, velocity.y, velocity.z };
		return false;
	}

	const bool majorEngineSpeedDrop = speed2D + SBT_CLASSIC_MAX_REASONABLE_SPEED_DROP < previousSpeed2D;
	if (previousSpeed2D < SBT_CLASSIC_MIN_STRAFE_SPEED || (majorEngineSpeedDrop && !sideTurnAligned))
	{
		state.lastYaw = angleValues.y;
		state.stepRemainder = 0.0f;
		state.hasMoveInput = true;
		state.lastMoveButtons = movementButtons;
		state.tinyYawFrames = 0;
		state.lastVelocity = { velocity.x, velocity.y, velocity.z };
		return false;
	}

	SbtVector3 cssVelocity { state.lastVelocity.x, state.lastVelocity.y, 0.0f };
	if (speed2D > previousSpeed2D)
	{
		cssVelocity = { velocity.x, velocity.y, 0.0f };
	}

	state.stepRemainder += GetServerFrameTime();
	int airSteps = 0;
	while (state.stepRemainder + 0.0001f >= SBT_CLASSIC_CSS_TICK_INTERVAL && airSteps < SBT_CLASSIC_MAX_AIR_STEPS_PER_FRAME)
	{
		state.stepRemainder -= SBT_CLASSIC_CSS_TICK_INTERVAL;
		airSteps++;
	}
	const bool immediateInputStep = movementButtonsChanged
		&& sideButtonsChanged
		&& sideTurnAligned
		&& std::fabs(sideMove) > 0.001f
		&& absYawDelta >= SBT_CLASSIC_MICRO_STRAFE_MIN_YAW_DELTA;
	if (immediateInputStep && airSteps < SBT_CLASSIC_MAX_AIR_STEPS_PER_FRAME)
	{
		airSteps++;
	}
	if (state.stepRemainder > SBT_CLASSIC_CSS_TICK_INTERVAL * SBT_CLASSIC_MAX_AIR_STEPS_PER_FRAME)
	{
		state.stepRemainder = 0.0f;
	}

	if (airSteps <= 0)
	{
		state.lastYaw = angleValues.y;
		state.hasMoveInput = true;
		state.lastMoveButtons = movementButtons;
		state.lastVelocity = { velocity.x, velocity.y, velocity.z };
		return false;
	}

	const float yaw = angleValues.y * SBT_DEG_TO_RAD;
	const float sy = std::sin(yaw);
	const float cy = std::cos(yaw);
	const SbtVector3 forward { cy, sy, 0.0f };
	const SbtVector3 right { sy, -cy, 0.0f };
	const bool steeringSpeedLoss = sideTurnAligned && std::fabs(sideMove) > 0.001f && previousSpeed2D > speed2D + 1.0f;
	SbtVector3 restoreDirection { velocity.x, velocity.y, 0.0f };
	bool hasRestoreDirection = speed2D >= SBT_CLASSIC_RESTORE_CURRENT_DIRECTION_MIN_SPEED && NormalizeVector3(restoreDirection);
	if (!hasRestoreDirection)
	{
		restoreDirection = {
			(forward.x * forwardMove) + (right.x * sideMove),
			(forward.y * forwardMove) + (right.y * sideMove),
			0.0f,
		};
		hasRestoreDirection = NormalizeVector3(restoreDirection);
	}

	for (int step = 0; step < airSteps; step++)
	{
		SbtVector3 wishVelocity {
			(forward.x * forwardMove) + (right.x * sideMove),
			(forward.y * forwardMove) + (right.y * sideMove),
			0.0f,
		};

		float wishSpeed = VectorLength2D(wishVelocity);
		if (wishSpeed <= 0.001f || !std::isfinite(wishSpeed))
		{
			break;
		}

		if (wishSpeed > SBT_CLASSIC_CSS_MAX_WISH_SPEED)
		{
			const float scale = SBT_CLASSIC_CSS_MAX_WISH_SPEED / wishSpeed;
			wishVelocity.x *= scale;
			wishVelocity.y *= scale;
			wishSpeed = SBT_CLASSIC_CSS_MAX_WISH_SPEED;
		}

		SbtVector3 wishDir { wishVelocity.x, wishVelocity.y, 0.0f };
		if (!NormalizeVector3(wishDir))
		{
			break;
		}

		const float cappedWishSpeed = (std::min)(wishSpeed, SBT_CLASSIC_CSS_AIR_WISH_SPEED_CAP);
		float currentSpeedAlongWishDir = (cssVelocity.x * wishDir.x) + (cssVelocity.y * wishDir.y);
		const float cssSpeed2DForAssist = VectorLength2D(cssVelocity);
		float assistedProjection = SBT_CLASSIC_ASSISTED_WISH_PROJECTION_HIGH_SPEED;
		if (cssSpeed2DForAssist < SBT_CLASSIC_LOW_SPEED_ASSIST_END)
		{
			assistedProjection = SBT_CLASSIC_ASSISTED_WISH_PROJECTION_LOW_SPEED;
		}
		else if (cssSpeed2DForAssist >= SBT_CLASSIC_HIGH_SPEED_ASSIST_START)
		{
			const float range = SBT_CLASSIC_HIGH_SPEED_ASSIST_END - SBT_CLASSIC_HIGH_SPEED_ASSIST_START;
			const float t = range <= 0.001f
				? 1.0f
				: (std::max)(0.0f, (std::min)(1.0f, (cssSpeed2DForAssist - SBT_CLASSIC_HIGH_SPEED_ASSIST_START) / range));
			assistedProjection = SBT_CLASSIC_ASSISTED_WISH_PROJECTION_HIGH_SPEED
				+ ((SBT_CLASSIC_ASSISTED_WISH_PROJECTION_VERY_HIGH_SPEED - SBT_CLASSIC_ASSISTED_WISH_PROJECTION_HIGH_SPEED) * t);
		}
		if (sideTurnAligned
			&& sideMove != 0.0f
			&& cssSpeed2DForAssist > cappedWishSpeed + 1.0f
			&& (currentSpeedAlongWishDir > assistedProjection
				|| currentSpeedAlongWishDir < SBT_CLASSIC_MIN_EFFECTIVE_WISH_PROJECTION))
		{
			const float desiredProjection = (std::min)(assistedProjection, cappedWishSpeed - 0.25f);
			const float ratio = (std::max)(-1.0f, (std::min)(1.0f, desiredProjection / cssSpeed2DForAssist));
			const float offset = std::acos(ratio);
			const float velocityYaw = std::atan2(cssVelocity.y, cssVelocity.x);
			const float sign = sideMove < 0.0f ? 1.0f : -1.0f;
			wishDir = { std::cos(velocityYaw + (sign * offset)), std::sin(velocityYaw + (sign * offset)), 0.0f };
			currentSpeedAlongWishDir = (cssVelocity.x * wishDir.x) + (cssVelocity.y * wishDir.y);
		}

		const float addSpeed = cappedWishSpeed - currentSpeedAlongWishDir;
		if (addSpeed <= 0.0f)
		{
			continue;
		}

		float accelSpeed = SBT_CLASSIC_CSS_AIR_ACCELERATE * wishSpeed * SBT_CLASSIC_CSS_TICK_INTERVAL;
		if (accelSpeed > addSpeed)
		{
			accelSpeed = addSpeed;
		}

		const float speedBeforeStep = VectorLength2D(cssVelocity);
		const SbtVector3 candidateVelocity {
			cssVelocity.x + (accelSpeed * wishDir.x),
			cssVelocity.y + (accelSpeed * wishDir.y),
			0.0f,
		};
		if (VectorLength2D(candidateVelocity) + 0.01f < speedBeforeStep)
		{
			continue;
		}

		cssVelocity.x = candidateVelocity.x;
		cssVelocity.y = candidateVelocity.y;
	}

	const float cssSpeed2D = VectorLength2D(cssVelocity);
	if (cssSpeed2D <= speed2D + 0.01f)
	{
		state.lastYaw = angleValues.y;
		state.hasMoveInput = true;
		state.lastMoveButtons = movementButtons;
		state.lastVelocity = { velocity.x, velocity.y, velocity.z };
		return false;
	}

	const float correctionLimit = steeringSpeedLoss
		? SBT_CLASSIC_STEERING_LOSS_MAX_CORRECTION_PER_FRAME
		: SBT_CLASSIC_MAX_CORRECTION_PER_FRAME;
	const float correction = (std::min)(correctionLimit, cssSpeed2D - speed2D);
	SbtVector3 adjustedHorizontal { cssVelocity.x, cssVelocity.y, 0.0f };
	if (steeringSpeedLoss && hasRestoreDirection)
	{
		const float restoredSpeed = speed2D + correction;
		adjustedHorizontal.x = restoreDirection.x * restoredSpeed;
		adjustedHorizontal.y = restoreDirection.y * restoredSpeed;
	}
	else if (correction < cssSpeed2D - speed2D)
	{
		if (!NormalizeVector3(adjustedHorizontal))
		{
			state.lastYaw = angleValues.y;
			state.hasMoveInput = true;
			state.lastMoveButtons = movementButtons;
			state.lastVelocity = { velocity.x, velocity.y, velocity.z };
			return false;
		}

		adjustedHorizontal.x *= speed2D + correction;
		adjustedHorizontal.y *= speed2D + correction;
	}

	const SbtVector3 adjusted {
		adjustedHorizontal.x,
		adjustedHorizontal.y,
		velocity.z,
	};

	const bool applied = SetEntityVector3Any(pawn,
		{ "CBaseEntity", "CBasePlayerPawn", "CCSPlayerPawn" },
		{ "m_vecAbsVelocity", "m_vecVelocity" },
		adjusted);
	if (applied)
	{
		NotifyNetworkStateChanged(pawn, {});
		state.hasMoveInput = true;
		state.lastMoveButtons = movementButtons;
		state.lastVelocity = { adjusted.x, adjusted.y, adjusted.z };
	}
	else
	{
		state.hasMoveInput = true;
		state.lastMoveButtons = movementButtons;
		state.lastVelocity = { velocity.x, velocity.y, velocity.z };
	}

	state.lastYaw = angleValues.y;
	return applied;
}

