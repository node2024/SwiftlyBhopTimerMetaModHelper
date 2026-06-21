// Restart and map_start teleport helpers.
// Included by swiftlybhoptimer_helper.cpp to keep helper features grouped without changing linkage.

void SwiftlyBhopTimerHelper::RequestPlayerRestart(int slot, float x, float y, float z, int delayFrames)
{
	if (slot < 0 || slot >= 64 || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
	{
		SbtMessage("restart skipped: invalid args slot=%d pos=%.3f %.3f %.3f\n", slot, x, y, z);
		return;
	}

	auto existing = std::remove_if(_pendingPlayerRestarts.begin(),
		_pendingPlayerRestarts.end(),
		[slot](const PendingPlayerRestart &pending) {
			return pending.slot == slot;
		});
	_pendingPlayerRestarts.erase(existing, _pendingPlayerRestarts.end());

	PendingPlayerRestart pending;
	pending.slot = slot;
	pending.x = x;
	pending.y = y;
	pending.z = z;
	pending.delayFrames = (std::max)(0, delayFrames);
	_pendingPlayerRestarts.push_back(pending);

	SbtMessage("restart queued: slot=%d pos=%.3f %.3f %.3f delay=%d\n", slot, x, y, z, pending.delayFrames);
}

void SwiftlyBhopTimerHelper::RequestPlayerRestartAtMapStart(int slot, int delayFrames, bool hasFallback, float fallbackX, float fallbackY, float fallbackZ)
{
	if (slot < 0 || slot >= 64)
	{
		SbtMessage("map_start restart skipped: invalid slot=%d\n", slot);
		return;
	}

	if (hasFallback && (!std::isfinite(fallbackX) || !std::isfinite(fallbackY) || !std::isfinite(fallbackZ)))
	{
		SbtMessage("map_start restart skipped: invalid fallback slot=%d pos=%.3f %.3f %.3f\n", slot, fallbackX, fallbackY, fallbackZ);
		return;
	}

	auto existing = std::remove_if(_pendingMapStartRestarts.begin(),
		_pendingMapStartRestarts.end(),
		[slot](const PendingMapStartRestart &pending) {
			return pending.slot == slot;
		});
	_pendingMapStartRestarts.erase(existing, _pendingMapStartRestarts.end());

	PendingMapStartRestart pending;
	pending.slot = slot;
	pending.hasFallback = hasFallback;
	pending.fallbackX = fallbackX;
	pending.fallbackY = fallbackY;
	pending.fallbackZ = fallbackZ;
	pending.delayFrames = (std::max)(0, delayFrames);
	_pendingMapStartRestarts.push_back(pending);

	SbtMessage("map_start restart queued: slot=%d fallback=%d pos=%.3f %.3f %.3f delay=%d\n",
		slot,
		hasFallback ? 1 : 0,
		fallbackX,
		fallbackY,
		fallbackZ,
		pending.delayFrames);
}

void SwiftlyBhopTimerHelper::ProcessPendingPlayerRestarts()
{
	for (auto it = _pendingPlayerRestarts.begin(); it != _pendingPlayerRestarts.end();)
	{
		if (it->delayFrames > 0)
		{
			it->delayFrames--;
			++it;
			continue;
		}

		if (ApplyPlayerRestartTeleport(it->slot, it->x, it->y, it->z))
		{
			it = _pendingPlayerRestarts.erase(it);
			continue;
		}

		it->attemptsRemaining--;
		if (it->attemptsRemaining <= 0)
		{
			SbtMessage("restart failed: slot=%d pos=%.3f %.3f %.3f\n", it->slot, it->x, it->y, it->z);
			it = _pendingPlayerRestarts.erase(it);
			continue;
		}

		it->delayFrames = 1;
		++it;
	}
}

void SwiftlyBhopTimerHelper::ProcessPendingMapStartRestarts()
{
	for (auto it = _pendingMapStartRestarts.begin(); it != _pendingMapStartRestarts.end();)
	{
		if (it->delayFrames > 0)
		{
			it->delayFrames--;
			++it;
			continue;
		}

		if (ApplyPlayerRestartAtMapStart(it->slot, it->hasFallback, it->fallbackX, it->fallbackY, it->fallbackZ))
		{
			it = _pendingMapStartRestarts.erase(it);
			continue;
		}

		it->attemptsRemaining--;
		if (it->attemptsRemaining <= 0)
		{
			SbtMessage("map_start restart failed: slot=%d fallback=%d pos=%.3f %.3f %.3f\n",
				it->slot,
				it->hasFallback ? 1 : 0,
				it->fallbackX,
				it->fallbackY,
				it->fallbackZ);
			it = _pendingMapStartRestarts.erase(it);
			continue;
		}

		it->delayFrames = 1;
		++it;
	}
}

bool SwiftlyBhopTimerHelper::ApplyPlayerRestartTeleport(int slot, float x, float y, float z)
{
	CEntityInstance *controller = GetControllerBySlot(slot);
	CEntityInstance *pawn = GetPlayerPawnFromController(controller);
	if (!pawn)
	{
		return false;
	}

	const SbtVector3 destination { x, y, z };
	const SbtVector3 zero {};
	int applied = 0;

	applied += SetEntityVector3Field(pawn, "CBasePlayerPawn", "m_vOldOrigin", destination) ? 1 : 0;
	applied += SetEntityVector3Field(pawn, "CBaseEntity", "m_vecAbsOrigin", destination) ? 1 : 0;
	applied += SetEntityVector3Field(pawn, "CBaseEntity", "m_vecVelocity", zero) ? 1 : 0;
	applied += SetEntityVector3Field(pawn, "CBaseEntity", "m_vecAbsVelocity", zero) ? 1 : 0;
	applied += SetEntityVector3Field(pawn, "CBaseEntity", "m_vecBaseVelocity", zero) ? 1 : 0;
	applied += AddEntityInt32Flags(pawn, "CBaseEntity", "m_fEffects", SBT_EF_NOINTERP) ? 1 : 0;

	void *sceneNode = GetRawPointerField(pawn, "CBaseEntity", "m_pGameSceneNode");
	if (sceneNode)
	{
		applied += SetRawVector3Field(sceneNode, pawn, "CGameSceneNode", "m_vecOrigin", destination) ? 1 : 0;
		applied += SetRawVector3Field(sceneNode, pawn, "CGameSceneNode", "m_vecAbsOrigin", destination) ? 1 : 0;
	}

	NotifyNetworkStateChanged(pawn, {});
	SbtMessage("restart applied: slot=%d fields=%d pos=%.3f %.3f %.3f\n", slot, applied, x, y, z);
	return applied > 0;
}

bool SwiftlyBhopTimerHelper::ApplyPlayerRestartAtMapStart(int slot, bool hasFallback, float fallbackX, float fallbackY, float fallbackZ)
{
	GimmickVector mapStart {};
	if (TryFindMapStartPosition(mapStart))
	{
		return ApplyPlayerRestartTeleport(slot, mapStart.x, mapStart.y, mapStart.z);
	}

	if (hasFallback)
	{
		SbtMessage("map_start restart using fallback: slot=%d pos=%.3f %.3f %.3f\n", slot, fallbackX, fallbackY, fallbackZ);
		return ApplyPlayerRestartTeleport(slot, fallbackX, fallbackY, fallbackZ);
	}

	return false;
}

bool SwiftlyBhopTimerHelper::TryFindMapStartPosition(GimmickVector &position)
{
	CGameEntitySystem *entitySystem = GetGameEntitySystem();
	if (!entitySystem)
	{
		return false;
	}

	int bestOrder = 100000;
	SbtVector3 bestPosition {};

	auto getMapStartOrder = [](const char *name) {
		if (!name || !name[0])
		{
			return 100000;
		}

		if (V_stricmp(name, "map_start") == 0)
		{
			return 0;
		}

		const char *prefix = "map_start";
		const size_t prefixLength = std::strlen(prefix);
		if (V_strnicmp(name, prefix, static_cast<int>(prefixLength)) != 0)
		{
			return 100000;
		}

		const char *suffix = name + prefixLength;
		if (!suffix[0])
		{
			return 0;
		}

		for (const char *cursor = suffix; *cursor; cursor++)
		{
			if (!std::isdigit(static_cast<unsigned char>(*cursor)))
			{
				return 100000;
			}
		}

		const int order = std::atoi(suffix);
		return order >= 1 && order <= 20 ? order : 100000;
	};

	for (int entityIndex = 1; entityIndex < MAX_TOTAL_ENTITIES; entityIndex++)
	{
		CEntityInstance *entity = GetEntityInstanceByIndex(entitySystem, entityIndex);
		if (!entity)
		{
			continue;
		}

		const int order = getMapStartOrder(GetEntityTargetName(entity));
		if (order >= bestOrder)
		{
			continue;
		}

		SbtVector3 origin {};
		if (!GetEntityOrigin(entity, origin))
		{
			continue;
		}

		bestOrder = order;
		bestPosition = origin;
		if (bestOrder == 0)
		{
			break;
		}
	}

	if (bestOrder >= 100000)
	{
		return false;
	}

	position.x = bestPosition.x;
	position.y = bestPosition.y;
	position.z = bestPosition.z;
	SbtMessage("map_start restart target found: order=%d pos=%.3f %.3f %.3f\n", bestOrder, position.x, position.y, position.z);
	return true;
}

