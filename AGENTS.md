# OROS 1 — ChatGPT Development Rules

## Mission
OROS 1 is a Windows-first custom C++23 game engine. Current milestone: **OROS-009 Living World**. The user wants to say **"Build OROS" in this ChatGPT conversation** and have the assistant implement the next coherent, testable increment through the connected GitHub repository, without routine user-run PowerShell scripts and without occupying their Codex workflow.

## Branch discipline
- Repository: drewweske/OROS-1.
- Verified pre-automation integration branch: `feature/oros-009-living-world` at `c9a95a9`.
- Official canonical branch: `main`; builds **#1–15 merged** by owner-authorized PR #8 at merge SHA `31937ead` on 2026-10-22, preserving full history. Pre-merge SHA `9705e6e` passed Windows/MSVC and 125/125 CTest.
- Current development branch for Build #16 onward: `automation/oros-016-world-growth`, branched from official `main`. Old `automation/oros-009-chat-build` and `feature/oros-009-living-world` remain historic; never force-push or rewrite them.
- Make reviewable development commits/PRs. **Canonize every 15 Build OROS invocations (#15, #30, #45, …)** only after explicit code/architecture review, clean mergeability and passing exact-SHA Windows CI. The user explicitly authorized this recurring mainline merge policy on 2026-10-22. Do not bypass required checks, override conflicts, force-push or merge a failing branch. If blocked, report the gate instead.
- A recurring policy does **not** start background agents; evaluate/merge when the corresponding Build OROS request is invoked.
- The user's local laptop checkout does not update automatically when GitHub advances.
- Reconcile any locally uncommitted changes before merging back; never assume they are on GitHub.

## Windows toolchain
- Use Windows Server 2022 GitHub Actions (`windows-2022`), Visual Studio 2022, MSVC, C++23, Direct3D 12.
- Fresh CI directory: `cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON -DOROS_WARNINGS_AS_ERRORS=ON`
- Compile: `cmake --build build --config Debug --parallel 4`
- Test: `ctest --test-dir build -C Debug --output-on-failure --no-tests=error`
- Existing laptop CMake cache can have different platform settings: do not reconfigure it with a conflicting explicit `-A x64`; use `cmake -S . -B build`.
- A passed CI build is not a passed interactive D3D12 window smoke test.

## Architecture constraints
- Preserve World-owned entity identity and normalized authoritative positions.
- Persist actor schedules, interrupted intents, factions, simulation fidelity state and visual memory across fidelity transitions.
- Gate expensive local visual perception by `deep_local` fidelity; respect authored collision geometry and streamed residency.
- Deterministic observations use simulation WorldTime. Do not invent visibility when out of range, outside field of view, occluded or unavailable. Preserve remembered last-known observations where specified; reject stale updates.
- Reuse validated AI value types for facing, visual profile and confidence; avoid unvalidated duplicate representations.
- Distinguish bootstrap demonstration code from a reusable multi-NPC simulation implementation.
- Respect architecture documents in the repository and project, incrementally; never create speculative large scaffolds all at once.

## Mandatory five-command architecture review cadence
- The phrase **"Build OROS"** in a user turn counts as one invocation, even when followed by other instructions. Count exactly once per requested development run, not per assistant tool call, commit, CI retry, or partial response.
- Track invocation numbers in `docs/development/build-oros-runs.md` and update the record in every completed run.
- **Every fifth invocation (5, 10, 15, ...)** perform a new full-repository structural inventory, module-by-module architecture review, critical-source and test review, CI status analysis, risks, findings and prioritized remediation plan; commit a dated report to `docs/architecture/reviews/`.
- Mandatory architecture reports have been committed for #5, #10 and #15; **next architecture review is #20**, independent of the next mainline canonization at #30. Run #2 was an extra baseline review. Reports live in `docs/architecture/reviews/`.
- A source inventory generated on every CI run is mechanical coverage, not a substitute for the fifth-run architectural reasoning review.
- If a request explicitly calls for an extra audit, do it regardless of command number.
- Never claim that a structural inventory is a line-by-line formal security audit. Report sampling limits and what could not be run.

## When the user says "Build OROS"
1. Read architecture, current branch, code, tests, and prior CI outcomes.
2. Choose one bounded coherent next milestone. Implement source plus meaningful automated tests directly on the working GitHub branch.
3. Let GitHub Actions compile the current commit on Windows, read build logs, and fix errors iteratively. Do not infer success from older CI runs.
4. State exactly what was changed, the commit/PR, test counts and any limitations. Ask only when a real blocker or external decision requires it.
5. Do not spend user Codex usage. Do not send lengthy copy-paste PowerShell for routine changes.
6. Never claim to have launched the end-user's native window or to have synchronized their laptop. PR merging needs deliberate review.

## Current handoff — Build OROS #16, October 2026
- First-party user screenshots confirmed the D3D12 grid renders and turning changes the view on a personal Windows GPU at run #7; later outpost revisions are built and tested in CI, not confirmed by another personal GPU screenshot.
- The Windows CI pipeline publishes the game executable `Oros1.exe` as the `OROS-1-Windows-Debug` Actions artifact after compilation and CTest pass. The local checkout with old uncommitted work remains **separate**. Read `docs/development/run-oros-on-windows.md` before advising user playtests.
- Outpost layout source of truth: `apps/bootstrap/outpost_layout.hpp`; three building boxes share an environment/floor EntityId with unique collider shape slots and are loaded in one WorldCell. This is a **bootstrap prototype** with no separate persistent building entities. NPC line-of-sight reads the same authoritative colliders as the player capsule controller.
- Known architecture risks recorded in `docs/architecture/reviews/2026-10-22-oros-whole-engine-review-10.md`: per-tick AI memory copying, bootstrap `main.cpp` centralization, collider/entity identity coupling, spawn-origin assumptions, streaming platform leakage, missing GPU/Release/stress CI.
- Review #15 is in `docs/architecture/reviews/2026-10-22-oros-whole-engine-review-15.md`. The tested #15 head `9705e6e` passed both Windows CI runs, 125/125 CTest, then merged to `main` in PR #8. Next mandatory review **#20**, next mainline canonization **#30**.
- Build #16 changes World entity-growth policy and adds diagnostics for populations 2/100/1000/10000. No timing win is claimed absent comparable old/new results. Next command **#17** after this run completes. The user's laptop checkout remains separate.

## Latest handoff — Build OROS #19 (2026-10-26)
- Development branch `automation/oros-016-world-growth`: #19 source/test SHA `0c17fcf5310b6deaf1835612f59f97a1ca7cbbed` passed [push CI](https://github.com/drewweske/OROS-1/actions/runs/40298439151) and [PR CI](https://github.com/drewweske/OROS-1/actions/runs/40298447657), Windows 2022/MSVC Debug **127/127 CTest**; inspect newer HEAD before claiming it remains latest.
- World can reconstruct live EntityIds from a trusted saved global issued-sequence high-water mark, with non-reuse across deleted/unloaded gaps and UINT64_MAX exhaustion. This is **not** a fully persisted World object/actor graph or disk-save integration; caller still reattaches components and supplies trustworthy global high-water mark.
- Observed LOS scaling hypothesis: `WorldCellColliderRegistry::colliders_relative_to()` reconstructs geometries from every resident cell. Needs bounded workloads, measured before/after data and blocked/clear/unavailable regression before spatial optimization.
- Build run ledger now #19. Next **#20** requires mandatory fifth-run architectural review. Official `main` stays `31937ead`; draft PR #9 stays unmerged. Do not overwrite user's local checkout.
