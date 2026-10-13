---
name: gamehub-wine-debugging
description: Diagnose why a Windows game fails in GameHub on macOS (Wine, GPTK/D3DMetal, DXMT) — GameHub file locations, reproducing the launch environment, reading Wine and game logs, proxy DLLs, and dumping and disassembling decrypted game code to find the failing check. Use when a game shows an error, hangs or crashes under GameHub, before writing a fix.
---

# gamehub-wine-debugging

Find the failing check from evidence, then fix exactly that. Guessing at spoofs (vendor IDs,
driver versions, feature tiers) wastes runs. Locate the code that decides first.

## 1. Where things live

Abbreviation: `WE` = `~/Library/Application Support/com.gamemac.www/wine-engine`.

| What | Path |
|---|---|
| Wine builds | `WE/containers/wine_installations/<id>/` (the build id is printed in each launch log) |
| Prefix (layered) | `WE/containers/virtual_containers/<n>/` over `WE/base_containers/<n>/` |
| Registry | `WE/containers/virtual_containers/<n>/user.reg`, `system.reg` |
| Graphics stacks | `WE/downloads/<stack>/`, e.g. `gptk-4.0-2/wine/x86_64-windows`, `gptk-4.0-2/external/libd3dshared.dylib` |
| Launch logs | `WE/wine-logs/wine-launch-*.log`: the full env (`env.*=`) plus Wine debug output |
| GameHub log | `~/Library/Logs/com.gamemac.www/gamehub.log` |
| Per-exe env | `WE/gamehub/graphics_stack_exceptions/<game>/<exe>.env` |

GameHub-specific behaviour:

- The prefix is layered with sandboxfs (`DYLD_INSERT_LIBRARIES=.../libsandboxfs.dylib` plus
  `SANDBOXFS_LAYER_MANIFEST`).
- GameHub deletes custom per-exe `.env` files when settings change. Put lasting overrides in the
  registry (`[Software\\Wine\\AppDefaults\\<exe>\\DllOverrides]`) or in the GameHub env settings.
- GameHub can force a different graphics stack for launcher helpers, e.g. DXMT for the Rockstar
  `Launcher.exe` and `SocialClubHelper.exe`.
- Store launches go through a fake launcher stub (for Epic,
  `C:\windows\command\EpicGamesLauncher.exe`).

Logs contain auth exchange codes and account IDs. Never paste them into commits or issues
unredacted.

## 2. Ground rules

- Before running anything in the prefix, check the game is closed:
  `pgrep -fl "wineserver|<GameExe>"`.
  A second wineserver with a different environment corrupts state and causes false "already running"
  errors.
- Edit `user.reg` only while `wineserver` is down. Back it up first.
- Kill leftovers from a failed run before the next one. They keep the store or launcher convinced
  the game is still running.
- Change one thing per user launch, and log enough to tell what it did.

## 3. Reproduce GameHub's environment

Copy the `env.*` lines from the latest launch log. A minimal harness for test programs:

```bash
env -i HOME="$HOME" PATH="$WI/bin:/usr/bin:/bin" \
  WINEPREFIX="$WE/containers/virtual_containers/1" WINEPREFIX_BASE="$WE/base_containers/1" \
  WINEARCH=win64 WINEDLLPATH="$G/wine" WINE_GPTK_LIBD3DSHARED_PATH="$G/external/libd3dshared.dylib" \
  DYLD_FALLBACK_LIBRARY_PATH="$WI/lib:$WI/lib/wine/" \
  DYLD_INSERT_LIBRARIES=/Applications/GameHub.app/Contents/Resources/libsandboxfs.dylib \
  SANDBOXFS_LIB_PATH=/Applications/GameHub.app/Contents/Resources/libsandboxfs.dylib \
  SANDBOXFS_LAYER_MANIFEST="<from the launch log>" WINE_PATH="$WI" WINEMSYNC=1 WINEDEBUG=-all \
  "$WI/bin/wine" ./test.exe
```

`WI` is the Wine build dir; `G` is the graphics stack dir. Stop the server afterwards with
`"$WI/bin/wineserver" -k`. Test programs should write their output to a file (`freopen`), because
stdout is noisy. macOS has no `timeout` command.

Build Windows test programs and DLLs with `x86_64-w64-mingw32-gcc` (`brew install mingw-w64`).

## 4. Read what the game reports

1. Find the launch log's `Exception 0x80000003` (a game fatal error is often an `int3`) and its
   `virtual_unwind backtrace` lines. They give the module and offset of the failing path.
2. Check the game's own logs and crash logs in the prefix, under `users/steamuser/AppData/Local/<publisher>/`
   and `users/steamuser/Documents/<publisher>/`.
3. Check whether a DLL the game ships is replaced by a Wine builtin, or the other way round.
   Common suspects: `amd_ags_x64`, `nvapi64`, `dxgi`, `d3d12`, overlay DLLs.

## 5. Proxy DLL technique

To see and change what the game gets from a system DLL, drop a proxy with the same name into the
game folder:

- Add `[Software\\Wine\\AppDefaults\\<exe>\\DllOverrides] "<dll>"="native,builtin"` so Wine loads it.
- Load the real DLL by full path. For GPTK, the path comes from `WINEDLLPATH`. Never load it by
  name: that loads the proxy again.
- Export every function the real DLL exports, and forward unchanged calls.
- To log COM interfaces, patch vtable slots: device, factory, adapter, output.
- To find the code that called you, log `RtlCaptureStackBackTrace` frames as `module+offset`.
- Write logs to a fixed file path through `Z:\...`.

Make vtable patching thread-safe. Games create devices from several threads at once. If two threads
patch the same vtable, the second one saves the hook as the "original", and the hook then calls
itself until the stack overflows (`0xc00000fd`). Patch under a lock, and skip vtables that are
already patched. Builds with logging are slower and can hide this race, so always test the shipped
build in the game.

A proxy for one DLL can break another stack component. For example, a proxy `dxgi.dll` gets picked
up by GPTK's own `d3d12`. Hook from the DLL the game calls directly instead.

## 6. Find the deciding check in protected executables

Many game executables are encrypted on disk (Arxan and similar), so static disassembly is useless.

1. From inside the proxy, dump the decrypted image once the game has initialised. Copy
   `GetModuleHandle(NULL)` region by region with `VirtualQuery`, skip unreadable pages, and write
   file offset = RVA.
2. Disassemble around the backtrace offsets:

   ```bash
   dd if=image.bin of=r.bin bs=1 skip=$((RVA)) count=$((LEN))
   x86_64-w64-mingw32-objdump -D -b binary -mi386:x86-64 -M intel --adjust-vma=$((0x140000000+RVA)) r.bin
   ```

3. Resolve `lea reg,[rip+X]` targets in the dump, as strings (module names, export names, error
   messages) and as globals.
4. Read the branch that leads to the fatal call: what the game loads (`GetModuleHandle`,
   `GetProcAddress`), which value it compares, and which helper DLL it chose.
5. To find a virtual method at runtime, have a proxy thread poll the global object pointer, log the
   vtable slot address, then dump.

## 7. Known GameHub / GPTK pitfalls

| Symptom | Cause | Fix |
|---|---|---|
| D3D12 game errors with "no D3D12" right after adapter selection | The game requires `D3D12Core.dll` loaded with `D3D12SDKVersion` above its minimum; GPTK never loads it | A proxy `d3d12.dll` loads a stub `D3D12Core.dll` |
| Device creation fails before any D3D12 call, with D3DMetal reporting AMD | The game uses AMD AGS to create the device | Disable `amd_ags_x64` for the exe |
| Launcher says "game is currently running"; the game hangs at start | The Epic overlay `EOSOVH-Win64-Shipping.dll` deadlocks the Wine loader lock | Disable `EOSOVH-Win64-Shipping` |
| An NVIDIA-spoofed game loops loading `nvapi64` | GPTK PE `nvapi64.dll` has the internal name `nvapi.dll` and fails its unix init | Symlink `nvapi.dll` → `nvapi64.dll` (and the matching `.so`) in the GPTK package |
| `D3D12_FEATURE_FEATURE_LEVELS` returns `0xf200` | A D3DMetal bug | A proxy caps it at `0xc200` |

Hidden D3DMetal variables (read from `D3DMetal.framework` strings): `D3DM_VENDOR_ID`,
`D3DM_DEVICE_ID`, `D3DM_DEVICE_DESCRIPTION`, `D3DM_DEVICE_SUBSYS`, `D3DM_DEVICE_REVISION`,
`D3DM_MTL4`, `D3DM_ENABLE_METALFX`, `D3DM_SUPPORT_DXR`, `D3DM_EXE_OVERRIDE`, `D3DM_MAX_FPS`,
`D3DM_SHOW_HUD_STATS`. A vendor spoof changes which vendor path the game takes (NVAPI, AGS), so it
can cause a failure as easily as fix one.

## 8. Hand-off

When the game runs, package the fix with `new-game-fix`.
