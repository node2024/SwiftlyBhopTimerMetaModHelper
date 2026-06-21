# SwiftlyBhopTimer MetaMod Helper

> [!IMPORTANT]
> このプロジェクトは Vibe Coding で作成されています。各プロジェクトは自由にフォークし、コピー、カスタマイズ、開発していただいて構いません。

SwiftlyBhopTimer 用の小さな MetaMod:Source 補助プラグインです。

## Helper とは

Helper は、SwiftlyBhopTimer 本体から呼び出される native 補助レイヤーです。

SwiftlyBhopTimer 本体は SwiftlyS2 の C# プラグインとして、タイマー、DB、HUD、チャットコマンド、MapData、記録保存を担当します。一方、この Helper は MetaMod:Source 上で動作し、CS2 エンジン寄りの処理を担当します。

切り分けている理由は、SwiftlyS2/C# からは安定して触りにくい処理があるためです。bot 生成、プレイヤーの native teleport、MoveType/noclip、viewmodel/hidefps、round timer、HTML HUD flashing 対策、trigger_push/telehop/bhop block/RNGFix 互換、Classic mode の movement 補助などは、サーバーモードや workshop cfg、エンジン側のコマンド制限の影響を受けやすいため、この native helper 側へ寄せています。

SwiftlyBhopTimer 本体は `sbt_*` サーバーコマンドを通して Helper を呼び出します。そのため、将来的に SwiftlyS2 以外のタイマー本体へ移行する場合でも、この Helper の一部機能は再利用しやすい構成です。

## 現在の範囲

- `sbt_replaybot_add` を登録します。
- `sbt_replaybot_add_once` を登録します。
- `sbt_replaybot_kick` を登録します。
- `sbt_replaybot_complete` を登録します。
- `sbt_replaybot_convars_apply` を登録します。
- `sbt_replaybot_optimize` を登録します。
- `sbt_roundflow_apply` で round / warmup 系 ConVar を native 側から再適用します。
- `sbt_movement_compat_apply` / `sbt_movement_compat_burst` でジャンプ/subtick 互換 ConVar を補助します。
- `sbt_roundtimer_sync` / `sbt_timelimit_*` で map time と round timer UI を補助します。
- `sbt_htmlhud_fix` で HTML HUD flashing 対策を行います。
- `sbt_bhop_helper_defaults` で bhop 向け Helper 設定を再適用します。
- `sbt_change_map` / `sbt_change_map_command` で map change を helper 側から実行します。
- `sbt_restart_player` / `sbt_restart_player_map_start` で `!r` 向けの native teleport を行います。
- `sbt_noclip` で MoveType/ActualMoveType と noclip movement を補助します。
- `sbt_bhop_mode` で Standard / Classic mode の native movement 補助を切り替えます。
- `sbt_hidefps` / `sbt_hidefps_toggle` / `sbt_hideweapon` で viewmodel と weapon 表示制御を補助します。
- `sharptimer_*` 互換コマンドと `rngfix_*` 互換コマンドを登録します。
- replay bot 追加前に bot 用 ConVar を適用します。
- 所有している replay bot の pawn に描画・補間向けの network flag を適用します。
- replay bot の上限管理と、1周だけ再生する bot の完了時 Kick 判定を helper 側で行います。
- CS2 内部の `CreateBot` 関数をシグネチャから解決し、cs2kz-metamod に近い方式で replay bot を直接作成します。
- `CreateBot` の解決に失敗しても `bot_add_ct` にはフォールバックしません。bot 生成はこの helper に閉じます。

現時点では `info_player_*` spawn entity の生成は実装していません。SwiftlyS2 C# 側からの動的 spawn entity 生成はテストで不安定だったため、追加する場合は対象 CS2 build で native entity 経路を検証してから実装する方針です。

## コマンド

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

SwiftlyBhopTimer 側は replay bot が必要なときに `sbt_replaybot_add` を呼びます。`!replay` メニューから追加された1周だけの replay bot には `sbt_replaybot_add_once` を使い、1周完了時に `sbt_replaybot_complete` を呼びます。replay bot 作成後や ConVar 再適用後には `sbt_replaybot_optimize` を呼び、map/config 遷移後には `sbt_roundflow_apply` を呼びます。

## ビルド要件

- MetaMod:Source source tree
- HL2SDK CS2 source tree / manifests
- AMBuild 2.2+
- C++17 compiler

Windows 例:

```powershell
python configure.py --sdks cs2 --targets x86_64 --mms_path C:\path\to\mmsource-2.0 --hl2sdk-root C:\path\to\hl2sdk-root --hl2sdk-manifests C:\path\to\hl2sdk-manifests
ambuild
```

Linux 例:

```bash
python3 configure.py --sdks cs2 --targets x86_64 --mms_path /path/to/mmsource-2.0 --hl2sdk-root /path/to/hl2sdk-root --hl2sdk-manifests /path/to/hl2sdk-manifests
ambuild
```

package は `build/package` に出力されます。

## 導入

生成された package を CS2 の `game/csgo` にコピーしてください。

Linux:

```text
game/csgo/addons/metamod/swiftlybhoptimer_helper.vdf
game/csgo/addons/swiftlybhoptimer_helper/bin/linuxsteamrt64/swiftlybhoptimer_helper.so
```

Windows:

```text
game/csgo/addons/metamod/swiftlybhoptimer_helper.vdf
game/csgo/addons/swiftlybhoptimer_helper/bin/win64/swiftlybhoptimer_helper.dll
```
