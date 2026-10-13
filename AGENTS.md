# GameHub-MacOS-Game-Fixes — repository rules

Fixes for Windows games that fail to run in GameHub (`com.gamemac.www`) on macOS. Every rule here
applies to people and AI agents alike.

## Skills

Load the matching skill before working:

| Task | Skill |
|---|---|
| Adding or updating a fix folder | `.claude/skills/new-game-fix/SKILL.md` |
| Finding why a game fails under GameHub / Wine / GPTK | `.claude/skills/gamehub-wine-debugging/SKILL.md` |

Agents whose runtime does not auto-load `.claude/skills` read the `SKILL.md` by path.

## Language

Everything committed is in English: READMEs, code comments, script output, commit messages.

## Layout

```
<Game-Name>-<Store>/          one folder per game and store, e.g. GTA-V-Enhanced-Epic
  README.md                   the fix, following the new-game-fix template
  install.sh                  optional; installs the fix
  files/                      exactly what gets copied into the game or prefix
  src/                        sources and build.sh for every binary in files/
README.md                     index of all fixes
```

A fix for the same game from another store gets its own folder.

## Hard rules

1. **No binary without its source.** Every `.dll`/`.exe` in `files/` is built by `src/build.sh` from
   sources in `src/`. Never commit a binary you cannot rebuild.
2. **No redistributed third-party files.** Do not commit game files, Microsoft redistributables
   (for example the real `D3D12Core.dll`), GPTK/D3DMetal binaries or GameHub files. Ship stubs or
   wrappers you wrote and tell users where to get the rest.
3. **No personal data.** No user names, home paths, machine-specific temp paths, account names,
   Epic/Rockstar IDs or auth tokens. Wine and launcher logs contain exchange codes and account IDs:
   never commit logs, and quote only the lines you need, with secrets removed.
4. **Nothing hardcoded to one machine.** Find paths at runtime: environment variables GameHub sets
   (`WINEDLLPATH`, `WINE_GPTK_LIBD3DSHARED_PATH`), `$HOME`, script arguments.
5. **Every claim traces to evidence.** The README's root cause comes from logs, disassembly or a test
   you ran. Mark anything you did not verify as unverified.
6. **Tested configuration is mandatory.** Mac model, macOS, GameHub version, graphics stack, Wine
   build, game version and store.
7. **Installers are safe.** `install.sh` refuses to run while Wine is running, backs up every file it
   edits, can run twice without duplicating anything, and the README documents how to undo it.
8. **Smallest fix that works.** Diagnostic code (logging, backtraces, memory dumps) never ships.
   Keep in a fix only what the tested configuration needs, and say in the README which parts were
   only tested together rather than proven necessary.

## Git

AI agents stage changes and stop; a maintainer reviews, commits and pushes. Agents never push.
