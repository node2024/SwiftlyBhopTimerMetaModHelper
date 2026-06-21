# SwiftlyBhopTimer MetaMod Helper

> [!IMPORTANT]
> This project was created through Vibe Coding. You are free to fork each project and copy, customize, or develop it however you like.

Small MetaMod:Source helper for SwiftlyBhopTimer.

## What This Helper Is

This helper is the native support layer used by SwiftlyBhopTimer.

The main SwiftlyBhopTimer plugin is a SwiftlyS2 C# plugin and owns timer logic,
database access, HUD, chat commands, MapData, and record storage. This helper
runs as a MetaMod:Source plugin and owns CS2 engine-adjacent behavior.

The split exists because some behavior is not exposed cleanly through
SwiftlyS2/C#, or is easily affected by server mode, workshop cfgs, and engine
command restrictions. Bot creation, native player teleport, MoveType/noclip,
viewmodel/hidefps, round timer support, HTML HUD flashing mitigation,
trigger_push/telehop/bhop block/RNGFix compatibility, and Classic-mode movement
assistance are kept here for reliability.

The SwiftlyS2 plugin talks to this helper through `sbt_*` server commands. That
also means parts of the helper can be reused by a future non-SwiftlyS2 timer
implementation as long as that layer can issue the same server commands.

## Current Scope

- Registers `sbt_replaybot_add`.
- Registers `sbt_replaybot_add_once`.
- Registers `sbt_replaybot_kick`.
- Registers `sbt_replaybot_complete`.
- Registers `sbt_replaybot_convars_apply`.
- Registers `sbt_replaybot_optimize`.
- Registers `sbt_roundflow_apply` for native round/warmup convar enforcement.
- Registers `sbt_movement_compat_apply` and `sbt_movement_compat_burst` for
  jump/subtick compatibility convar support.
- Registers `sbt_roundtimer_sync` and `sbt_timelimit_*` for map time and round
  timer UI support.
- Registers `sbt_htmlhud_fix` for HTML HUD flashing mitigation.
- Registers `sbt_bhop_helper_defaults` for reapplying bhop-oriented helper
  defaults.
- Registers `sbt_change_map` and `sbt_change_map_command` for helper-side map
  changes.
- Registers `sbt_restart_player` and `sbt_restart_player_map_start` for native
  restart teleports.
- Registers `sbt_noclip` for MoveType/ActualMoveType and noclip movement
  support.
- Registers `sbt_bhop_mode` for Standard / Classic native movement assistance.
- Registers `sbt_hidefps`, `sbt_hidefps_toggle`, and `sbt_hideweapon` for
  viewmodel and weapon visibility support.
- Registers SharpTimer-compatible `sharptimer_*` commands and `rngfix_*`
  compatibility commands.
- Applies replay-bot-friendly convars before adding a bot.
- Applies replay-bot playback network flags to owned replay bot pawns.
- Owns replay bot quota and one-shot replay bot completion/kick decisions.
- Resolves the internal CS2 `CreateBot` function and creates the replay bot
  directly, similar to the approach used by cs2kz-metamod.
- Does not fall back to `bot_add_ct`; replay bot creation is intentionally kept
  inside this helper so the SwiftlyS2 plugin only requests helper actions.

It does not generate `info_player_*` spawn entities yet. Dynamic spawn entity
creation from the SwiftlyS2 C# side was unstable in testing, so spawnpoint
injection should be added here only after validating the native entity path for
the target CS2 build.

## Commands

```text
sbt_replaybot_add
sbt_replaybot_add_once
sbt_replaybot_kick
sbt_replaybot_complete
sbt_replaybot_convars_apply
sbt_replaybot_optimize
sbt_roundflow_apply
sbt_movement_compat_apply
sbt_movement_compat_burst
sbt_roundtimer_sync
sbt_htmlhud_fix
sbt_bhop_helper_defaults
sbt_timelimit_init
sbt_timelimit_set
sbt_bhop_mode
sbt_hidefps
sbt_hidefps_toggle
sbt_hideweapon
sbt_change_map
sbt_change_map_command
sbt_restart_player
sbt_restart_player_map_start
sbt_noclip
```

SwiftlyBhopTimer calls `sbt_replaybot_add` when it needs a replay bot, and
`sbt_replaybot_add_once` for menu-spawned replay bots that should be removed
after one pass. It calls `sbt_replaybot_complete` when a one-shot replay has
finished, `sbt_replaybot_optimize` after replay bot creation/convar refreshes,
and `sbt_roundflow_apply` after map/config transitions that may reapply
workshop or game-mode settings.

## Build Requirements

- MetaMod:Source source tree
- HL2SDK CS2 source tree / manifests
- AMBuild 2.2+
- C++17 compiler

The layout follows common CS2 MetaMod projects:

```powershell
python configure.py --sdks cs2 --targets x86_64 --mms_path C:\path\to\mmsource-2.0 --hl2sdk-root C:\path\to\hl2sdk-root --hl2sdk-manifests C:\path\to\hl2sdk-manifests
ambuild
```

Linux example:

```bash
python3 configure.py --sdks cs2 --targets x86_64 --mms_path /path/to/mmsource-2.0 --hl2sdk-root /path/to/hl2sdk-root --hl2sdk-manifests /path/to/hl2sdk-manifests
ambuild
```

The package output is written under `build/package`.

## Install

Copy the generated package into the CS2 `game/csgo` directory so the final
layout becomes:

```text
game/csgo/addons/metamod/swiftlybhoptimer_helper.vdf
game/csgo/addons/swiftlybhoptimer_helper/bin/linuxsteamrt64/swiftlybhoptimer_helper.so
```

or on Windows:

```text
game/csgo/addons/metamod/swiftlybhoptimer_helper.vdf
game/csgo/addons/swiftlybhoptimer_helper/bin/win64/swiftlybhoptimer_helper.dll
```
