#pragma once

#include <ISmmPlugin.h>
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include "entityhandle.h"

class CCSPlayerController;
class CEntityInstance;

using CreateBot_t = CCSPlayerController *(void *botProfile, int32_t teamNumber);
using SwitchTeam_t = void(CCSPlayerController *controller, int32_t teamNumber);

class SwiftlyBhopTimerHelper final : public ISmmPlugin, public IMetamodListener
{
public:
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);
	bool Unload(char *error, size_t maxlen);
	bool Pause(char *error, size_t maxlen);
	bool Unpause(char *error, size_t maxlen);
	void AllPluginsLoaded();
	void OnLevelInit(char const *pMapName,
		char const *pMapEntities,
		char const *pOldLevel,
		char const *pLandmarkName,
		bool loadGame,
		bool background);
	void HookGameFrame(bool simulating, bool bFirstTick, bool bLastTick);

	const char *GetAuthor();
	const char *GetName();
	const char *GetDescription();
	const char *GetURL();
	const char *GetLicense();
	const char *GetVersion();
	const char *GetDate();
	const char *GetLogTag();

	void RequestReplayBotAdd(const char *reason, const char *displayName = nullptr, bool oneShot = false);
	void RequestReplayBotKick(const char *reason, const char *displayName = nullptr);
	void RequestReplayBotComplete(const char *reason, const char *displayName = nullptr);
	void ApplyReplayBotConvars(bool quiet = false);
	void ApplyReplayBotPlaybackFixes(bool quiet = false);
	void ApplyRoundFlowConvars();
	void ApplyMovementCompatibilityConvars(bool quiet = false);
	void ScheduleMovementCompatibilityBurst();
	void SetForceAutoBhop(bool enabled);
	void ApplyAutoBhopConvars(bool quiet = false);
	void PrintAutoBhopStatus();
	bool ForcePlayerTeam(int slot, int teamNumber);
	void ApplyRoundTimerSync(float totalSeconds, float elapsedSeconds, float currentTime);
	void ApplyHtmlHudFlashFix(float currentTime, bool quiet = false);
	void InitTimeLimit();
	void SetTimeLimit(float minutes, const char *reason);
	void ScheduleInitialTimeLimitApply();
	void ApplyBhopPhysicsConvars(const char *gravity,
		const char *jumpImpulse,
		const char *airAccelerate,
		const char *airMaxWishSpeed,
		const char *friction);
	void PrintBhopPhysicsStatus();
	void SetBhopMode(int slot, const char *modeName);
	void PrintBhopModeStatus(int slot);
	void ApplyHideFps(int slot, float viewmodelOffsetX, float viewmodelOffsetY, float viewmodelOffsetZ, float viewmodelFov);
	void SetHideFps(int slot, bool hide);
	void SetHideWeapon(int slot, bool hide);
	void DebugHideFps(int slot);
	bool SetPlayerNoclip(int slot, bool enabled);
	bool SetSpectateTarget(int viewerSlot, int targetSlot);
	void RequestPlayerRestart(int slot, float x, float y, float z, int delayFrames);
	void RequestPlayerRestartAtMapStart(int slot, int delayFrames, bool hasFallback, float fallbackX, float fallbackY, float fallbackZ);
	void RequestMapChange(const char *mapName, const char *workshopId);
	void RequestMapChangeCommand(const char *command);
	void SetTriggerPushFix(bool enabled);
	void SetDisableTelehop(bool enabled);
	void SetMaxBhopBlockTime(float seconds);
	void SetKillPointServerCommandEntities(bool enabled);
	void PrintGimmickStatus();
	void SetRngFixDownhill(bool enabled);
	void SetRngFixUphill(int mode);
	void SetRngFixEdge(bool enabled);
	void SetRngFixTriggerJump(bool enabled);
	void SetRngFixTelehop(bool enabled);
	void SetRngFixStairs(bool enabled);
	void SetRngFixUseOldSlopeFixLogic(bool enabled);
	void SetRngFixDebug(int level);
	void PrintRngFixStatus();
	void SetSurfFix(bool enabled);
	void PrintSurfStatus();
	void ApplyDefaultBhopHelperSettings(bool quiet = false);
	void OnCheckTransmit(void **pInfo, int infoCount);

private:
	struct GimmickVector
	{
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
	};

	struct GimmickBounds
	{
		CEntityHandle handle {};
		int entityIndex = -1;
		GimmickVector mins {};
		GimmickVector maxs {};
		std::string name;
	};

	struct TriggerPushRecord
	{
		GimmickBounds bounds {};
		float speed = 0.0f;
		GimmickVector direction {};
		bool speedZeroed = false;
	};

	struct ReplayBotRecord
	{
		CCSPlayerController *controller = nullptr;
		std::string name;
		int slot = -1;
		bool oneShot = false;
		bool persistent = false;
		bool teamFinalized = false;
	};

	struct PendingPlayerRestart
	{
		int slot = -1;
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		int delayFrames = 0;
		int attemptsRemaining = 8;
	};

	struct PendingMapStartRestart
	{
		int slot = -1;
		bool hasFallback = false;
		float fallbackX = 0.0f;
		float fallbackY = 0.0f;
		float fallbackZ = 0.0f;
		int delayFrames = 0;
		int attemptsRemaining = 8;
	};

	struct RngFixSlotState
	{
		bool valid = false;
		bool onGround = false;
		bool lastTeleportDetected = false;
		bool pendingTelehopRestore = false;
		int lastTeleportFrame = -100000;
		int lastMoveFrame = -100000;
		int pendingTelehopFrame = -100000;
		GimmickVector origin {};
		GimmickVector velocity {};
		GimmickVector lastMoveVelocity {};
		GimmickVector pendingTelehopVelocity {};
	};

	struct ClassicBhopSlotState
	{
		bool valid = false;
		bool hasMoveInput = false;
		float lastYaw = 0.0f;
		float stepRemainder = 0.0f;
		int32_t lastMoveButtons = 0;
		int tinyYawFrames = 0;
		GimmickVector lastVelocity {};
	};

	bool ResolveCreateBot();
	bool ResolveSwitchTeam();
	CCSPlayerController *CreateReplayBot(const char *displayName, bool oneShot);
	CEntityInstance *ResolveReplayBotController(ReplayBotRecord &record);
	bool IsReplayBotController(CEntityInstance *controller);
	bool FinalizeReplayBotRecord(ReplayBotRecord &record, bool quiet);
	void PruneStaleReplayBotRecords(bool quiet = false);
	bool PruneReplayBotsForNewBot();
	bool HasReplayBotName(const char *displayName) const;
	bool ReplayBotNameMatches(const std::string &recordName, const char *displayName) const;
	bool RemoveReplayBotByName(const char *displayName, bool kick);
	void RemovePersistentReplayBotsForReplacement(const char *displayName);
	bool RemoveReplayBotByNameOrTag(const char *displayName, bool kick);
	void RemoveReplayBotAt(size_t index, bool kick);
	void KickReplayBotName(const char *displayName);
	void KickReplayBotNameOrTag(const char *displayName);
	void ProcessPendingPlayerRestarts();
	void ProcessPendingMapStartRestarts();
	bool ApplyPlayerRestartTeleport(int slot, float x, float y, float z);
	bool ApplyPlayerRestartAtMapStart(int slot, bool hasFallback, float fallbackX, float fallbackY, float fallbackZ);
	bool TryFindMapStartPosition(GimmickVector &position);
	void ProcessNoclipPlayers();
	bool ApplyPlayerNoclipState(int slot, CEntityInstance *pawn, bool enabled, bool quiet);
	bool ApplyPlayerNoclipMovement(int slot, CEntityInstance *controller, CEntityInstance *pawn);
	bool TryReadNoclipButtons(CEntityInstance *controller, CEntityInstance *pawn, int32_t &buttons);
	bool TryReadNoclipAngles(CEntityInstance *controller, CEntityInstance *pawn, GimmickVector &angles);
	bool IsNoclipActiveForPawn(int slot, CEntityInstance *pawn) const;
	void ServerCommand(const char *command, bool quiet = false);
	void SetConVar(const char *name, const char *value, bool quiet = false);
	void ProcessMovementCompatibilityBurst();
	void ProcessMovementCompatibilityMaintenance();
	bool EnsureMovementCompatibilityConvars(bool quiet = false);
	void ProcessAutoBhopForce();
	void ProcessSurfPhysics();
	bool IsSurfLikeContact(const RngFixSlotState &previous, const GimmickVector &origin, const GimmickVector &velocity, bool onGround) const;
	void ProcessBhopModes();
	bool ProcessClassicBhopMovement(int slot, CEntityInstance *controller, CEntityInstance *pawn);
	bool ApplyReplayBotPlaybackFixesToRecord(ReplayBotRecord &record);
	void RescanGimmickEntities(bool quiet = false);
	void ProcessGimmickFixes();
	void ProcessTriggerPushFix(CEntityInstance *pawn, int slot, const GimmickVector &origin, const GimmickVector &previousOrigin, bool hasPreviousOrigin);
	void ProcessTeleportSpeedReset(CEntityInstance *pawn, int slot, const GimmickVector &origin, const GimmickVector &previousOrigin, bool hasPreviousOrigin);
	void ProcessBhopBlockLimit(CEntityInstance *pawn, int slot, const GimmickVector &origin, const GimmickVector &previousOrigin, bool hasPreviousOrigin);
	void KillPointServerCommandEntities(bool quiet = false);
	void ProcessRngFixes();
	bool IsInsidePriorityGimmickContact(const GimmickVector &previousOrigin, const GimmickVector &origin) const;
	bool ProcessRngFixLandingSpeed(CEntityInstance *pawn, int slot, const RngFixSlotState &previous, const GimmickVector &origin, const GimmickVector &velocity, bool onGround, GimmickVector &effectiveVelocity);
	bool ProcessRngFixTriggerJump(CEntityInstance *pawn, int slot, const RngFixSlotState &previous, const GimmickVector &origin, const GimmickVector &velocity, bool onGround, GimmickVector &effectiveVelocity);
	bool ProcessRngFixTelehop(CEntityInstance *pawn, int slot, const RngFixSlotState &previous, const GimmickVector &origin, const GimmickVector &velocity, GimmickVector &effectiveVelocity);

	CreateBot_t *_createBot = nullptr;
	SwitchTeam_t *_switchTeam = nullptr;
	int _initialTimeLimitApplyFramesRemaining = 0;
	int _initialTimeLimitApplyDelayFrames = 0;
	int _initialTimeLimitApplyPasses = 0;
	int _movementCompatBurstPassesRemaining = 0;
	int _movementCompatBurstDelayFrames = 0;
	int _movementCompatMaintainDelayFrames = 0;
	float _scheduledTimeLimitMinutes = 60.0f;
	std::array<bool, 65> _hideWeaponSlots {};
	std::array<bool, 65> _noclipSlots {};
	std::array<int, 65> _bhopModeSlots {};
	std::array<ClassicBhopSlotState, 65> _classicBhopSlots {};
	struct HideFpsSlotState
	{
		bool enabled = false;
		bool captured = false;
		float offsetX = 1.0f;
		float offsetY = 1.0f;
		float offsetZ = -1.0f;
		float fov = 60.0f;
	};
	std::array<HideFpsSlotState, 65> _hideFpsSlots {};
	std::vector<ReplayBotRecord> _replayBots;
	std::vector<PendingPlayerRestart> _pendingPlayerRestarts;
	std::vector<PendingMapStartRestart> _pendingMapStartRestarts;
	std::vector<GimmickBounds> _priorityTriggerRecords;
	std::vector<TriggerPushRecord> _triggerPushRecords;
	std::vector<GimmickBounds> _triggerTeleportRecords;
	std::vector<GimmickBounds> _bhopBlockRecords;
	std::array<CEntityHandle, 65> _lastTriggerPushTriggers {};
	std::array<CEntityHandle, 65> _lastTeleportTriggers {};
	std::array<CEntityHandle, 65> _lastBhopBlocks {};
	std::array<int, 65> _bhopBlockTicks {};
	std::array<int, 65> _gimmickPriorityTicks {};
	std::array<int, 65> _surfSuppressTicks {};
	std::array<RngFixSlotState, 65> _gimmickSlots {};
	std::array<RngFixSlotState, 65> _rngFixSlots {};
	std::array<RngFixSlotState, 65> _surfSlots {};
	int _rngFixFrame = 0;
	bool _triggerPushFixEnabled = false;
	bool _disableTelehop = false;
	bool _killPointServerCommandEntities = false;
	bool _rngFixDownhill = false;
	int _rngFixUphill = 0;
	bool _rngFixEdge = false;
	bool _rngFixTriggerJump = false;
	bool _rngFixTelehop = false;
	bool _rngFixStairs = false;
	bool _rngFixUseOldSlopeFixLogic = false;
	bool _surfFixEnabled = true;
	bool _forceAutoBhop = false;
	int _rngFixDebug = 0;
	int _lastTriggerPushSkippedNoBounds = 0;
	int _lastTriggerPushSkippedNoSpeed = 0;
	int _lastTriggerPushSkippedNoDirection = 0;
	int _maxBhopBlockTicks = 0;
	int _gimmickRescanFrames = 0;
	bool _gimmickScanWarned = false;
	bool _noclipInputWarned = false;
	bool _noclipAnglesWarned = false;
};

extern SwiftlyBhopTimerHelper g_SwiftlyBhopTimerHelper;

PLUGIN_GLOBALVARS();
