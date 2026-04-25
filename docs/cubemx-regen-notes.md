# CubeMX Regeneration Notes

Record of what happened when we tried to evolve `cerebellum.ioc` in STM32CubeMX. Pick up here when we come back to the code-gen workflow.

## Context

- Seed: `Projects/NUCLEO-N657X0-Q/Examples/GPIO/GPIO_IOToggle` from STM32Cube_FW_N6 (hand-copied into the repo; source tree lives under `Core/Src`, `Core/Inc`, `Core/Startup`).
- Working blinky in `Core/Src/main.c` (LD1 on PG8, 1 Hz, plus `printf` routed to ITM).
- Build: CMake + Ninja, arm-none-eabi toolchain file in `cmake/`. `CMakeLists.txt` references `Core/`.
- Debug: dev-boot over SWD via `tools/stlink-gdbserver-devboot.sh` (strips `--halt`, sets `-m 1`). Working.
- Goal when we opened CubeMX: add PG0 as `LED2` and blink it at 1 Hz too.

## The `.ioc` as committed

Key settings in the seed `cerebellum.ioc`:

```
MxCube.Version=6.13.0
Mcu.ContextProject=FullSecure
Mcu.Context0=FSBL
Mcu.Context1=Appli
Mcu.Context2=ExtMemLoader
Mcu.ContextNb=3
ProjectManager.TargetToolchain=EWARM V9.40
ProjectManager.UnderRoot=false
ProjectManager.LibraryCopy=2        # Copy only necessary
ProjectManager.CoupleFile=false
```

The seed is a **3-context FullSecure template**. That is the important fact. CubeMX always wants to generate three output trees for that template (`FSBL/`, `Appli/`, `ExtMemLoader/`), one per context. The fact that the seed checks code into `Core/` is because ST's example did it that way by hand — it is not what CubeMX regeneration does.

## What we tried, in order

### Attempt 1 — CubeMX 6.17.0 + default toolchain

- Hit **Generate Code**.
- Result:
  - Created `FSBL/Src/`, `FSBL/Inc/` with fresh CubeMX template `main.c` / `main.h` (no blinky, no ITM printf).
  - Created `EWARM/` project files.
  - Dropped a **646 MB `STM32Cube_FW_N6_V1.3.0/`** directory at the repo root (full firmware package copy).
  - Created `CMakePresets.json`, `.mxproject` at root.
  - The original `Drivers/` tree appeared to change — HAL headers for peripherals not referenced in the `.ioc` were pruned. At the time I mis-read this as "CubeMX deleted Drivers", which was wrong; it was `LibraryCopy=2` doing intentional pruning.
- Rollback: `git checkout -- .` + `rm -rf EWARM STM32Cube_FW_N6_V1.3.0 .mxproject CMakePresets.json`.

### Attempt 2 — same CubeMX, "Keep User Code when re-generating" experiments

- Tried with "Keep User Code" unchecked first, then checked.
- Note for future: **this MUST be checked**, otherwise `/* USER CODE BEGIN */ ... /* USER CODE END */` blocks get wiped. My first-round advice here was wrong.
- Regardless of the checkbox, output still went to `FSBL/` because that is what the template dictates.

### Attempt 3 — downgrade to CubeMX 6.13.0

- Reasoning at the time: maybe 6.17.0 regressed. This was the wrong hypothesis.
- Installed 6.13.0. It only ships V1.0.0 of the N6 firmware package in its manager; manually pointed `ProjectManager.CustomerFirmwarePackage` at the already-installed V1.3.0 on disk.
- Regenerated. Same problem: output in `FSBL/`, new build-system folder, `Drivers/` pruned.
- **Conclusion: CubeMX version is not the variable.**

### Attempt 4 — switch TargetToolchain to Makefile

- Reasoning: maybe EWARM-specific output was the trigger. Changed `ProjectManager.TargetToolchain` from `EWARM V9.40` → `Makefile`, left `UnderRoot=false`, `LibraryCopy=2` (verified after one `.ioc` save flipped it to `1`).
- Diff showed harmless churn: `NVIC2`/`CORTEX_M55_NS`/`AppNS` dropped, `VP_RIF_VS_RIF1` added, a big `RIF.RIFAWARE.*` block auto-added, `CoupleFile: false → true`.
- Hit Generate.
- Result:
  - `Core/` — **untouched** (`git diff --stat Core/` empty). Our working blinky code is intact.
  - `FSBL/Inc/`, `FSBL/Src/` — new parallel tree with default-template `main.c`/`main.h` (no ITM printf, no blinky logic).
  - `Makefile/FSBL/` — generated Makefile, `startup_stm32n657xx_fsbl.s`, `STM32N657XX_AXISRAM2_fsbl.ld`.
  - `Drivers/` — pruned to only what the current `.ioc` references (gpio, dma, rcc, pwr, exti, cortex, core hal). The many "deleted" entries in `git status` are intentional `LibraryCopy=2` pruning.
  - `.mxproject` at repo root.

## Actual root cause

`Mcu.ContextProject=FullSecure` pins the template to a 3-context layout. CubeMX always writes generated code into `FSBL/`, `Appli/`, `ExtMemLoader/` subdirectories for that template, regardless of CubeMX version or toolchain choice. The seed's `Core/` layout came from hand-copying ST's example — it isn't what the template produces.

So every `Generate Code` click writes to `FSBL/` while the CMake build keeps pointing at `Core/`. The two trees diverge immediately, and any CubeMX-generated config changes never reach the actual build.

## My misreads along the way (so we don't repeat them)

- "CubeMX 6.17.0 deleted `Drivers/`." **No** — `LibraryCopy=2` pruned unused HAL headers. The remaining set still covers what we actually use.
- "The old `FSBL/` got deleted." **No** — `FSBL/` was never in git. CubeMX was creating it fresh each time.
- "Toolchain = Makefile will fix the output layout." **No** — toolchain changes the build-system files (`Makefile/` vs `EWARM/`), not the context-based source tree layout.
- "Keep User Code when re-generating is optional." **No** — must be checked; otherwise USER CODE blocks get wiped.

## Three paths that were on the table

1. **Keep `Core/` canonical, stop regenerating.** Hand-edit `Core/Inc/main.h` + `Core/Src/main.c` to add LED2 on PG0. Treat `cerebellum.ioc` as pin-planning documentation only. Delete `FSBL/` and `Makefile/`. Fast, preserves the working dev-boot setup, costs us the CubeMX round-trip for future peripheral adds.

2. **Adopt `FSBL/` as canonical.** Port `Core/Src/main.c`'s blinky + ITM code into `FSBL/Src/main.c`. Delete `Core/`. Update `CMakeLists.txt` include paths from `Core/*` to `FSBL/*` (and startup / linker paths to `Makefile/FSBL/*` or wherever we land them). Then CubeMX regen works cleanly from then on. More up-front work, but the `.ioc` becomes the source of truth.

3. **Rework the `.ioc`** to generate into `Core/` — switch away from `FullSecure` multi-context to a single-context project. Invasive; risks breaking the known-good dev-boot config. Not recommended unless we have reason to need CubeMX round-tripping *and* the `Core/` name.

## Decision (2026-04-25)

We are choosing **path 1** for this repository.

- `Core/` remains canonical.
- `CMakeLists.txt` + the VS Code workspace remain the source of truth for build/debug.
- `cerebellum.ioc` stays committed as a **planning / verification artifact only**.
- We do **not** use CubeMX code generation inside this repo.
- If we need generated scaffolding for a new peripheral, create a throwaway CubeMX project outside the repo and copy the relevant init code back by hand.
- If we ever decide we want true CubeMX round-tripping on STM32N6, do it as a **fresh migration** to an `FSBL/` + `Appli/`-style layout rather than trying to retrofit the current hand-maintained `Core/` tree.

## When we pick this back up

- Keep using `cerebellum.ioc` only for planning / validation, not in-repo code generation.
- When adding a peripheral, apply the final init changes by hand in `Core/` or `Application/`.
- If we later want true CubeMX round-tripping, do it as a fresh migration branch to an `FSBL/` + `Appli/`-style project layout.
- Pin the known-good reference tooling in the README once we've validated it end-to-end:
  `CubeMX 6.13.0 + STM32Cube_FW_N6 V1.3.0` as the last-known reference environment for the current `.ioc`.
- Current rollback command if we need a clean slate:
  ```
  git checkout -- .
  rm -rf FSBL Makefile .mxproject
  git clean -fd   # after reviewing `git clean -nd` output
  ```
