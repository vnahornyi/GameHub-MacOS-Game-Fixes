# GameHub-MacOS-Game-Fixes

Fixes for Windows games that fail to run in GameHub on macOS.

Each fix lives in its own folder, with a README covering the root cause, the tested configuration,
and install and uninstall steps.

## Fixes

| Game | Store | Graphics stack | Fixes | Folder |
|---|---|---|---|---|
| GTA V Enhanced | Epic Games | `gptk-4.0-2` | `ERR_GFX_D3D_NOD3D12`, `ERR_GFX_INIT`, missing dialogue | [GTA-V-Enhanced-Epic](GTA-V-Enhanced-Epic/) |

## Contributing

Read [AGENTS.md](AGENTS.md) first: it holds the repository rules. To add a fix, follow
[`.claude/skills/new-game-fix`](.claude/skills/new-game-fix/SKILL.md). To find out why a game fails,
follow [`.claude/skills/gamehub-wine-debugging`](.claude/skills/gamehub-wine-debugging/SKILL.md).
