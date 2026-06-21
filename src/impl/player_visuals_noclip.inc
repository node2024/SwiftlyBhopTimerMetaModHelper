// Viewmodel hiding, forced team, spectate target, noclip, transmit filtering.
// Included by swiftlybhoptimer_helper.cpp to keep helper features grouped without changing linkage.

void SwiftlyBhopTimerHelper::ApplyHideFps(int slot, float viewmodelOffsetX, float viewmodelOffsetY, float viewmodelOffsetZ, float viewmodelFov)
{
	CEntityInstance *controller = GetControllerBySlot(slot);
	CEntityInstance *pawn = GetPlayerPawnFromController(controller);
	if (!pawn)
	{
		SbtMessage("hidefps skipped: pawn unavailable for slot %d\n", slot);
		return;
	}

	bool applied = false;
	applied |= SetPawnFloatField(pawn, "m_flViewmodelOffsetX", viewmodelOffsetX);
	applied |= SetPawnFloatField(pawn, "m_flViewmodelOffsetY", viewmodelOffsetY);
	applied |= SetPawnFloatField(pawn, "m_flViewmodelOffsetZ", viewmodelOffsetZ);
	applied |= SetPawnFloatField(pawn, "m_flViewmodelFOV", viewmodelFov);

	if (!applied)
	{
		SbtMessage("hidefps failed: schema fields unavailable for slot %d\n", slot);
	}
}

void SwiftlyBhopTimerHelper::SetHideFps(int slot, bool hide)
{
	if (slot < 0 || slot >= static_cast<int>(_hideFpsSlots.size()))
	{
		SbtMessage("hidefps toggle skipped: invalid slot %d\n", slot);
		return;
	}

	HideFpsSlotState &state = _hideFpsSlots[slot];
	state.enabled = hide;
	_hideWeaponSlots[slot] = hide;

	CEntityInstance *controller = GetControllerBySlot(slot);
	CEntityInstance *pawn = GetPlayerPawnFromController(controller);
	if (!pawn)
	{
		bool sentDrawViewModel = SendClientConVar(slot, "r_drawviewmodel", hide ? "0" : "1");
		if (!hide)
		{
			state.captured = false;
			state.offsetX = DEFAULT_VIEWMODEL_OFFSET_X;
			state.offsetY = DEFAULT_VIEWMODEL_OFFSET_Y;
			state.offsetZ = DEFAULT_VIEWMODEL_OFFSET_Z;
			state.fov = DEFAULT_VIEWMODEL_FOV;
		}

		SbtMessage("hidefps slot %d = %d; pawn unavailable r_drawviewmodel=%d\n", slot, hide ? 1 : 0, sentDrawViewModel ? 1 : 0);
		ServerCommand("sv_fullupdate", true);
		return;
	}

	if (hide)
	{
		if (!state.captured)
		{
			float value = 0.0f;
			if (GetPawnFloatField(pawn, "m_flViewmodelOffsetX", value) && !IsHiddenViewmodelValue(value))
			{
				state.offsetX = value;
			}
			if (GetPawnFloatField(pawn, "m_flViewmodelOffsetY", value) && !IsHiddenViewmodelValue(value))
			{
				state.offsetY = value;
			}
			if (GetPawnFloatField(pawn, "m_flViewmodelOffsetZ", value) && !IsHiddenViewmodelValue(value))
			{
				state.offsetZ = value;
			}
			if (GetPawnFloatField(pawn, "m_flViewmodelFOV", value) && value > HIDEFPS_FOV)
			{
				state.fov = value;
			}
			state.captured = true;
		}

		ApplyHideFps(slot, HIDEFPS_OFFSET_X, HIDEFPS_OFFSET_Y, HIDEFPS_OFFSET_Z, HIDEFPS_FOV);
		int hiddenViewModels = ApplyOwnedViewModelEntities(pawn, true, true);
		bool sentDrawViewModel = SendClientConVar(slot, "r_drawviewmodel", "0");
		SbtMessage("hidefps slot %d = 1; viewmodels=%d r_drawviewmodel=%d\n", slot, hiddenViewModels, sentDrawViewModel ? 1 : 0);
		ServerCommand("sv_fullupdate", true);
		return;
	}

	int restoredViewModels = ApplyOwnedViewModelEntities(pawn, false, true);
	bool sentDrawViewModel = SendClientConVar(slot, "r_drawviewmodel", "1");
	ApplyHideFps(slot,
		state.captured ? state.offsetX : DEFAULT_VIEWMODEL_OFFSET_X,
		state.captured ? state.offsetY : DEFAULT_VIEWMODEL_OFFSET_Y,
		state.captured ? state.offsetZ : DEFAULT_VIEWMODEL_OFFSET_Z,
		state.captured ? state.fov : DEFAULT_VIEWMODEL_FOV);

	state.captured = false;
	state.offsetX = DEFAULT_VIEWMODEL_OFFSET_X;
	state.offsetY = DEFAULT_VIEWMODEL_OFFSET_Y;
	state.offsetZ = DEFAULT_VIEWMODEL_OFFSET_Z;
	state.fov = DEFAULT_VIEWMODEL_FOV;
	SbtMessage("hidefps slot %d = 0; viewmodels=%d r_drawviewmodel=%d\n", slot, restoredViewModels, sentDrawViewModel ? 1 : 0);
	ServerCommand("sv_fullupdate", true);
}

void SwiftlyBhopTimerHelper::SetHideWeapon(int slot, bool hide)
{
	if (slot < 0 || slot >= static_cast<int>(_hideWeaponSlots.size()))
	{
		SbtMessage("hideweapon skipped: invalid slot %d\n", slot);
		return;
	}

	_hideWeaponSlots[slot] = hide;
	SbtMessage("hideweapon slot %d = %d\n", slot, hide ? 1 : 0);
	ServerCommand("sv_fullupdate", true);
}

void SwiftlyBhopTimerHelper::DebugHideFps(int slot)
{
	if (slot < 0 || slot >= static_cast<int>(_hideFpsSlots.size()))
	{
		SbtMessage("hidefps debug skipped: invalid slot %d\n", slot);
		return;
	}

	CGameEntitySystem *entitySystem = GetGameEntitySystem();
	CEntityInstance *controller = GetControllerBySlot(slot);
	CEntityInstance *pawn = GetPlayerPawnFromController(controller);
	SbtMessage("hidefps debug slot=%d enabled=%d controller=%p pawn=%p entitySystem=%p networkMessages=%p gameEventSystem=%p\n",
		slot,
		_hideFpsSlots[slot].enabled ? 1 : 0,
		controller,
		pawn,
		entitySystem,
		g_pNetworkMessages,
		g_pSbtGameEventSystem);

	SbtMessage("hidefps schema offsets: m_pViewModelServices=%d/%d m_hViewModel=%d/%d m_hOwner=%d/%d m_fEffects=%d/%d m_clrRender=%d/%d\n",
		FindSchemaFieldOffset("CCSPlayerPawn", "m_pViewModelServices").found ? 1 : 0,
		FindSchemaFieldOffset("CCSPlayerPawn", "m_pViewModelServices").offset,
		FindSchemaFieldOffset("CCSPlayer_ViewModelServices", "m_hViewModel").found ? 1 : 0,
		FindSchemaFieldOffset("CCSPlayer_ViewModelServices", "m_hViewModel").offset,
		FindSchemaFieldOffset("CBaseViewModel", "m_hOwner").found ? 1 : 0,
		FindSchemaFieldOffset("CBaseViewModel", "m_hOwner").offset,
		FindSchemaFieldOffset("CBaseEntity", "m_fEffects").found ? 1 : 0,
		FindSchemaFieldOffset("CBaseEntity", "m_fEffects").offset,
		FindSchemaFieldOffset("CBaseModelEntity", "m_clrRender").found ? 1 : 0,
		FindSchemaFieldOffset("CBaseModelEntity", "m_clrRender").offset);

	if (!entitySystem || !pawn)
	{
		return;
	}

	CEntityHandle *viewModels = GetPawnViewModelHandles(pawn);
	if (!viewModels)
	{
		SbtMessage("hidefps debug: pawn viewmodel handles unavailable\n");
	}
	else
	{
		for (int viewModelIndex = 0; viewModelIndex < 3; viewModelIndex++)
		{
			CEntityInstance *viewModel = GetEntityInstanceByHandle(entitySystem, viewModels[viewModelIndex]);
			SbtMessage("hidefps debug viewmodel[%d] valid=%d entity=%p index=%d class=%s\n",
				viewModelIndex,
				viewModels[viewModelIndex].IsValid() ? 1 : 0,
				viewModel,
				viewModel ? viewModel->GetEntityIndex().Get() : -1,
				viewModel && viewModel->GetClassname() ? viewModel->GetClassname() : "none");
		}
	}

	int ownedCount = 0;
	for (int entityIndex = 1; entityIndex < MAX_TOTAL_ENTITIES; entityIndex++)
	{
		CEntityInstance *entity = GetEntityInstanceByIndex(entitySystem, entityIndex);
		if (!IsOwnedViewModel(entity, pawn))
		{
			continue;
		}

		ownedCount++;
		SbtMessage("hidefps debug owned viewmodel class=%s index=%d\n",
			entity->GetClassname() ? entity->GetClassname() : "unknown",
			entity->GetEntityIndex().Get());
	}

	SbtMessage("hidefps debug owned viewmodel count=%d\n", ownedCount);
}

bool SwiftlyBhopTimerHelper::ForcePlayerTeam(int slot, int teamNumber)
{
	if (slot < 0 || slot >= 64 || (teamNumber != CS_TEAM_T && teamNumber != CS_TEAM_CT))
	{
		SbtMessage("force team skipped: invalid slot/team slot=%d team=%d\n", slot, teamNumber);
		return false;
	}

	CEntityInstance *controller = GetControllerBySlot(slot);
	if (!controller)
	{
		SbtMessage("force team skipped: controller unavailable for slot %d\n", slot);
		return false;
	}

	if (IsReplayBotController(controller))
	{
		SbtMessage("force team skipped: slot %d is a replay bot\n", slot);
		return false;
	}

	ApplyReplayBotConvars(true);

	bool applied = false;
	if (ResolveSwitchTeam())
	{
		_switchTeam(reinterpret_cast<CCSPlayerController *>(controller), teamNumber);
		applied = true;
	}
	else
	{
		applied |= SetEntityInt32Field(controller, "CCSPlayerController", "m_iTeamNum", teamNumber);
	}

	CEntityInstance *pawn = GetPlayerPawnFromController(controller);
	if (pawn)
	{
		applied |= SetEntityInt32Field(pawn, "CCSPlayerPawn", "m_iTeamNum", teamNumber);
		NotifyNetworkStateChanged(pawn, {});
	}

	SbtMessage("force team slot %d team=%d applied=%d\n", slot, teamNumber, applied ? 1 : 0);
	return applied;
}

bool SwiftlyBhopTimerHelper::SetSpectateTarget(int viewerSlot, int targetSlot)
{
	if (viewerSlot < 0 || viewerSlot >= 64 || targetSlot < 0 || targetSlot >= 64)
	{
		SbtMessage("spec target skipped: invalid slots viewer=%d target=%d\n", viewerSlot, targetSlot);
		return false;
	}

	CEntityInstance *viewerController = GetControllerBySlot(viewerSlot);
	CEntityInstance *targetController = GetControllerBySlot(targetSlot);
	CEntityInstance *viewerPawn = GetPlayerPawnFromController(viewerController);
	CEntityInstance *targetPawn = GetPlayerPawnFromController(targetController);
	if (!viewerPawn || !targetPawn)
	{
		SbtMessage("spec target skipped: pawn unavailable viewer=%d target=%d viewerPawn=%p targetPawn=%p\n",
			viewerSlot,
			targetSlot,
			viewerPawn,
			targetPawn);
		return false;
	}

	void *observerServices = GetPawnObserverServices(viewerPawn);
	if (!observerServices)
	{
		SbtMessage("spec target skipped: observer services unavailable for slot %d\n", viewerSlot);
		return false;
	}

	const bool targetApplied = SetObserverServicesTarget(observerServices, viewerPawn, targetPawn);
	const bool modeApplied = SetObserverServicesMode(observerServices, viewerPawn, 4);
	if (!targetApplied)
	{
		SbtMessage("spec target failed: m_hObserverTarget unavailable viewer=%d target=%d\n", viewerSlot, targetSlot);
		return false;
	}

	if (!modeApplied)
	{
		SbtMessage("spec target applied without observer mode: m_iObserverMode unavailable viewer=%d target=%d\n", viewerSlot, targetSlot);
	}

	return true;
}

bool SwiftlyBhopTimerHelper::SetPlayerNoclip(int slot, bool enabled)
{
	if (slot < 0 || slot >= static_cast<int>(_noclipSlots.size()))
	{
		SbtMessage("noclip skipped: invalid slot %d\n", slot);
		return false;
	}

	_noclipSlots[slot] = enabled;
	_rngFixSlots[slot].valid = false;
	_classicBhopSlots[slot].valid = false;
	_lastTriggerPushTriggers[slot] = CEntityHandle();
	_lastTeleportTriggers[slot] = CEntityHandle();
	_lastBhopBlocks[slot] = CEntityHandle();
	_bhopBlockTicks[slot] = 0;

	CEntityInstance *controller = GetControllerBySlot(slot);
	CEntityInstance *pawn = GetPlayerPawnFromController(controller);
	if (!pawn)
	{
		SbtMessage("noclip skipped: pawn unavailable for slot %d\n", slot);
		return enabled;
	}

	return ApplyPlayerNoclipState(slot, pawn, enabled, false);
}

void SwiftlyBhopTimerHelper::ProcessNoclipPlayers()
{
	for (int slot = 0; slot < static_cast<int>(_noclipSlots.size()); slot++)
	{
		if (!_noclipSlots[slot])
		{
			continue;
		}

		CEntityInstance *controller = GetControllerBySlot(slot);
		CEntityInstance *pawn = GetPlayerPawnFromController(controller);
		if (!pawn)
		{
			continue;
		}

		ApplyPlayerNoclipState(slot, pawn, true, true);
		ApplyPlayerNoclipMovement(slot, controller, pawn);
	}
}

bool SwiftlyBhopTimerHelper::ApplyPlayerNoclipState(int slot, CEntityInstance *pawn, bool enabled, bool quiet)
{
	if (!pawn)
	{
		return false;
	}

	const uint8_t moveType = enabled ? SBT_MOVETYPE_NOCLIP : SBT_MOVETYPE_WALK;
	int applied = 0;
	uint8_t currentMoveType = 0;
	if (!ReadEntityUInt8Field(pawn, "CBaseEntity", "m_MoveType", currentMoveType) || currentMoveType != moveType)
	{
		applied += SetEntityUInt8Field(pawn, "CBaseEntity", "m_MoveType", moveType) ? 1 : 0;
	}

	uint8_t currentActualMoveType = 0;
	if (!ReadEntityUInt8Field(pawn, "CBaseEntity", "m_nActualMoveType", currentActualMoveType) || currentActualMoveType != moveType)
	{
		applied += SetEntityUInt8Field(pawn, "CBaseEntity", "m_nActualMoveType", moveType) ? 1 : 0;
	}

	if (!enabled)
	{
		const SbtVector3 zero {};
		applied += SetEntityVector3Field(pawn, "CBaseEntity", "m_vecVelocity", zero) ? 1 : 0;
		applied += SetEntityVector3Field(pawn, "CBaseEntity", "m_vecAbsVelocity", zero) ? 1 : 0;
		applied += SetEntityVector3Field(pawn, "CBaseEntity", "m_vecBaseVelocity", zero) ? 1 : 0;
	}

	if (applied > 0)
	{
		NotifyNetworkStateChanged(pawn, {});
		SetConVar("sv_noclipspeed", "5", true);
		SetConVar("sv_noclipaccelerate", "5", true);
	}

	if (!quiet)
	{
		SbtMessage("noclip slot %d = %d fields=%d\n", slot, enabled ? 1 : 0, applied);
	}

	return applied > 0 || enabled;
}

bool SwiftlyBhopTimerHelper::ApplyPlayerNoclipMovement(int slot, CEntityInstance *controller, CEntityInstance *pawn)
{
	if (!pawn)
	{
		return false;
	}

	int32_t buttons = 0;
	if (!TryReadNoclipButtons(controller, pawn, buttons))
	{
		if (!_noclipInputWarned)
		{
			_noclipInputWarned = true;
			SbtMessage("noclip movement warning: button state was not found; falling back to engine noclip movement\n");
		}
		return false;
	}

	GimmickVector angleValues {};
	if (!TryReadNoclipAngles(controller, pawn, angleValues))
	{
		if (!_noclipAnglesWarned)
		{
			_noclipAnglesWarned = true;
			SbtMessage("noclip movement warning: eye angles were not found; using world-axis fallback\n");
		}
	}

	const float pitch = angleValues.x * SBT_DEG_TO_RAD;
	const float yaw = angleValues.y * SBT_DEG_TO_RAD;
	const float sp = std::sin(pitch);
	const float cp = std::cos(pitch);
	const float sy = std::sin(yaw);
	const float cy = std::cos(yaw);

	const SbtVector3 forward { cp * cy, cp * sy, -sp };
	const SbtVector3 right { sy, -cy, 0.0f };
	const SbtVector3 up { 0.0f, 0.0f, 1.0f };
	SbtVector3 direction {};

	if ((buttons & SBT_IN_FORWARD) != 0)
	{
		direction.x += forward.x;
		direction.y += forward.y;
		direction.z += forward.z;
	}
	if ((buttons & SBT_IN_BACK) != 0)
	{
		direction.x -= forward.x;
		direction.y -= forward.y;
		direction.z -= forward.z;
	}
	if ((buttons & SBT_IN_MOVERIGHT) != 0)
	{
		direction.x += right.x;
		direction.y += right.y;
		direction.z += right.z;
	}
	if ((buttons & SBT_IN_MOVELEFT) != 0)
	{
		direction.x -= right.x;
		direction.y -= right.y;
		direction.z -= right.z;
	}
	if ((buttons & SBT_IN_JUMP) != 0)
	{
		direction.x += up.x;
		direction.y += up.y;
		direction.z += up.z;
	}
	if ((buttons & SBT_IN_DUCK) != 0)
	{
		direction.x -= up.x;
		direction.y -= up.y;
		direction.z -= up.z;
	}

	SbtVector3 velocity {};
	const bool hasInput = NormalizeVector3(direction);
	if (hasInput)
	{
		float speed = SBT_NOCLIP_SPEED;
		if ((buttons & SBT_IN_SPEED) != 0)
		{
			speed *= SBT_NOCLIP_FAST_MULTIPLIER;
		}
		if ((buttons & SBT_IN_WALK) != 0)
		{
			speed *= SBT_NOCLIP_SLOW_MULTIPLIER;
		}

		velocity = { direction.x * speed, direction.y * speed, direction.z * speed };
	}

	SetEntityVector3Any(pawn,
		{ "CBaseEntity", "CBasePlayerPawn", "CCSPlayerPawn" },
		{ "m_vecVelocity", "m_vecAbsVelocity" },
		velocity);
	const SbtVector3 zero {};
	SetEntityVector3Any(pawn,
		{ "CBaseEntity", "CBasePlayerPawn", "CCSPlayerPawn" },
		{ "m_vecBaseVelocity" },
		zero);

	if (!hasInput)
	{
		return true;
	}

	SbtVector3 origin {};
	if (!GetEntityOrigin(pawn, origin))
	{
		return true;
	}

	const float frameTime = GetServerFrameTime();
	const SbtVector3 destination {
		origin.x + (velocity.x * frameTime),
		origin.y + (velocity.y * frameTime),
		origin.z + (velocity.z * frameTime),
	};

	int applied = 0;
	applied += SetEntityVector3Field(pawn, "CBasePlayerPawn", "m_vOldOrigin", destination) ? 1 : 0;
	applied += SetEntityVector3Field(pawn, "CBaseEntity", "m_vecAbsOrigin", destination) ? 1 : 0;

	void *sceneNode = GetRawPointerField(pawn, "CBaseEntity", "m_pGameSceneNode");
	if (sceneNode)
	{
		applied += SetRawVector3Field(sceneNode, pawn, "CGameSceneNode", "m_vecOrigin", destination) ? 1 : 0;
		applied += SetRawVector3Field(sceneNode, pawn, "CGameSceneNode", "m_vecAbsOrigin", destination) ? 1 : 0;
	}

	if (applied > 0)
	{
		NotifyNetworkStateChanged(pawn, {});
	}

	return applied > 0;
}

bool SwiftlyBhopTimerHelper::TryReadNoclipButtons(CEntityInstance *controller, CEntityInstance *pawn, int32_t &buttons)
{
	void *movementServices = GetRawPointerField(pawn, "CBasePlayerPawn", "m_pMovementServices");
	if (!movementServices)
	{
		movementServices = GetRawPointerField(pawn, "CCSPlayerPawn", "m_pMovementServices");
	}
	if (movementServices)
	{
		int32_t combinedButtons = 0;
		bool foundMovementButtons = false;
		for (const char *schemaClass : { "CCSPlayer_MovementServices", "CPlayer_MovementServices", "CPlayer_MovementServices_Humanoid" })
		{
			for (const char *fieldName : { "m_nButtons", "m_nQueuedButtonDownMask", "m_nButtonDownMaskPrev" })
			{
				uint64_t rawButtons64 = 0;
				if (ReadRawUInt64Field(movementServices, schemaClass, fieldName, rawButtons64))
				{
					combinedButtons |= static_cast<int32_t>(rawButtons64 & 0xFFFFFFFFu);
					foundMovementButtons = true;
					continue;
				}

				int32_t rawButtons32 = 0;
				if (ReadRawInt32Field(movementServices, schemaClass, fieldName, rawButtons32))
				{
					combinedButtons |= rawButtons32;
					foundMovementButtons = true;
				}
			}
		}

		if (foundMovementButtons)
		{
			buttons = combinedButtons;
			return true;
		}
	}

	if (ReadEntityInt32Any(pawn,
		{ "CCSPlayerPawn", "CBasePlayerPawn", "CBasePlayer", "CBaseEntity" },
		{ "m_nButtons", "m_afButtonPressed", "m_afButtonLast" },
		buttons))
	{
		return true;
	}

	return ReadEntityInt32Any(controller,
		{ "CCSPlayerController", "CBasePlayerController", "CBasePlayerPawn", "CBasePlayer" },
		{ "m_nButtons", "m_afButtonPressed", "m_afButtonLast" },
		buttons);
}

bool SwiftlyBhopTimerHelper::TryReadNoclipAngles(CEntityInstance *controller, CEntityInstance *pawn, GimmickVector &angles)
{
	SbtVector3 value {};
	if (ReadEntityVector3Any(pawn,
		{ "CCSPlayerPawn", "CBasePlayerPawn", "CBasePlayer", "CBaseEntity" },
		{ "m_angEyeAngles", "m_angRotation", "m_angAbsRotation" },
		value))
	{
		angles = { value.x, value.y, value.z };
		return true;
	}

	void *cameraServices = GetRawPointerField(pawn, "CBasePlayerPawn", "m_pCameraServices");
	if (!cameraServices)
	{
		cameraServices = GetRawPointerField(pawn, "CCSPlayerPawn", "m_pCameraServices");
	}
	if (cameraServices)
	{
		for (const char *schemaClass : { "CCSPlayer_CameraServices", "CPlayer_CameraServices" })
		{
			for (const char *fieldName : { "m_angEyeAngles", "m_angViewAngles", "m_vecCsViewPunchAngle" })
			{
				if (ReadRawVector3Field(cameraServices, schemaClass, fieldName, value))
				{
					angles = { value.x, value.y, value.z };
					return true;
				}
			}
		}
	}

	if (ReadEntityVector3Any(controller,
		{ "CCSPlayerController", "CBasePlayerController", "CBasePlayerPawn", "CBasePlayer", "CBaseEntity" },
		{ "m_angEyeAngles", "m_angRotation", "m_angAbsRotation" },
		value))
	{
		angles = { value.x, value.y, value.z };
		return true;
	}

	angles = {};
	return false;
}

bool SwiftlyBhopTimerHelper::IsNoclipActiveForPawn(int slot, CEntityInstance *pawn) const
{
	if (slot >= 0 && slot < static_cast<int>(_noclipSlots.size()) && _noclipSlots[slot])
	{
		return true;
	}

	if (!pawn)
	{
		return false;
	}

	uint8_t moveType = 0;
	if (ReadEntityUInt8Field(pawn, "CBaseEntity", "m_MoveType", moveType) && moveType == SBT_MOVETYPE_NOCLIP)
	{
		return true;
	}

	if (ReadEntityUInt8Field(pawn, "CBaseEntity", "m_nActualMoveType", moveType) && moveType == SBT_MOVETYPE_NOCLIP)
	{
		return true;
	}

	return false;
}

void SwiftlyBhopTimerHelper::OnCheckTransmit(void **pInfo, int infoCount)
{
	if (!pInfo || infoCount <= 0)
	{
		return;
	}

	for (int i = 0; i < infoCount; i++)
	{
		if (!pInfo[i])
		{
			continue;
		}

		auto *transmitInfo = reinterpret_cast<CBitVec<16384> **>(pInfo[i]);
		CBitVec<16384> *transmitEdict = *transmitInfo;
		if (!transmitEdict)
		{
			continue;
		}

		int targetSlot = *reinterpret_cast<int *>(reinterpret_cast<uintptr_t>(pInfo[i]) + CHECK_TRANSMIT_PLAYER_SLOT_OFFSET);
		if (targetSlot < 0 || targetSlot >= static_cast<int>(_hideWeaponSlots.size()) || !_hideWeaponSlots[targetSlot])
		{
			continue;
		}

		CEntityInstance *controller = GetControllerBySlot(targetSlot);
		CEntityInstance *pawn = GetPlayerPawnFromController(controller);
		ClearOwnedViewModelTransmit(transmitEdict, pawn);

		CUtlVector<CEntityHandle> *weapons = GetPawnWeaponHandles(pawn);
		if (!weapons)
		{
			continue;
		}

		FOR_EACH_VEC(*weapons, weaponIndex)
		{
			const CEntityHandle &weaponHandle = weapons->Element(weaponIndex);
			if (!weaponHandle.IsValid())
			{
				continue;
			}

			ClearTransmitEntity(transmitEdict, weaponHandle);
		}
	}
}

