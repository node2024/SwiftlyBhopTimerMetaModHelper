#include "swiftlybhoptimer_helper.h"

#include "icvar.h"
#include "eiface.h"
#include "interface.h"
#include "iserver.h"
#include "tier0/dbg.h"
#include "utlvector.h"
#include "entity2/entityinstance.h"
#include "entity2/entitysystem.h"
#include "entityhandle.h"
#include "edict.h"
#include "engine/igameeventsystem.h"
#include "interfaces/interfaces.h"
#include "irecipientfilter.h"
#include "networkbasetypes.pb.h"
#include "networksystem/inetworkmessages.h"
#include "schemasystem/schemasystem.h"
#include "tier0/icommandline.h"
#include "tier1/utlstring.h"

#include <cstdint>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <map>
#include <string>

#if defined _WIN32
#include <windows.h>
#else
#include <link.h>
#endif

#include "tier0/memdbgon.h"

IVEngineServer2 *g_pEngineServer2 = nullptr;
extern ISchemaSystem *g_pSchemaSystem;
static IGameResourceService *g_pSbtGameResourceServiceServer = nullptr;
static ISource2GameEntities *g_pSbtSource2GameEntities = nullptr;
static IGameEventSystem *g_pSbtGameEventSystem = nullptr;
static IServerGameDLL *g_pSbtServerGameDll = nullptr;

static void SbtMessage(const char *msg, ...);

SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, false, bool, bool, bool);
SH_DECL_HOOK7_void(ISource2GameEntities, CheckTransmit, SH_NOATTRIB, false, CCheckTransmitInfo **, int, CBitVec<16384> &, CBitVec<16384> &,
	const Entity2Networkable_t **, const uint16 *, int);

static void SbtHookCheckTransmit(CCheckTransmitInfo **pInfo,
	int infoCount,
	CBitVec<16384> &,
	CBitVec<16384> &,
	const Entity2Networkable_t **,
	const uint16 *,
	int)
{
	g_SwiftlyBhopTimerHelper.OnCheckTransmit(reinterpret_cast<void **>(pInfo), infoCount);
	RETURN_META(MRES_IGNORED);
}

namespace
{
constexpr int CS_TEAM_T = 2;
constexpr int CS_TEAM_CT = 3;
constexpr size_t MAX_REPLAY_BOTS = 5;

#if defined _WIN32
constexpr const char *SERVER_SCHEMA_MODULE = "server.dll";
#else
constexpr const char *SERVER_SCHEMA_MODULE = "libserver.so";
#endif

constexpr uintptr_t CHECK_TRANSMIT_PLAYER_SLOT_OFFSET = 576;
constexpr float HIDEFPS_OFFSET_X = 0.0f;
constexpr float HIDEFPS_OFFSET_Y = -1000.0f;
constexpr float HIDEFPS_OFFSET_Z = -1000.0f;
constexpr float HIDEFPS_FOV = 1.0f;
constexpr float DEFAULT_VIEWMODEL_OFFSET_X = 1.0f;
constexpr float DEFAULT_VIEWMODEL_OFFSET_Y = 1.0f;
constexpr float DEFAULT_VIEWMODEL_OFFSET_Z = -1.0f;
constexpr float DEFAULT_VIEWMODEL_FOV = 60.0f;
constexpr int EF_NODRAW = 0x20;
constexpr uint8_t RENDER_ALPHA_HIDDEN = 0;
constexpr uint8_t RENDER_ALPHA_VISIBLE = 255;
constexpr float MAX_TIMER_CONVAR_MINUTES = 1440.0f;
constexpr float DEFAULT_TIMER_CONVAR_MINUTES = 60.0f;
constexpr int SBT_EF_NOINTERP = 1 << 3;
constexpr int SBT_FL_ONGROUND = 1 << 0;
constexpr uint8_t SBT_MOVETYPE_WALK = 2;
constexpr uint8_t SBT_MOVETYPE_NOCLIP = 7;
constexpr int32_t SBT_IN_JUMP = 1 << 1;
constexpr int32_t SBT_IN_DUCK = 1 << 2;
constexpr int32_t SBT_IN_FORWARD = 1 << 3;
constexpr int32_t SBT_IN_BACK = 1 << 4;
constexpr int32_t SBT_IN_MOVELEFT = 1 << 9;
constexpr int32_t SBT_IN_MOVERIGHT = 1 << 10;
constexpr int32_t SBT_IN_SPEED = 1 << 17;
constexpr int32_t SBT_IN_WALK = 1 << 18;
constexpr float SBT_DEG_TO_RAD = 0.017453292519943295769f;
constexpr float SBT_NOCLIP_SPEED = 1600.0f;
constexpr float SBT_NOCLIP_FAST_MULTIPLIER = 1.75f;
constexpr float SBT_NOCLIP_SLOW_MULTIPLIER = 1.0f;
constexpr float SBT_FALLBACK_TICK_INTERVAL = 1.0f / 64.0f;
constexpr float RNGFIX_NON_JUMP_VELOCITY = 140.0f;
constexpr float RNGFIX_SURF_VERTICAL_VELOCITY = 96.0f;
constexpr int SBT_SURF_SUPPRESS_TICKS = 3;
constexpr float SBT_SURF_MIN_SPEED = 300.0f;
constexpr float SBT_SURF_MIN_VERTICAL_SPEED = 48.0f;
constexpr float SBT_SURF_VERTICAL_DROP_GRACE = -7.0f;
constexpr float SBT_SURF_REDIRECT_FALL_SPEED = -120.0f;
constexpr float SBT_SURF_REDIRECT_MIN_Z_GAIN = 48.0f;
constexpr float SBT_SURF_MAX_Z_GAIN_PER_TICK = 18.0f;
constexpr float SBT_SURF_MAX_EXTRA_ABOVE_GRAVITY = 56.0f;
constexpr float SBT_SURF_GRAVITY = 800.0f;
constexpr float SBT_TRIGGER_TOUCH_PADDING = 18.0f;
constexpr float SBT_TRIGGER_SWEEP_PADDING = 24.0f;
constexpr int SBT_BHOP_MODE_STANDARD = 0;
constexpr int SBT_BHOP_MODE_CLASSIC = 1;
constexpr float SBT_CLASSIC_MIN_STRAFE_SPEED = 80.0f;
constexpr float SBT_CLASSIC_MIN_YAW_DELTA = 0.012f;
constexpr float SBT_CLASSIC_MICRO_STRAFE_MIN_YAW_DELTA = 0.0015f;
constexpr float SBT_CLASSIC_CSS_TICK_INTERVAL = 0.01f;
constexpr float SBT_CLASSIC_CSS_AIR_ACCELERATE = 108.0f;
constexpr float SBT_CLASSIC_CSS_MAX_WISH_SPEED = 250.0f;
constexpr float SBT_CLASSIC_CSS_AIR_WISH_SPEED_CAP = 30.0f;
constexpr float SBT_CLASSIC_ASSISTED_WISH_PROJECTION_LOW_SPEED = 26.2f;
constexpr float SBT_CLASSIC_ASSISTED_WISH_PROJECTION_HIGH_SPEED = 24.0f;
constexpr float SBT_CLASSIC_ASSISTED_WISH_PROJECTION_VERY_HIGH_SPEED = 10.0f;
constexpr float SBT_CLASSIC_LOW_SPEED_ASSIST_END = 320.0f;
constexpr float SBT_CLASSIC_HIGH_SPEED_ASSIST_START = 1000.0f;
constexpr float SBT_CLASSIC_HIGH_SPEED_ASSIST_END = 1600.0f;
constexpr float SBT_CLASSIC_MIN_EFFECTIVE_WISH_PROJECTION = 4.0f;
constexpr float SBT_CLASSIC_MAX_REASONABLE_SPEED_DROP = 96.0f;
constexpr float SBT_CLASSIC_COLLISION_SPEED_DROP_MIN = 48.0f;
constexpr float SBT_CLASSIC_COLLISION_SPEED_DROP_RATIO = 0.08f;
constexpr float SBT_CLASSIC_COLLISION_DIRECTION_DOT_MAX = 0.985f;
constexpr float SBT_CLASSIC_MAX_CORRECTION_PER_FRAME = 30.0f;
constexpr float SBT_CLASSIC_STEERING_LOSS_MAX_CORRECTION_PER_FRAME = 2048.0f;
constexpr float SBT_CLASSIC_RESTORE_CURRENT_DIRECTION_MIN_SPEED = 64.0f;
constexpr int SBT_CLASSIC_MAX_AIR_STEPS_PER_FRAME = 3;
constexpr int SBT_CLASSIC_TINY_YAW_GRACE_FRAMES = 4;

#if defined _WIN32
constexpr uintptr_t GAME_ENTITY_SYSTEM_OFFSET = 88;
#else
constexpr uintptr_t GAME_ENTITY_SYSTEM_OFFSET = 80;
#endif

static CConVarRef<float> g_sbtMpTimelimit("mp_timelimit");
static CConVarRef<float> g_sbtMpRoundtime("mp_roundtime");
static CConVarRef<float> g_sbtMpRoundtimeDefuse("mp_roundtime_defuse");
static CConVarRef<float> g_sbtMpRoundtimeHostage("mp_roundtime_hostage");
static CConVarRef<CUtlString> g_sbtNextlevel("nextlevel");
static CConVarRef<bool> g_sbtSvJumpPrecisionEnable("sv_jump_precision_enable");
static CConVarRef<bool> g_sbtSvLegacyJump("sv_legacy_jump");
static CConVarRef<bool> g_sbtSvSubtickMovementViewAngles("sv_subtick_movement_view_angles");
static CVValue_t g_sbtHardcodedTimeLimit(MAX_TIMER_CONVAR_MINUTES);
static CVValue_t *g_sbtMpTimelimitValue = nullptr;
static CVValue_t *g_sbtMpRoundtimeValue = nullptr;
static CVValue_t *g_sbtMpRoundtimeDefuseValue = nullptr;
static CVValue_t *g_sbtMpRoundtimeHostageValue = nullptr;
static CVValue_t *g_sbtMpRoundtimeMaxValue = nullptr;
static CVValue_t *g_sbtMpRoundtimeDefuseMaxValue = nullptr;
static CVValue_t *g_sbtMpRoundtimeHostageMaxValue = nullptr;
static bool g_sbtTimeLimitMirrorInstalled = false;

struct SchemaFieldOffset
{
	int offset = 0;
	bool found = false;
};

static void NotifyNetworkStateChanged(CEntityInstance *entity, SchemaFieldOffset fieldOffset);

struct SbtVector3
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
};

class SbtSingleRecipientFilter final : public IRecipientFilter
{
public:
	explicit SbtSingleRecipientFilter(int slot)
	{
		if (slot >= 0 && slot < ABSOLUTE_PLAYER_LIMIT)
		{
			_recipients.Set(slot);
		}
	}

	NetChannelBufType_t GetNetworkBufType() const override
	{
		return BUF_RELIABLE;
	}

	bool IsInitMessage() const override
	{
		return false;
	}

	const CPlayerBitVec &GetRecipients() const override
	{
		return _recipients;
	}

	CPlayerSlot GetPredictedPlayerSlot() const override
	{
		return CPlayerSlot(-1);
	}

private:
	CPlayerBitVec _recipients;
};

class SbtBotProfile
{
public:
	const char *botName = "SwiftlyBhopTimerReplay";
	float aggression = 0.0f;
	float skill = 0.0f;
	float teamwork = 0.0f;
	float aimFocusInitial = 0.0f;
	float aimFocusDecay = 0.0f;
	float aimFocusOffsetScale = 0.0f;
	float aimFocusInterval = 0.0f;
	uint16_t weaponPreferences[16] = { 0xFFFF };
	int weaponPreferencesCount = 0;
	int cost = 0;
	int skin = 0;
	uint8_t difficultyFlags = 0;
	int voicePitch = 0;
	float reactionTime = 0.0f;
	float attackDelay = 0.0f;
	int teamNum = CS_TEAM_CT;
	bool preferSilencer = false;
	int voiceBank = 0;
	float lookAngleMaxAccelNormal = 0.0f;
	float lookAngleStiffnessNormal = 0.0f;
	float lookAngleDampingNormal = 0.0f;
	float lookAngleMaxAccelAttacking = 0.0f;
	float lookAngleStiffnessAttacking = 0.0f;
	float lookAngleDampingAttacking = 0.0f;
	CUtlVector<SbtBotProfile *> profileTemplates;

	static SbtBotProfile *PersistentProfile(int teamNum, const char *botName)
	{
		auto *profile = new SbtBotProfile();
		auto *stableName = new char[128];
		V_strncpy(stableName, botName && botName[0] ? botName : "SwiftlyBhopTimerReplay", 128);
		profile->teamNum = teamNum;
		profile->botName = stableName;
		return profile;
	}
};

static bool PatternMatches(const uint8_t *address, const int *pattern, size_t patternLength)
{
	for (size_t i = 0; i < patternLength; i++)
	{
		if (pattern[i] >= 0 && address[i] != static_cast<uint8_t>(pattern[i]))
		{
			return false;
		}
	}

	return true;
}

static void *FindPatternInRange(uintptr_t start, size_t size, const int *pattern, size_t patternLength)
{
	if (!start || size < patternLength)
	{
		return nullptr;
	}

	auto *bytes = reinterpret_cast<const uint8_t *>(start);
	for (size_t i = 0; i <= size - patternLength; i++)
	{
		if (PatternMatches(bytes + i, pattern, patternLength))
		{
			return const_cast<uint8_t *>(bytes + i);
		}
	}

	return nullptr;
}

#if defined _WIN32
static void *FindCreateBotSignature()
{
	static constexpr int pattern[] = {
		0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x6C, 0x24, 0x18, 0x57, 0x48, 0x81, 0xEC, 0xC0, 0x00,
		0x00, 0x00, 0x41, 0x0F, 0xB6, 0xD8
	};

	HMODULE module = GetModuleHandleA("server.dll");
	if (!module)
	{
		return nullptr;
	}

	auto *dos = reinterpret_cast<IMAGE_DOS_HEADER *>(module);
	auto *nt = reinterpret_cast<IMAGE_NT_HEADERS *>(reinterpret_cast<uint8_t *>(module) + dos->e_lfanew);
	return FindPatternInRange(reinterpret_cast<uintptr_t>(module), nt->OptionalHeader.SizeOfImage, pattern, sizeof(pattern) / sizeof(pattern[0]));
}

static void *FindSwitchTeamSignature()
{
	static constexpr int pattern[] = {
		0x40, 0x53, 0x57, 0x48, 0x81, 0xEC, -1, -1, -1, -1, 0x48, 0x8B, 0xD9, 0x8B, 0xFA
	};

	HMODULE module = GetModuleHandleA("server.dll");
	if (!module)
	{
		return nullptr;
	}

	auto *dos = reinterpret_cast<IMAGE_DOS_HEADER *>(module);
	auto *nt = reinterpret_cast<IMAGE_NT_HEADERS *>(reinterpret_cast<uint8_t *>(module) + dos->e_lfanew);
	return FindPatternInRange(reinterpret_cast<uintptr_t>(module), nt->OptionalHeader.SizeOfImage, pattern, sizeof(pattern) / sizeof(pattern[0]));
}
#else
struct ModuleScanContext
{
	const int *pattern;
	size_t patternLength;
	void *result;
};

static int ScanModuleCallback(struct dl_phdr_info *info, size_t, void *data)
{
	auto *context = reinterpret_cast<ModuleScanContext *>(data);
	if (!info || !info->dlpi_name || !std::strstr(info->dlpi_name, "libserver.so"))
	{
		return 0;
	}

	for (int i = 0; i < info->dlpi_phnum; i++)
	{
		const auto &header = info->dlpi_phdr[i];
		if (header.p_type != PT_LOAD || !(header.p_flags & PF_X))
		{
			continue;
		}

		auto start = static_cast<uintptr_t>(info->dlpi_addr + header.p_vaddr);
		context->result = FindPatternInRange(start, header.p_memsz, context->pattern, context->patternLength);
		if (context->result)
		{
			return 1;
		}
	}

	return 0;
}

static void *FindCreateBotSignature()
{
	static constexpr int pattern[] = {
		0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x49, 0x89, 0xFE, 0x41, 0x55, 0x41, 0x54, 0x41,
		0x89, 0xD4, 0x53, 0x48, 0x81, 0xEC, 0xC8, 0x00, 0x00, 0x00
	};

	ModuleScanContext context { pattern, sizeof(pattern) / sizeof(pattern[0]), nullptr };
	dl_iterate_phdr(ScanModuleCallback, &context);
	return context.result;
}

static void *FindSwitchTeamSignature()
{
	static constexpr int pattern[] = {
		0x55, 0x48, 0x89, 0xE5, 0x41, 0x54, 0x49, 0x89, 0xFC, 0x89, 0xF7
	};

	ModuleScanContext context { pattern, sizeof(pattern) / sizeof(pattern[0]), nullptr };
	dl_iterate_phdr(ScanModuleCallback, &context);
	return context.result;
}
#endif

static SchemaFieldOffset FindSchemaFieldOffsetRecursive(SchemaClassInfoData_t *classInfo, const char *fieldName)
{
	if (!classInfo || !fieldName || !fieldName[0])
	{
		return {};
	}

	for (int i = 0; i < classInfo->m_nFieldCount; i++)
	{
		SchemaClassFieldData_t &field = classInfo->m_pFields[i];
		if (field.m_pszName && V_strcmp(field.m_pszName, fieldName) == 0)
		{
			return { field.m_nSingleInheritanceOffset, true };
		}
	}

	for (int i = 0; i < classInfo->m_nBaseClassCount; i++)
	{
		SchemaFieldOffset baseOffset = FindSchemaFieldOffsetRecursive(classInfo->m_pBaseClasses[i].m_pClass, fieldName);
		if (baseOffset.found)
		{
			baseOffset.offset += classInfo->m_pBaseClasses[i].m_nOffset;
			return baseOffset;
		}
	}

	return {};
}

static SchemaFieldOffset FindSchemaFieldOffset(const char *className, const char *fieldName)
{
	static std::map<std::string, SchemaFieldOffset> cache;
	std::string cacheKey = std::string(className ? className : "") + "::" + (fieldName ? fieldName : "");
	auto found = cache.find(cacheKey);
	if (found != cache.end())
	{
		return found->second;
	}

	if (!g_pSchemaSystem)
	{
		cache[cacheKey] = {};
		return {};
	}

	CSchemaSystemTypeScope *typeScope = g_pSchemaSystem->FindTypeScopeForModule(SERVER_SCHEMA_MODULE);
	if (!typeScope)
	{
		cache[cacheKey] = {};
		return {};
	}

	SchemaMetaInfoHandle_t<CSchemaClassInfo> classHandle = typeScope->FindDeclaredClass(className);
	SchemaClassInfoData_t *classInfo = classHandle.Get();
	SchemaFieldOffset result = FindSchemaFieldOffsetRecursive(classInfo, fieldName);
	cache[cacheKey] = result;
	return result;
}

static CGameEntitySystem *GetGameEntitySystem()
{
	if (!g_pSbtGameResourceServiceServer)
	{
		return nullptr;
	}

	return *reinterpret_cast<CGameEntitySystem **>(reinterpret_cast<uintptr_t>(g_pSbtGameResourceServiceServer) + GAME_ENTITY_SYSTEM_OFFSET);
}

static CEntityIdentity *GetEntityIdentityByIndex(CGameEntitySystem *entitySystem, int index)
{
	if (!entitySystem || index < 0 || index >= MAX_TOTAL_ENTITIES)
	{
		return nullptr;
	}

	const int chunk = index / MAX_ENTITIES_IN_LIST;
	const int offset = index % MAX_ENTITIES_IN_LIST;
	CEntityIdentity *chunkBase = entitySystem->m_EntityList.m_pIdentityChunks[chunk];
	return chunkBase ? &chunkBase[offset] : nullptr;
}

static CEntityInstance *GetEntityInstanceByIndex(CGameEntitySystem *entitySystem, int index)
{
	CEntityIdentity *identity = GetEntityIdentityByIndex(entitySystem, index);
	return identity ? identity->m_pInstance : nullptr;
}

static CEntityInstance *GetEntityInstanceByHandle(CGameEntitySystem *entitySystem, const CEntityHandle &handle)
{
	if (!handle.IsValid())
	{
		return nullptr;
	}

	CEntityIdentity *identity = GetEntityIdentityByIndex(entitySystem, handle.GetEntryIndex());
	if (!identity || identity->m_EHandle.GetSerialNumber() != handle.GetSerialNumber())
	{
		return nullptr;
	}

	return identity->m_pInstance;
}

static CEntityInstance *GetControllerBySlot(int slot)
{
	CGameEntitySystem *entitySystem = GetGameEntitySystem();
	if (!entitySystem || slot < 0 || slot >= 64)
	{
		return nullptr;
	}

	return GetEntityInstanceByIndex(entitySystem, slot + 1);
}

static int FindControllerSlot(CEntityInstance *controller)
{
	if (!controller)
	{
		return -1;
	}

	for (int slot = 0; slot < 64; slot++)
	{
		if (GetControllerBySlot(slot) == controller)
		{
			return slot;
		}
	}

	return -1;
}

static CEntityInstance *GetPlayerPawnFromController(CEntityInstance *controller)
{
	CGameEntitySystem *entitySystem = GetGameEntitySystem();
	if (!controller || !entitySystem)
	{
		return nullptr;
	}

	SchemaFieldOffset pawnHandleOffset = FindSchemaFieldOffset("CCSPlayerController", "m_hPlayerPawn");
	if (!pawnHandleOffset.found)
	{
		return nullptr;
	}

	auto *pawnHandle = reinterpret_cast<CEntityHandle *>(reinterpret_cast<uint8_t *>(controller) + pawnHandleOffset.offset);
	if (!pawnHandle->IsValid())
	{
		return nullptr;
	}

	return GetEntityInstanceByHandle(entitySystem, *pawnHandle);
}

static void *GetPawnObserverServices(CEntityInstance *pawn)
{
	if (!pawn)
	{
		return nullptr;
	}

	for (const char *schemaClass : { "CCSPlayerPawn", "CBasePlayerPawn" })
	{
		SchemaFieldOffset observerServicesOffset = FindSchemaFieldOffset(schemaClass, "m_pObserverServices");
		if (!observerServicesOffset.found)
		{
			continue;
		}

		return *reinterpret_cast<void **>(reinterpret_cast<uint8_t *>(pawn) + observerServicesOffset.offset);
	}

	return nullptr;
}

static bool SetObserverServicesTarget(void *observerServices, CEntityInstance *viewerPawn, CEntityInstance *targetPawn)
{
	if (!observerServices || !viewerPawn || !targetPawn)
	{
		return false;
	}

	SchemaFieldOffset targetOffset = FindSchemaFieldOffset("CPlayer_ObserverServices", "m_hObserverTarget");
	if (!targetOffset.found)
	{
		return false;
	}

	auto *observerTarget = reinterpret_cast<CEntityHandle *>(reinterpret_cast<uint8_t *>(observerServices) + targetOffset.offset);
	*observerTarget = targetPawn->GetRefEHandle();
	NotifyNetworkStateChanged(viewerPawn, {});
	return true;
}

static bool SetObserverServicesMode(void *observerServices, CEntityInstance *viewerPawn, uint8_t observerMode)
{
	if (!observerServices || !viewerPawn)
	{
		return false;
	}

	SchemaFieldOffset modeOffset = FindSchemaFieldOffset("CPlayer_ObserverServices", "m_iObserverMode");
	if (!modeOffset.found)
	{
		return false;
	}

	*reinterpret_cast<uint8_t *>(reinterpret_cast<uint8_t *>(observerServices) + modeOffset.offset) = observerMode;
	NotifyNetworkStateChanged(viewerPawn, {});
	return true;
}

static bool SetPawnFloatField(CEntityInstance *pawn, const char *fieldName, float value)
{
	if (!pawn)
	{
		return false;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset("CCSPlayerPawn", fieldName);
	if (!fieldOffset.found)
	{
		return false;
	}

	*reinterpret_cast<float *>(reinterpret_cast<uint8_t *>(pawn) + fieldOffset.offset) = value;
	pawn->NetworkStateChanged(NetworkStateChangedData(static_cast<uint32>(fieldOffset.offset)));
	return true;
}

static bool SetEntityBoolField(CEntityInstance *entity, const char *schemaClass, const char *fieldName, bool value)
{
	if (!entity)
	{
		return false;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset(schemaClass, fieldName);
	if (!fieldOffset.found)
	{
		return false;
	}

	auto *field = reinterpret_cast<bool *>(reinterpret_cast<uint8_t *>(entity) + fieldOffset.offset);
	if (*field == value)
	{
		return true;
	}

	*field = value;
	entity->NetworkStateChanged(NetworkStateChangedData(static_cast<uint32>(fieldOffset.offset)));
	entity->NetworkStateChanged(NetworkStateChangedData(true));
	return true;
}

static bool SetEntityInt32Field(CEntityInstance *entity, const char *schemaClass, const char *fieldName, int32_t value)
{
	if (!entity)
	{
		return false;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset(schemaClass, fieldName);
	if (!fieldOffset.found)
	{
		return false;
	}

	auto *field = reinterpret_cast<int32_t *>(reinterpret_cast<uint8_t *>(entity) + fieldOffset.offset);
	if (*field == value)
	{
		return true;
	}

	*field = value;
	entity->NetworkStateChanged(NetworkStateChangedData(static_cast<uint32>(fieldOffset.offset)));
	entity->NetworkStateChanged(NetworkStateChangedData(true));
	return true;
}

static bool SetEntityUInt8Field(CEntityInstance *entity, const char *schemaClass, const char *fieldName, uint8_t value)
{
	if (!entity)
	{
		return false;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset(schemaClass, fieldName);
	if (!fieldOffset.found)
	{
		return false;
	}

	auto *field = reinterpret_cast<uint8_t *>(reinterpret_cast<uint8_t *>(entity) + fieldOffset.offset);
	if (*field == value)
	{
		return true;
	}

	*field = value;
	entity->NetworkStateChanged(NetworkStateChangedData(static_cast<uint32>(fieldOffset.offset)));
	entity->NetworkStateChanged(NetworkStateChangedData(true));
	return true;
}

static bool ContainsCaseInsensitive(const char *value, const char *needle)
{
	if (!value || !needle || !needle[0])
	{
		return false;
	}

	std::string source(value);
	std::string target(needle);
	std::transform(source.begin(), source.end(), source.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	std::transform(target.begin(), target.end(), target.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return source.find(target) != std::string::npos;
}

static void NotifyNetworkStateChanged(CEntityInstance *entity, SchemaFieldOffset fieldOffset)
{
	if (!entity)
	{
		return;
	}

	if (fieldOffset.found)
	{
		entity->NetworkStateChanged(NetworkStateChangedData(static_cast<uint32>(fieldOffset.offset)));
	}

	entity->NetworkStateChanged(NetworkStateChangedData(true));
}

static bool IsFiniteVector3(const SbtVector3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

static bool SetRawVector3Field(void *target, CEntityInstance *networkEntity, const char *schemaClass, const char *fieldName, const SbtVector3 &value)
{
	if (!target || !IsFiniteVector3(value))
	{
		return false;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset(schemaClass, fieldName);
	if (!fieldOffset.found)
	{
		return false;
	}

	auto *field = reinterpret_cast<float *>(reinterpret_cast<uint8_t *>(target) + fieldOffset.offset);
	field[0] = value.x;
	field[1] = value.y;
	field[2] = value.z;
	NotifyNetworkStateChanged(networkEntity, fieldOffset);
	return true;
}

static bool ReadRawVector3Field(void *target, const char *schemaClass, const char *fieldName, SbtVector3 &value)
{
	if (!target)
	{
		return false;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset(schemaClass, fieldName);
	if (!fieldOffset.found)
	{
		return false;
	}

	auto *field = reinterpret_cast<float *>(reinterpret_cast<uint8_t *>(target) + fieldOffset.offset);
	value = { field[0], field[1], field[2] };
	return IsFiniteVector3(value);
}

static bool SetEntityVector3Field(CEntityInstance *entity, const char *schemaClass, const char *fieldName, const SbtVector3 &value)
{
	return SetRawVector3Field(entity, entity, schemaClass, fieldName, value);
}

static bool ReadEntityVector3Field(CEntityInstance *entity, const char *schemaClass, const char *fieldName, SbtVector3 &value)
{
	return ReadRawVector3Field(entity, schemaClass, fieldName, value);
}

static void *GetRawPointerField(void *target, const char *schemaClass, const char *fieldName)
{
	if (!target)
	{
		return nullptr;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset(schemaClass, fieldName);
	if (!fieldOffset.found)
	{
		return nullptr;
	}

	return *reinterpret_cast<void **>(reinterpret_cast<uint8_t *>(target) + fieldOffset.offset);
}

static void *GetRawFieldAddress(void *target, const char *schemaClass, const char *fieldName)
{
	if (!target)
	{
		return nullptr;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset(schemaClass, fieldName);
	if (!fieldOffset.found)
	{
		return nullptr;
	}

	return reinterpret_cast<uint8_t *>(target) + fieldOffset.offset;
}

static bool AddEntityInt32Flags(CEntityInstance *entity, const char *schemaClass, const char *fieldName, int32_t flags)
{
	if (!entity)
	{
		return false;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset(schemaClass, fieldName);
	if (!fieldOffset.found)
	{
		return false;
	}

	auto *field = reinterpret_cast<int32_t *>(reinterpret_cast<uint8_t *>(entity) + fieldOffset.offset);
	const int32_t updated = *field | flags;
	if (*field == updated)
	{
		return true;
	}

	*field = updated;
	NotifyNetworkStateChanged(entity, fieldOffset);
	return true;
}

static bool SetRawBoolField(void *target, CEntityInstance *networkEntity, const char *schemaClass, const char *fieldName, bool value)
{
	if (!target)
	{
		return false;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset(schemaClass, fieldName);
	if (!fieldOffset.found)
	{
		return false;
	}

	*reinterpret_cast<bool *>(reinterpret_cast<uint8_t *>(target) + fieldOffset.offset) = value;
	NotifyNetworkStateChanged(networkEntity, fieldOffset);
	return true;
}

static bool SetRawInt32Field(void *target, CEntityInstance *networkEntity, const char *schemaClass, const char *fieldName, int32_t value)
{
	if (!target)
	{
		return false;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset(schemaClass, fieldName);
	if (!fieldOffset.found)
	{
		return false;
	}

	*reinterpret_cast<int32_t *>(reinterpret_cast<uint8_t *>(target) + fieldOffset.offset) = value;
	NotifyNetworkStateChanged(networkEntity, fieldOffset);
	return true;
}

static bool SetRawFloatField(void *target, CEntityInstance *networkEntity, const char *schemaClass, const char *fieldName, float value)
{
	if (!target || !std::isfinite(value))
	{
		return false;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset(schemaClass, fieldName);
	if (!fieldOffset.found)
	{
		return false;
	}

	*reinterpret_cast<float *>(reinterpret_cast<uint8_t *>(target) + fieldOffset.offset) = value;
	NotifyNetworkStateChanged(networkEntity, fieldOffset);
	return true;
}

static bool ReadRawFloatField(void *target, const char *schemaClass, const char *fieldName, float &value)
{
	if (!target)
	{
		return false;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset(schemaClass, fieldName);
	if (!fieldOffset.found)
	{
		return false;
	}

	value = *reinterpret_cast<float *>(reinterpret_cast<uint8_t *>(target) + fieldOffset.offset);
	return std::isfinite(value);
}

static bool ReadRawInt32Field(void *target, const char *schemaClass, const char *fieldName, int32_t &value)
{
	if (!target)
	{
		return false;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset(schemaClass, fieldName);
	if (!fieldOffset.found)
	{
		return false;
	}

	value = *reinterpret_cast<int32_t *>(reinterpret_cast<uint8_t *>(target) + fieldOffset.offset);
	return true;
}

static bool ReadRawUInt64Field(void *target, const char *schemaClass, const char *fieldName, uint64_t &value)
{
	if (!target)
	{
		return false;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset(schemaClass, fieldName);
	if (!fieldOffset.found)
	{
		return false;
	}

	value = *reinterpret_cast<uint64_t *>(reinterpret_cast<uint8_t *>(target) + fieldOffset.offset);
	return true;
}

static bool ReadEntityFloatField(CEntityInstance *entity, const char *schemaClass, const char *fieldName, float &value)
{
	return ReadRawFloatField(entity, schemaClass, fieldName, value);
}

static bool ReadEntityInt32Field(CEntityInstance *entity, const char *schemaClass, const char *fieldName, int32_t &value)
{
	return ReadRawInt32Field(entity, schemaClass, fieldName, value);
}

static bool ReadEntityUInt8Field(CEntityInstance *entity, const char *schemaClass, const char *fieldName, uint8_t &value)
{
	if (!entity)
	{
		return false;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset(schemaClass, fieldName);
	if (!fieldOffset.found)
	{
		return false;
	}

	value = *reinterpret_cast<uint8_t *>(reinterpret_cast<uint8_t *>(entity) + fieldOffset.offset);
	return true;
}

static bool ReadEntityFloatAny(CEntityInstance *entity, std::initializer_list<const char *> schemaClasses, std::initializer_list<const char *> fieldNames, float &value)
{
	for (const char *schemaClass : schemaClasses)
	{
		for (const char *fieldName : fieldNames)
		{
			if (ReadEntityFloatField(entity, schemaClass, fieldName, value))
			{
				return true;
			}
		}
	}

	return false;
}

static bool ReadEntityVector3Any(CEntityInstance *entity, std::initializer_list<const char *> schemaClasses, std::initializer_list<const char *> fieldNames, SbtVector3 &value)
{
	for (const char *schemaClass : schemaClasses)
	{
		for (const char *fieldName : fieldNames)
		{
			if (ReadEntityVector3Field(entity, schemaClass, fieldName, value))
			{
				return true;
			}
		}
	}

	return false;
}

static bool ReadEntityInt32Any(CEntityInstance *entity, std::initializer_list<const char *> schemaClasses, std::initializer_list<const char *> fieldNames, int32_t &value)
{
	for (const char *schemaClass : schemaClasses)
	{
		for (const char *fieldName : fieldNames)
		{
			if (ReadEntityInt32Field(entity, schemaClass, fieldName, value))
			{
				return true;
			}
		}
	}

	return false;
}

static bool SetEntityVector3Any(CEntityInstance *entity, std::initializer_list<const char *> schemaClasses, std::initializer_list<const char *> fieldNames, const SbtVector3 &value)
{
	bool applied = false;
	for (const char *schemaClass : schemaClasses)
	{
		for (const char *fieldName : fieldNames)
		{
			applied |= SetEntityVector3Field(entity, schemaClass, fieldName, value);
		}
	}

	return applied;
}

static bool SetEntityFloatAny(CEntityInstance *entity, std::initializer_list<const char *> schemaClasses, std::initializer_list<const char *> fieldNames, float value)
{
	bool applied = false;
	for (const char *schemaClass : schemaClasses)
	{
		for (const char *fieldName : fieldNames)
		{
			applied |= SetRawFloatField(entity, entity, schemaClass, fieldName, value);
		}
	}

	return applied;
}

static float VectorLength3D(const SbtVector3 &value)
{
	return std::sqrt((value.x * value.x) + (value.y * value.y) + (value.z * value.z));
}

static float VectorLength2D(const SbtVector3 &value)
{
	return std::sqrt((value.x * value.x) + (value.y * value.y));
}

static float NormalizeAngleDelta(float angle)
{
	while (angle > 180.0f)
	{
		angle -= 360.0f;
	}
	while (angle < -180.0f)
	{
		angle += 360.0f;
	}
	return angle;
}

static float GetServerFrameTime()
{
	CGlobalVars *globals = g_SMAPI ? g_SMAPI->GetCGlobals() : nullptr;
	if (!globals || !std::isfinite(globals->frametime) || globals->frametime <= 0.0f || globals->frametime > 0.1f)
	{
		return SBT_FALLBACK_TICK_INTERVAL;
	}

	return globals->frametime;
}

static bool NormalizeVector3(SbtVector3 &value)
{
	const float length = VectorLength3D(value);
	if (length <= 0.001f || !std::isfinite(length))
	{
		return false;
	}

	value.x /= length;
	value.y /= length;
	value.z /= length;
	return IsFiniteVector3(value);
}

static bool TryNormalizeTriggerPushDirection(const SbtVector3 &raw, SbtVector3 &direction)
{
	if (!IsFiniteVector3(raw))
	{
		return false;
	}

	direction = raw;
	const float vectorLength = VectorLength3D(direction);
	if (vectorLength > 0.001f && vectorLength <= 4.0f)
	{
		return NormalizeVector3(direction);
	}

	if (std::fabs(raw.x) > 360.0f || std::fabs(raw.y) > 360.0f || std::fabs(raw.z) > 360.0f)
	{
		return false;
	}

	const float pitch = raw.x * SBT_DEG_TO_RAD;
	const float yaw = raw.y * SBT_DEG_TO_RAD;
	const float sp = std::sin(pitch);
	const float cp = std::cos(pitch);
	const float sy = std::sin(yaw);
	const float cy = std::cos(yaw);
	direction = { cp * cy, cp * sy, -sp };
	return NormalizeVector3(direction);
}

static bool ReadTriggerPushDirection(CEntityInstance *entity, SbtVector3 &direction)
{
	const char *schemaClasses[] = { "CTriggerPush", "CBaseTrigger", "CBaseEntity" };
	const char *fieldNames[] = {
		"m_vecPushDirEntitySpace",
		"m_vecPushDir",
		"m_vecPushDirection",
		"m_vecPushEntitySpace",
		"m_angPushEntitySpace",
		"m_angPushAngle",
		"m_angPushDir",
		"m_angPushDirection"
	};

	for (const char *schemaClass : schemaClasses)
	{
		for (const char *fieldName : fieldNames)
		{
			SbtVector3 raw {};
			if (ReadEntityVector3Field(entity, schemaClass, fieldName, raw) && TryNormalizeTriggerPushDirection(raw, direction))
			{
				return true;
			}
		}
	}

	return false;
}

static const char *GetEntityTargetName(CEntityInstance *entity)
{
	if (!entity || !entity->m_pEntity)
	{
		return "";
	}

	const char *name = entity->m_pEntity->m_name.String();
	return name ? name : "";
}

static bool GetEntityOrigin(CEntityInstance *entity, SbtVector3 &origin)
{
	if (!entity)
	{
		return false;
	}

	if (ReadEntityVector3Any(entity, { "CBaseEntity", "CBaseModelEntity", "CBaseTrigger" }, { "m_vecAbsOrigin", "m_vecOrigin" }, origin))
	{
		return true;
	}

	void *sceneNode = GetRawPointerField(entity, "CBaseEntity", "m_pGameSceneNode");
	return ReadRawVector3Field(sceneNode, "CGameSceneNode", "m_vecAbsOrigin", origin)
		|| ReadRawVector3Field(sceneNode, "CGameSceneNode", "m_vecOrigin", origin);
}

static bool GetEntityCollisionExtents(CEntityInstance *entity, SbtVector3 &mins, SbtVector3 &maxs)
{
	if (!entity)
	{
		return false;
	}

	void *collision = GetRawFieldAddress(entity, "CBaseEntity", "m_Collision");
	if (!collision)
	{
		collision = GetRawFieldAddress(entity, "CBaseModelEntity", "m_Collision");
	}
	if (!collision)
	{
		collision = GetRawFieldAddress(entity, "CBaseTrigger", "m_Collision");
	}
	if (collision && ReadRawVector3Field(collision, "CCollisionProperty", "m_vecMins", mins)
		&& ReadRawVector3Field(collision, "CCollisionProperty", "m_vecMaxs", maxs))
	{
		return true;
	}

	return ReadEntityVector3Any(entity, { "CBaseModelEntity", "CBaseEntity", "CBaseTrigger" }, { "m_vecMins" }, mins)
		&& ReadEntityVector3Any(entity, { "CBaseModelEntity", "CBaseEntity", "CBaseTrigger" }, { "m_vecMaxs" }, maxs);
}

static bool GetEntityWorldBounds(CEntityInstance *entity, SbtVector3 &mins, SbtVector3 &maxs)
{
	SbtVector3 origin {};
	SbtVector3 localMins {};
	SbtVector3 localMaxs {};
	if (!GetEntityOrigin(entity, origin) || !GetEntityCollisionExtents(entity, localMins, localMaxs))
	{
		return false;
	}

	mins = {
		origin.x + (std::min)(localMins.x, localMaxs.x),
		origin.y + (std::min)(localMins.y, localMaxs.y),
		origin.z + (std::min)(localMins.z, localMaxs.z),
	};
	maxs = {
		origin.x + (std::max)(localMins.x, localMaxs.x),
		origin.y + (std::max)(localMins.y, localMaxs.y),
		origin.z + (std::max)(localMins.z, localMaxs.z),
	};

	const float sizeX = maxs.x - mins.x;
	const float sizeY = maxs.y - mins.y;
	const float sizeZ = maxs.z - mins.z;
	return IsFiniteVector3(mins) && IsFiniteVector3(maxs) && sizeX >= 1.0f && sizeY >= 1.0f && sizeZ >= 1.0f;
}

static bool PointInsideBounds(const SbtVector3 &position, const SbtVector3 &mins, const SbtVector3 &maxs, float padding = 0.0f)
{
	return position.x >= mins.x - padding && position.x <= maxs.x + padding
		&& position.y >= mins.y - padding && position.y <= maxs.y + padding
		&& position.z >= mins.z - padding && position.z <= maxs.z + padding;
}

static bool SegmentIntersectsBounds(const SbtVector3 &start, const SbtVector3 &end, const SbtVector3 &mins, const SbtVector3 &maxs, float padding = 0.0f)
{
	if (PointInsideBounds(start, mins, maxs, padding) || PointInsideBounds(end, mins, maxs, padding))
	{
		return true;
	}

	float tMin = 0.0f;
	float tMax = 1.0f;
	auto updateAxis = [&](float startValue, float endValue, float minValue, float maxValue) {
		const float expandedMin = minValue - padding;
		const float expandedMax = maxValue + padding;
		const float delta = endValue - startValue;
		if (std::fabs(delta) <= 0.000001f)
		{
			return startValue >= expandedMin && startValue <= expandedMax;
		}

		float enter = (expandedMin - startValue) / delta;
		float exit = (expandedMax - startValue) / delta;
		if (enter > exit)
		{
			std::swap(enter, exit);
		}

		tMin = (std::max)(tMin, enter);
		tMax = (std::min)(tMax, exit);
		return tMin <= tMax;
	};

	return updateAxis(start.x, end.x, mins.x, maxs.x)
		&& updateAxis(start.y, end.y, mins.y, maxs.y)
		&& updateAxis(start.z, end.z, mins.z, maxs.z)
		&& tMax >= 0.0f
		&& tMin <= 1.0f;
}

static bool EntityHandleEquals(const CEntityHandle &left, const CEntityHandle &right)
{
	return left.IsValid()
		&& right.IsValid()
		&& left.GetEntryIndex() == right.GetEntryIndex()
		&& left.GetSerialNumber() == right.GetSerialNumber();
}

static void *GetGameRulesPointer(CEntityInstance *proxy)
{
	if (!proxy)
	{
		return nullptr;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset("CCSGameRulesProxy", "m_pGameRules");
	if (!fieldOffset.found)
	{
		return nullptr;
	}

	return *reinterpret_cast<void **>(reinterpret_cast<uint8_t *>(proxy) + fieldOffset.offset);
}

static CEntityInstance *FindGameRulesProxy()
{
	CGameEntitySystem *entitySystem = GetGameEntitySystem();
	if (!entitySystem)
	{
		return nullptr;
	}

	if (!FindSchemaFieldOffset("CCSGameRulesProxy", "m_pGameRules").found)
	{
		return nullptr;
	}

	for (int entityIndex = 0; entityIndex < MAX_TOTAL_ENTITIES; entityIndex++)
	{
		CEntityInstance *entity = GetEntityInstanceByIndex(entitySystem, entityIndex);
		if (!entity)
		{
			continue;
		}

		const char *className = entity->GetClassname();
		if (!ContainsCaseInsensitive(className, "gamerules"))
		{
			continue;
		}

		if (GetGameRulesPointer(entity))
		{
			return entity;
		}
	}

	return nullptr;
}

static bool AreTimeLimitConVarsAvailable()
{
	return g_sbtMpTimelimit.IsValidRef() && g_sbtMpTimelimit.IsConVarDataAvailable()
		&& g_sbtMpRoundtime.IsValidRef() && g_sbtMpRoundtime.IsConVarDataAvailable()
		&& g_sbtMpRoundtimeDefuse.IsValidRef() && g_sbtMpRoundtimeDefuse.IsConVarDataAvailable()
		&& g_sbtMpRoundtimeHostage.IsValidRef() && g_sbtMpRoundtimeHostage.IsConVarDataAvailable();
}

static std::string GetDefaultMapName()
{
	std::string commandLine = CommandLine()->GetCmdLine();
	size_t pos = commandLine.find("+map ");
	if (pos != std::string::npos)
	{
		size_t start = pos + 5;
		size_t end = commandLine.find(' ', start);
		if (end == std::string::npos)
		{
			end = commandLine.length();
		}

		std::string mapName = commandLine.substr(start, end - start);
		mapName.erase(std::remove(mapName.begin(), mapName.end(), '"'), mapName.end());
		return mapName;
	}

	return "de_dust2";
}

static void EnsureNextLevel()
{
	if (g_sbtNextlevel.IsValidRef() && g_sbtNextlevel.IsConVarDataAvailable() && g_sbtNextlevel.Get().IsEmpty())
	{
		g_sbtNextlevel.Set(GetDefaultMapName().c_str());
	}
}

static bool IsMirroredTimeLimitConVar(ConVarRefAbstract *ref)
{
	if (!ref || !AreTimeLimitConVarsAvailable())
	{
		return false;
	}

	const uint16 refIndex = ref->GetAccessIndex();
	return g_sbtMpTimelimit.GetAccessIndex() == refIndex
		|| g_sbtMpRoundtime.GetAccessIndex() == refIndex
		|| g_sbtMpRoundtimeDefuse.GetAccessIndex() == refIndex
		|| g_sbtMpRoundtimeHostage.GetAccessIndex() == refIndex;
}

static void ReflectRoundTimeToGameRules(const char *value)
{
	if (!value || !value[0])
	{
		return;
	}

	const float minutes = static_cast<float>(std::atof(value));
	if (!std::isfinite(minutes) || minutes <= 0.0f)
	{
		return;
	}

	CEntityInstance *gameRulesProxy = FindGameRulesProxy();
	void *gameRules = GetGameRulesPointer(gameRulesProxy);
	if (!gameRules)
	{
		return;
	}

	const int32_t roundTimeSeconds = static_cast<int32_t>(std::ceil(minutes * 60.0f));
	if (SetRawInt32Field(gameRules, gameRulesProxy, "CCSGameRules", "m_iRoundTime", roundTimeSeconds))
	{
		NotifyNetworkStateChanged(gameRulesProxy, {});
	}
}

static void ApplyMirroredTimeLimit(float minutes, const char *reason)
{
	if (!AreTimeLimitConVarsAvailable())
	{
		SbtMessage("Time limit apply skipped: one or more time convars are unavailable. Reason=%s\n", reason ? reason : "unknown");
		return;
	}

	if (!std::isfinite(minutes) || minutes <= 0.0f)
	{
		SbtMessage("Time limit apply skipped: invalid minutes %.3f. Reason=%s\n", minutes, reason ? reason : "unknown");
		return;
	}

	g_sbtMpTimelimit.Set(minutes);
	g_sbtMpRoundtime.Set(minutes);
	g_sbtMpRoundtimeDefuse.Set(minutes);
	g_sbtMpRoundtimeHostage.Set(minutes);

	char value[32];
	std::snprintf(value, sizeof(value), "%.3f", minutes);
	ReflectRoundTimeToGameRules(value);

	SbtMessage("Time limit applied: %.3f minutes. Reason=%s\n", minutes, reason ? reason : "unknown");
}

static void MirrorTimeLimitConVarChanged(ConVarRefAbstract *ref, CSplitScreenSlot, const char *pNewValue, const char *, void *)
{
	if (!ref)
	{
		return;
	}

	if (g_sbtNextlevel.IsValidRef()
		&& g_sbtNextlevel.IsConVarDataAvailable()
		&& g_sbtNextlevel.GetAccessIndex() == ref->GetAccessIndex()
		&& g_sbtNextlevel.Get().IsEmpty())
	{
		g_sbtNextlevel.Set(GetDefaultMapName().c_str());
		return;
	}

	if (!IsMirroredTimeLimitConVar(ref) || !pNewValue || !pNewValue[0])
	{
		return;
	}

	ConVarRefAbstract *timeConvars[] = {
		&g_sbtMpTimelimit,
		&g_sbtMpRoundtime,
		&g_sbtMpRoundtimeDefuse,
		&g_sbtMpRoundtimeHostage
	};

	for (ConVarRefAbstract *convar : timeConvars)
	{
		if (!convar->IsFlagSet(FCVAR_PERFORMING_CALLBACKS))
		{
			convar->SetString(pNewValue);
		}
	}

	ReflectRoundTimeToGameRules(pNewValue);
	SbtMessage("Time limit mirror callback applied: %s minutes.\n", pNewValue);
}

static void InstallTimeLimitMirror()
{
	if (!g_pCVar || g_sbtTimeLimitMirrorInstalled)
	{
		return;
	}

	EnsureNextLevel();

	if (!AreTimeLimitConVarsAvailable())
	{
		SbtMessage("Time limit mirror unavailable: one or more time convars are missing.\n");
		return;
	}

	g_sbtMpTimelimitValue = g_sbtMpTimelimit.GetConVarData()->Value(-1);
	g_sbtMpRoundtimeValue = g_sbtMpRoundtime.GetConVarData()->Value(-1);
	g_sbtMpRoundtimeDefuseValue = g_sbtMpRoundtimeDefuse.GetConVarData()->Value(-1);
	g_sbtMpRoundtimeHostageValue = g_sbtMpRoundtimeHostage.GetConVarData()->Value(-1);
	g_sbtMpRoundtimeMaxValue = g_sbtMpRoundtime.GetConVarData()->MaxValue();
	g_sbtMpRoundtimeDefuseMaxValue = g_sbtMpRoundtimeDefuse.GetConVarData()->MaxValue();
	g_sbtMpRoundtimeHostageMaxValue = g_sbtMpRoundtimeHostage.GetConVarData()->MaxValue();

	g_sbtMpTimelimit.GetConVarData()->SetMaxValue(&g_sbtHardcodedTimeLimit);
	g_sbtMpRoundtime.GetConVarData()->SetMaxValue(&g_sbtHardcodedTimeLimit);
	g_sbtMpRoundtimeDefuse.GetConVarData()->SetMaxValue(&g_sbtHardcodedTimeLimit);
	g_sbtMpRoundtimeHostage.GetConVarData()->SetMaxValue(&g_sbtHardcodedTimeLimit);
	g_pCVar->InstallGlobalChangeCallback(MirrorTimeLimitConVarChanged);
	g_sbtTimeLimitMirrorInstalled = true;
	SbtMessage("Time limit mirror installed.\n");
}

static void RemoveTimeLimitMirror()
{
	if (!g_pCVar || !g_sbtTimeLimitMirrorInstalled)
	{
		return;
	}

	g_sbtMpTimelimit.GetConVarData()->RemoveMaxValue();
	if (g_sbtMpRoundtimeMaxValue)
	{
		g_sbtMpRoundtime.GetConVarData()->SetMaxValue(g_sbtMpRoundtimeMaxValue);
	}
	if (g_sbtMpRoundtimeDefuseMaxValue)
	{
		g_sbtMpRoundtimeDefuse.GetConVarData()->SetMaxValue(g_sbtMpRoundtimeDefuseMaxValue);
	}
	if (g_sbtMpRoundtimeHostageMaxValue)
	{
		g_sbtMpRoundtimeHostage.GetConVarData()->SetMaxValue(g_sbtMpRoundtimeHostageMaxValue);
	}

	g_pCVar->RemoveGlobalChangeCallback(MirrorTimeLimitConVarChanged);
	g_sbtTimeLimitMirrorInstalled = false;
}

static bool SetControllerConnectedState(CEntityInstance *controller)
{
	if (!controller)
	{
		return false;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset("CBasePlayerController", "m_iConnected");
	if (!fieldOffset.found)
	{
		return false;
	}

	auto *field = reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(controller) + fieldOffset.offset);
	if (*field == 0)
	{
		return true;
	}

	*field = 0;
	controller->NetworkStateChanged(NetworkStateChangedData(static_cast<uint32>(fieldOffset.offset)));
	controller->NetworkStateChanged(NetworkStateChangedData(true));
	return true;
}

static bool SetControllerPlayerName(CEntityInstance *controller, const char *name)
{
	if (!controller || !name || !name[0])
	{
		return false;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset("CBasePlayerController", "m_iszPlayerName");
	if (!fieldOffset.found)
	{
		return false;
	}

	auto *field = reinterpret_cast<char *>(reinterpret_cast<uint8_t *>(controller) + fieldOffset.offset);
	V_strncpy(field, name, 128);
	controller->NetworkStateChanged(NetworkStateChangedData(static_cast<uint32>(fieldOffset.offset)));
	controller->NetworkStateChanged(NetworkStateChangedData(true));
	return true;
}

static const char *GetControllerPlayerName(CEntityInstance *controller)
{
	if (!controller)
	{
		return nullptr;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset("CBasePlayerController", "m_iszPlayerName");
	if (!fieldOffset.found)
	{
		return nullptr;
	}

	return reinterpret_cast<const char *>(reinterpret_cast<uint8_t *>(controller) + fieldOffset.offset);
}

static bool ControllerNameMatches(CEntityInstance *controller, const std::string &name)
{
	const char *controllerName = GetControllerPlayerName(controller);
	return controllerName && controllerName[0] && V_strcmp(controllerName, name.c_str()) == 0;
}

static bool GetPawnFloatField(CEntityInstance *pawn, const char *fieldName, float &value)
{
	if (!pawn)
	{
		return false;
	}

	SchemaFieldOffset fieldOffset = FindSchemaFieldOffset("CCSPlayerPawn", fieldName);
	if (!fieldOffset.found)
	{
		return false;
	}

	value = *reinterpret_cast<float *>(reinterpret_cast<uint8_t *>(pawn) + fieldOffset.offset);
	return true;
}

static bool IsHiddenViewmodelValue(float value)
{
	return value <= HIDEFPS_OFFSET_Y * 0.5f;
}

static void *GetPawnWeaponServices(CEntityInstance *pawn)
{
	if (!pawn)
	{
		return nullptr;
	}

	SchemaFieldOffset weaponServicesOffset = FindSchemaFieldOffset("CCSPlayerPawn", "m_pWeaponServices");
	if (!weaponServicesOffset.found)
	{
		return nullptr;
	}

	return *reinterpret_cast<void **>(reinterpret_cast<uint8_t *>(pawn) + weaponServicesOffset.offset);
}

static CUtlVector<CEntityHandle> *GetPawnWeaponHandles(CEntityInstance *pawn)
{
	void *weaponServices = GetPawnWeaponServices(pawn);
	if (!weaponServices)
	{
		return nullptr;
	}

	SchemaFieldOffset myWeaponsOffset = FindSchemaFieldOffset("CPlayer_WeaponServices", "m_hMyWeapons");
	if (!myWeaponsOffset.found)
	{
		return nullptr;
	}

	return reinterpret_cast<CUtlVector<CEntityHandle> *>(reinterpret_cast<uint8_t *>(weaponServices) + myWeaponsOffset.offset);
}

static CEntityHandle *GetPawnViewModelHandles(CEntityInstance *pawn)
{
	if (!pawn)
	{
		return nullptr;
	}

	SchemaFieldOffset viewModelServicesOffset = FindSchemaFieldOffset("CCSPlayerPawn", "m_pViewModelServices");
	if (!viewModelServicesOffset.found)
	{
		return nullptr;
	}

	void *viewModelServices = *reinterpret_cast<void **>(reinterpret_cast<uint8_t *>(pawn) + viewModelServicesOffset.offset);
	if (!viewModelServices)
	{
		return nullptr;
	}

	SchemaFieldOffset viewModelOffset = FindSchemaFieldOffset("CCSPlayer_ViewModelServices", "m_hViewModel");
	if (!viewModelOffset.found)
	{
		return nullptr;
	}

	return reinterpret_cast<CEntityHandle *>(reinterpret_cast<uint8_t *>(viewModelServices) + viewModelOffset.offset);
}

static void ClearTransmitEntity(CBitVec<16384> *transmitEdict, const CEntityHandle &handle)
{
	if (!transmitEdict || !handle.IsValid())
	{
		return;
	}

	int entityIndex = handle.GetEntryIndex();
	if (entityIndex > 0 && entityIndex < 16384)
	{
		transmitEdict->Clear(entityIndex);
	}
}

static void ClearTransmitEntity(CBitVec<16384> *transmitEdict, CEntityInstance *entity)
{
	if (!transmitEdict || !entity)
	{
		return;
	}

	int entityIndex = entity->GetEntityIndex().Get();
	if (entityIndex > 0 && entityIndex < 16384)
	{
		transmitEdict->Clear(entityIndex);
	}
}

static bool SetEntityEffectsNoDraw(CEntityInstance *entity, bool hide)
{
	if (!entity)
	{
		return false;
	}

	SchemaFieldOffset effectsOffset = FindSchemaFieldOffset("CBaseEntity", "m_fEffects");
	if (!effectsOffset.found)
	{
		return false;
	}

	auto *effects = reinterpret_cast<int *>(reinterpret_cast<uint8_t *>(entity) + effectsOffset.offset);
	int nextEffects = hide ? (*effects | EF_NODRAW) : (*effects & ~EF_NODRAW);
	if (*effects == nextEffects)
	{
		return true;
	}

	*effects = nextEffects;
	entity->NetworkStateChanged(NetworkStateChangedData(static_cast<uint32>(effectsOffset.offset)));
	entity->NetworkStateChanged(NetworkStateChangedData(true));
	return true;
}

static bool SetEntityRenderAlpha(CEntityInstance *entity, bool hide)
{
	if (!entity)
	{
		return false;
	}

	SchemaFieldOffset colorOffset = FindSchemaFieldOffset("CBaseModelEntity", "m_clrRender");
	if (!colorOffset.found)
	{
		return false;
	}

	auto *rgba = reinterpret_cast<uint8_t *>(entity) + colorOffset.offset;
	uint8_t alpha = hide ? RENDER_ALPHA_HIDDEN : RENDER_ALPHA_VISIBLE;
	if (rgba[3] == alpha)
	{
		return true;
	}

	rgba[3] = alpha;
	entity->NetworkStateChanged(NetworkStateChangedData(static_cast<uint32>(colorOffset.offset)));
	entity->NetworkStateChanged(NetworkStateChangedData(true));
	return true;
}

static bool SetViewModelEntityHidden(CEntityInstance *entity, bool hide)
{
	bool applied = false;
	applied |= SetEntityRenderAlpha(entity, hide);
	applied |= SetEntityEffectsNoDraw(entity, hide);
	return applied;
}

static bool EntityHandleTargets(CEntityInstance *entity, const char *schemaClass, const char *fieldName, CEntityInstance *target)
{
	if (!entity || !target)
	{
		return false;
	}

	SchemaFieldOffset handleOffset = FindSchemaFieldOffset(schemaClass, fieldName);
	if (!handleOffset.found)
	{
		return false;
	}

	auto *handle = reinterpret_cast<CEntityHandle *>(reinterpret_cast<uint8_t *>(entity) + handleOffset.offset);
	return handle->IsValid() && handle->GetEntryIndex() == target->GetEntityIndex().Get()
		&& handle->GetSerialNumber() == target->GetRefEHandle().GetSerialNumber();
}

static bool IsOwnedViewModel(CEntityInstance *entity, CEntityInstance *pawn)
{
	if (!entity || !pawn)
	{
		return false;
	}

	const char *className = entity->GetClassname();
	if (!className)
	{
		return false;
	}

	std::string lowerClassName(className);
	std::transform(lowerClassName.begin(), lowerClassName.end(), lowerClassName.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});

	if (lowerClassName.find("viewmodel") == std::string::npos && lowerClassName.find("view_model") == std::string::npos
		&& lowerClassName.find("arms") == std::string::npos)
	{
		return false;
	}

	return EntityHandleTargets(entity, "CBaseViewModel", "m_hOwner", pawn)
		|| EntityHandleTargets(entity, "CBaseEntity", "m_hOwnerEntity", pawn);
}

static int ApplyOwnedViewModelEntities(CEntityInstance *pawn, bool hide, bool debug)
{
	if (!pawn)
	{
		return 0;
	}

	CGameEntitySystem *entitySystem = GetGameEntitySystem();
	if (!entitySystem)
	{
		return 0;
	}

	int appliedCount = 0;
	CEntityHandle *viewModels = GetPawnViewModelHandles(pawn);
	if (viewModels)
	{
		for (int viewModelIndex = 0; viewModelIndex < 3; viewModelIndex++)
		{
			CEntityInstance *viewModel = GetEntityInstanceByHandle(entitySystem, viewModels[viewModelIndex]);
			if (!viewModel)
			{
				continue;
			}

			if (SetViewModelEntityHidden(viewModel, hide))
			{
				appliedCount++;
				if (debug)
				{
					SbtMessage("hidefps viewmodel[%d] %s index=%d hide=%d\n",
						viewModelIndex,
						viewModel->GetClassname() ? viewModel->GetClassname() : "unknown",
						viewModel->GetEntityIndex().Get(),
						hide ? 1 : 0);
				}
			}
		}
	}

	for (int entityIndex = 1; entityIndex < MAX_TOTAL_ENTITIES; entityIndex++)
	{
		CEntityInstance *entity = GetEntityInstanceByIndex(entitySystem, entityIndex);
		if (!entity || !IsOwnedViewModel(entity, pawn))
		{
			continue;
		}

		if (SetViewModelEntityHidden(entity, hide))
		{
			appliedCount++;
			if (debug)
			{
				SbtMessage("hidefps owned viewmodel %s index=%d hide=%d\n",
					entity->GetClassname() ? entity->GetClassname() : "unknown",
					entity->GetEntityIndex().Get(),
					hide ? 1 : 0);
			}
		}
	}

	return appliedCount;
}

static void ClearOwnedViewModelTransmit(CBitVec<16384> *transmitEdict, CEntityInstance *pawn)
{
	if (!transmitEdict || !pawn)
	{
		return;
	}

	CGameEntitySystem *entitySystem = GetGameEntitySystem();
	if (!entitySystem)
	{
		return;
	}

	CEntityHandle *viewModels = GetPawnViewModelHandles(pawn);
	int clearedCount = 0;
	if (viewModels)
	{
		for (int viewModelIndex = 0; viewModelIndex < 3; viewModelIndex++)
		{
			if (!viewModels[viewModelIndex].IsValid())
			{
				continue;
			}

			ClearTransmitEntity(transmitEdict, viewModels[viewModelIndex]);
			clearedCount++;
		}
	}

	if (clearedCount > 0)
	{
		return;
	}

	for (int entityIndex = 1; entityIndex < MAX_TOTAL_ENTITIES; entityIndex++)
	{
		CEntityInstance *entity = GetEntityInstanceByIndex(entitySystem, entityIndex);
		if (IsOwnedViewModel(entity, pawn))
		{
			ClearTransmitEntity(transmitEdict, entity);
		}
	}
}
}

static void SbtMessage(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	char buffer[1024] = {};
	V_vsnprintf(buffer, sizeof(buffer) - 1, msg, args);
	ConColorMsg(Color(86, 220, 255, 255), "[SwiftlyBhopTimerHelper] %s", buffer);

	va_end(args);
}

static bool StartsWith(const std::string &value, const char *prefix)
{
	return prefix && value.rfind(prefix, 0) == 0;
}

static bool SendClientConVar(int slot, const char *name, const char *value)
{
	if (slot < 0 || slot >= 64 || !name || !name[0] || !value || !g_pNetworkMessages || !g_pSbtGameEventSystem)
	{
		return false;
	}

	INetworkMessageInternal *netmsg = g_pNetworkMessages->FindNetworkMessagePartial("SetConVar");
	if (!netmsg)
	{
		return false;
	}

	auto *msg = netmsg->AllocateMessage()->ToPB<CNETMsg_SetConVar>();
	if (!msg)
	{
		return false;
	}

	CMsg_CVars_CVar *cvar = msg->mutable_convars()->add_cvars();
	cvar->set_name(name);
	cvar->set_value(value);

	SbtSingleRecipientFilter filter(slot);
	g_pSbtGameEventSystem->PostEventAbstract(0, false, &filter, netmsg, msg, 0);
	delete msg;
	return true;
}

static bool IsPersistentReplayBotName(const char *displayName)
{
	if (!displayName)
	{
		return false;
	}

	std::string name(displayName);
	return name == "SBT_SR" || StartsWith(name, "[SR]") || StartsWith(name, "[SR-ST]") || StartsWith(name, "[SR-CL]") || StartsWith(name, "[SR Replay]") || name.find("SwiftlyBhopTimerReplay") != std::string::npos;
}

static std::string GetReplayBotNameTag(const char *displayName)
{
	if (!displayName || displayName[0] != '[')
	{
		return "";
	}

	std::string name(displayName);
	auto closingBracket = name.find(']');
	if (closingBracket == std::string::npos || closingBracket == 0)
	{
		return "";
	}

	return name.substr(0, closingBracket + 1);
}

static bool IsServerRecordReplayBotTag(const std::string &tag)
{
	return tag == "[SR]" || tag == "[SR-ST]" || tag == "[SR-CL]" || tag == "[SR Replay]";
}

static std::string GetPersistentReplayReplacementKey(const char *displayName)
{
	if (!displayName || !displayName[0])
	{
		return "";
	}

	auto tag = GetReplayBotNameTag(displayName);
	if (tag == "[SR-CL]")
	{
		return "[SR-CL]";
	}

	if (tag == "[SR-ST]" || tag == "[SR]" || tag == "[SR Replay]")
	{
		return "[SR-ST]";
	}

	std::string name(displayName);
	if (name == "SBT_SR" || name.find("SwiftlyBhopTimerReplay") != std::string::npos)
	{
		return "[SR-ST]";
	}

	return "";
}

static bool IsProtectedReplayBotKickTarget(const char *displayName)
{
	if (!displayName || !displayName[0])
	{
		return true;
	}

	if (IsPersistentReplayBotName(displayName))
	{
		return true;
	}

	auto tag = GetReplayBotNameTag(displayName);
	return IsServerRecordReplayBotTag(tag);
}

static bool IsSafeMapChangeArg(const char *value)
{
	if (!value || !value[0])
	{
		return false;
	}

	for (const char *cursor = value; *cursor; ++cursor)
	{
		if (*cursor == '\n' || *cursor == '\r' || *cursor == ';')
		{
			return false;
		}
	}

	return true;
}

static bool ParseCommandBool(const CCommand &args, int index, bool fallback)
{
	if (args.ArgC() <= index)
	{
		return fallback;
	}

	const char *value = args.Arg(index);
	if (!value || !value[0])
	{
		return fallback;
	}

	if (!V_stricmp(value, "1") || !V_stricmp(value, "true") || !V_stricmp(value, "on") || !V_stricmp(value, "yes") || !V_stricmp(value, "enabled"))
	{
		return true;
	}

	if (!V_stricmp(value, "0") || !V_stricmp(value, "false") || !V_stricmp(value, "off") || !V_stricmp(value, "no") || !V_stricmp(value, "disabled"))
	{
		return false;
	}

	return std::atoi(value) != 0;
}

SwiftlyBhopTimerHelper g_SwiftlyBhopTimerHelper;

PLUGIN_EXPOSE(SwiftlyBhopTimerHelper, g_SwiftlyBhopTimerHelper);


// Feature implementation fragments. Keep this order unless dependencies are moved intentionally.
#include "impl/commands.inc"
#include "impl/lifecycle.inc"
#include "impl/gimmicks_rng.inc"
#include "impl/restart.inc"
#include "impl/replay_commands.inc"
#include "impl/roundflow_movement.inc"
#include "impl/timers_hud_modes.inc"
#include "impl/player_visuals_noclip.inc"
#include "impl/replay_bot_impl.inc"
#include "impl/engine_commands.inc"
#include "impl/plugin_metadata.inc"
