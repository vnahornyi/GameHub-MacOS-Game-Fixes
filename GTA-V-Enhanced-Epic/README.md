# GTA V Enhanced (Epic Games) — GameHub + gptk-4.0-2

A fix for running GTA V Enhanced from Epic Games in GameHub on macOS with the `gptk-4.0-2` graphics
stack (Game Porting Toolkit 4, D3DMetal).

## Symptoms

Without the fix, the game exits with one of these errors:

- `ERR_GFX_D3D_NOD3D12`
- `ERR_GFX_INIT` (once the first one is fixed)

## Tested on

| | |
|---|---|
| Mac | MacBook Pro, M1 Pro |
| macOS | 27.0.1 |
| GameHub | 0.8.613 |
| Graphics | `gptk-4.0-2` |
| Wine | `wine-proton_10.0` |
| Game | GTA V Enhanced 1.0.1158.16 (Epic Games) |

## Root causes

### `ERR_GFX_D3D_NOD3D12`

The GPU is not the problem; the game accepts the adapter. Right after picking it, the game looks for
the `D3D12Core.dll` module (Agility SDK) in memory and reads its `D3D12SDKVersion` export. The
version must be greater than 610, otherwise the game exits with `ERR_GFX_D3D_NOD3D12`.

On Windows, the system `d3d12.dll` loads `D3D12Core.dll`. The GPTK `d3d12.dll` does not.

### `ERR_GFX_INIT`

D3DMetal reports an AMD GPU. On AMD, the game creates its device through AMD AGS
(`amd_ags_x64.dll` → `agsDriverExtensionsDX12_CreateDevice`) instead of `D3D12CreateDevice`. AGS
needs a real AMD driver, so device creation fails on a Mac. With AGS disabled, the game takes the
plain D3D12 path through D3DMetal.

## What the fix contains

| File | Goes to | What it does |
|---|---|---|
| `files/d3d12.dll` | the game folder | A wrapper around the GPTK `d3d12.dll`. Loads `gptkfix/D3D12Core.dll` and forwards every call to D3DMetal |
| `files/gptkfix/D3D12Core.dll` | `<game folder>/gptkfix/` | A stub that only exports `D3D12SDKVersion = 611` |
| Wine registry | the container's `user.reg` | For `GTA5_Enhanced.exe`: `d3d12` = `native,builtin`; `amd_ags_x64` and the EOS overlay disabled |

The wrapper finds the real GPTK `d3d12.dll` through `WINEDLLPATH`, which GameHub sets.
`WINE_GPTK_LIBD3DSHARED_PATH` is the fallback. No user path is hardcoded.

The wrapper also adjusts a few D3DMetal answers during adapter selection:

- it caps `D3D12_FEATURE_FEATURE_LEVELS` at 12_2, because D3DMetal returns the nonexistent `0xf200`;
- it names the adapter "AMD Radeon RX 6800 XT";
- it reports AMD driver version 32.0.13031.3015.

The game may pass without these adjustments, but the tested configuration includes them, so they
stay.

Test status: the game ran with a build of this wrapper that differed only in having diagnostic
logging and a hardcoded GPTK path. The packaged `d3d12.dll` was tested under GameHub's Wine
environment with a test program: it found GPTK through `WINEDLLPATH`, created a device and loaded
the `D3D12Core.dll` stub. It has not yet been run with the game itself.

## Install

1. Close the game, the Rockstar Launcher and anything else running through GameHub.

2. Run the script with the game folder:

   ```bash
   ./install.sh ~/GameHub/GTAVEnhanced
   ```

   The script:

   - copies `d3d12.dll` and `gptkfix/D3D12Core.dll` into the game folder;
   - backs up `user.reg` next to it as `user.reg.bak-gta5fix-<date>`;
   - writes the `AppDefaults\GTA5_Enhanced.exe\DllOverrides` section to the registry.

   If the game is not in the first virtual container, pass the container number as the second
   argument:

   ```bash
   ./install.sh ~/GameHub/GTAVEnhanced 2
   ```

   Containers live in
   `~/Library/Application Support/com.gamemac.www/wine-engine/containers/virtual_containers/`.

3. Apply the [GameHub settings](#gamehub-settings) and launch the game.

### Manual install

The same steps without the script:

1. Copy `files/d3d12.dll` into the game folder, next to `GTA5_Enhanced.exe`.
2. Copy `files/gptkfix/D3D12Core.dll` to `<game folder>/gptkfix/D3D12Core.dll`.
3. With Wine closed, append to the container's `user.reg`:

   ```
   [Software\\Wine\\AppDefaults\\GTA5_Enhanced.exe\\DllOverrides]
   "amd_ags_x64"=""
   "d3d12"="native,builtin"
   "EOSOVH-Win64-Shipping"=""
   ```

## GameHub settings

In the game's settings in GameHub:

- Graphics: `gptk-4.0-2`.
- Environment variable `WINEDLLOVERRIDES=EOSOVH-Win64-Shipping=d`. It disables the Epic overlay,
  which deadlocks the Wine loader. Without it, the game hangs and the Rockstar Launcher shows
  "A game is currently running".
- Remove `D3DM_VENDOR_ID` and `D3DM_DEVICE_ID` if you set them. The fix relies on D3DMetal's
  default AMD adapter.

## Expected after the fix

- At startup the game shows "Please update your graphics driver. Game requires version 24.12.1 or
  newer". Click OK and the game starts. The game reads the AMD driver version from AGS, which is
  disabled.
- Verifying files through Epic may delete `d3d12.dll` from the game folder. Run `install.sh` again
  if it does.

## Uninstall

1. Delete `<game folder>/d3d12.dll` and the `<game folder>/gptkfix` folder.
2. Restore `user.reg` from the backup, or delete the
   `AppDefaults\\GTA5_Enhanced.exe\\DllOverrides` section.

## Building from source

Requires `mingw-w64` (`brew install mingw-w64`):

```bash
./src/build.sh
```

The script rebuilds `files/d3d12.dll` and `files/gptkfix/D3D12Core.dll`.
