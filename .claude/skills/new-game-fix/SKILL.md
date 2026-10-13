---
name: new-game-fix
description: Add or update a game fix folder in GameHub-MacOS-Game-Fixes — folder layout, README template, install.sh requirements, source and build rules, and the pre-staging checklist. Use when packaging a working fix for a game that runs in GameHub on macOS, or when changing an existing fix.
---

# new-game-fix

Packages a fix that already works on a real install. Finding the fix is
`gamehub-wine-debugging`; this skill starts once the game runs.

## 1. Folder

`<Game-Name>-<Store>/` at the repo root, using title-case words joined with `-`: `GTA-V-Enhanced-Epic`,
`Cyberpunk-2077-Steam`. Inside:

| Path | When |
|---|---|
| `README.md` | always |
| `files/` | when anything is copied into the game folder or the prefix |
| `src/` + `src/build.sh` | when `files/` holds a binary |
| `install.sh` | when installing takes more than copying one file |

## 2. Strip the fix to what ships

- Remove all diagnostics: logging, backtraces, memory dumps, test hooks.
- Replace machine paths with runtime discovery: GameHub's environment (`WINEDLLPATH`,
  `WINE_GPTK_LIBD3DSHARED_PATH`), `$HOME`, script arguments.
- Search the folder for personal data before staging:

  ```bash
  rg -n -i "/Users/|/var/folders|AUTH_|epicuserid|epicusername|exchangecode" <Game-Name>-<Store>/
  ```

  Expect no hits outside README text that explains a placeholder.

## 3. Build and test what you ship

1. Rebuild every binary with `src/build.sh`. `build.sh` uses `mingw-w64`
   (`x86_64-w64-mingw32-gcc`), runs from any directory and writes into `../files/`.
2. Test the shipped build itself, not the diagnostic one it came from. At minimum, load it under the
   GameHub Wine environment (the harness in `gamehub-wine-debugging`). Best, launch the game with it.
3. Test `install.sh` against a copy: point `HOME` at a temp dir that holds a copy of the container's
   `user.reg` and a fake game folder, run it twice, then diff the result with the original.
4. Write in the README what was tested and how. Mark anything else as untested.

## 4. install.sh requirements

- `set -euo pipefail`; prints a usage line when arguments are missing.
- Takes the game folder as an argument, and the container id when the prefix matters (default `1`).
- Checks that the game executable exists in the given folder.
- Refuses to run while `wineserver` is running: Wine rewrites `user.reg` on exit and would drop the
  edit.
- Backs up every file it edits, using a timestamped suffix.
- Running it twice gives the same result. For a registry section, delete the existing section, then
  append a fresh one.
- Prints each change it made.

## 5. README template

Sections in this order:

1. `# <Game> (<Store>) — GameHub + <graphics stack>`, plus one paragraph on what the fix is for.
2. **Symptoms**: the exact error codes and messages users see.
3. **Tested on**: a table with Mac, macOS, GameHub, graphics stack, Wine build, game version and
   store.
4. **Root causes**: one subsection per error. Explain what the game checks and why it fails under
   GameHub, with the evidence behind it.
5. **What the fix contains**: a table with file, destination and purpose. List every registry
   value and environment variable.
6. **Install**: the script steps, then a **Manual install** subsection.
7. **GameHub settings**: graphics stack, environment variables, and which variables to remove.
8. **Expected after the fix**: harmless warnings, and what can undo the fix (store file
   verification, GameHub settings changes).
9. **Uninstall**.
10. **Building from source**.

## 6. Index and staging

1. Add a row to the root `README.md` fixes table: game, store, graphics stack, the errors it fixes,
   and a link to the folder.
2. Stage only the files of this fix and the index row. Show the staged diff and stop; a maintainer
   commits.
