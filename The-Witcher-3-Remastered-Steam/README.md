# The Witcher 3: Wild Hunt Remastered (Steam) — GameHub + gptk-4.0-2

A fix for running The Witcher 3: Wild Hunt Remastered (the 5.x "next-gen" update, DX12) from Steam
in GameHub on macOS with the `gptk-4.0-2` graphics stack (Game Porting Toolkit 4, D3DMetal).

## Symptoms

Without the fix, the game window stays black a few seconds after launch and never shows the menu.
GameHub eventually kills the game (`launch.exit.success=false ... normalized_exit_code=137` in the
launch log).

## Tested on

| | |
|---|---|
| Mac | MacBook Pro (MacBookPro18,1), M1 Pro |
| macOS | 27.0.1 |
| GameHub | 0.8.613 |
| Graphics | `gptk-4.0-2` |
| Wine | `wine-proton_10.0` (`wine-10.0-ge3bfd9eaa79b`) |
| Game | The Witcher 3: Wild Hunt Remastered, `bin/x64_dx12/witcher3.exe` 5.0.0.1048522, Steam build 25773555 |

## Root cause

### Black screen at startup

During startup the game creates one graphics pipeline state with stream output: a geometry shader,
no pixel shader, and three stream-output entries. D3DMetal's shader converter
(`libmetalirconverter.dylib`) crashes while converting it. From the launch log, right after
D3DMetal starts creating pipelines:

```
Exception 0xc0000005 at 0x20c1a6a58: .../gptk-4.0-2/external/D3DMetal.framework/Versions/A/Resources/libmetalirconverter.dylib + 0x470a58 (_ZNK4llvm5Value7getNameEv + 0)
```

Wine then cannot unwind through the macOS frames: the same `virtual_unwind` backtrace repeats
millions of times (19 GB of stderr in five minutes), and the window stays black.

The fix rejects that one pipeline with `E_INVALIDARG` before it reaches D3DMetal. The game carries
on without it. A diagnostic build of the wrapper logged exactly one rejected pipeline per launch:
`SO entries=3 gs=6900 ps=0`. All other pipelines were created normally, and the converter crash no
longer appeared in the launch log.

Which visual effect uses that pipeline is unverified; one effect may be missing.

The same crash is reported for CrossOver and UTM, and the community CrossOver fixes reject the same
pipeline: [tholtman1-del/witcher3-crossover-fix](https://github.com/tholtman1-del/witcher3-crossover-fix)
and [Dellety/witcher3-501-crossover-fix](https://github.com/Dellety/witcher3-501-crossover-fix)
(both MIT). This fix takes the idea of rejecting stream-output pipelines from them; the code is
written for GameHub. The CrossOver fixes also replace the game's FidelityFX loader and change
`D3D12CreateDevice` return values. Under GameHub the game started without those changes, so they
are not included.

## What the fix contains

| File | Goes to | What it does |
|---|---|---|
| `files/d3d12.dll` | `<game folder>/bin/x64_dx12/` | A wrapper around the GPTK `d3d12.dll`. Forwards every call to D3DMetal and makes `ID3D12Device::CreateGraphicsPipelineState` return `E_INVALIDARG` for pipelines with stream output |
| Wine registry | the container's `user.reg` | For `witcher3.exe`: `d3d12` = `native,builtin` |

The wrapper finds the real GPTK `d3d12.dll` through `WINEDLLPATH`, which GameHub sets.
`WINE_GPTK_LIBD3DSHARED_PATH` is the fallback. No user path is hardcoded.

## Install

1. Close the game and Steam in GameHub.

2. Run the script with the game folder (the one that contains `bin/x64_dx12/witcher3.exe`):

   ```bash
   ./install.sh "<Steam library>/steamapps/common/The Witcher 3"
   ```

   The script:

   - copies `d3d12.dll` into `bin/x64_dx12/` (an existing different `d3d12.dll` is backed up as
     `d3d12.dll.bak-w3fix-<date>`);
   - backs up `user.reg` next to it as `user.reg.bak-w3fix-<date>`;
   - writes the `AppDefaults\witcher3.exe\DllOverrides` section to the registry.

   If the game is not in the first virtual container, pass the container number as the second
   argument:

   ```bash
   ./install.sh "<Steam library>/steamapps/common/The Witcher 3" 2
   ```

   Containers live in
   `~/Library/Application Support/com.gamemac.www/wine-engine/containers/virtual_containers/`.
   For Steam installed inside GameHub, the game folder is
   `<container>/drive_c/Program Files (x86)/Steam/steamapps/common/The Witcher 3`.

3. Apply the [GameHub settings](#gamehub-settings) and launch the game.

### Manual install

The same steps without the script:

1. Copy `files/d3d12.dll` to `<game folder>/bin/x64_dx12/d3d12.dll`, next to `witcher3.exe`.
2. With Wine closed, append to the container's `user.reg`:

   ```
   [Software\\Wine\\AppDefaults\\witcher3.exe\\DllOverrides]
   "d3d12"="native,builtin"
   ```

## GameHub settings

In the game's settings in GameHub:

- Graphics: `gptk-4.0-2`.
- Launch argument `--launcher-skip`. It starts the game directly instead of through the REDlauncher.
  The fix was tested with it.

## Expected after the fix

- Steam file verification or a game update may delete `bin/x64_dx12/d3d12.dll`. Run `install.sh`
  again if it does.

## Uninstall

1. Delete `<game folder>/bin/x64_dx12/d3d12.dll`.
2. Restore `user.reg` from the backup, or delete the
   `AppDefaults\\witcher3.exe\\DllOverrides` section.

## Building from source

Requires `mingw-w64` (`brew install mingw-w64`):

```bash
./src/build.sh
```

The script rebuilds `files/d3d12.dll`.
