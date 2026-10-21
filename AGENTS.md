# OROS 1 — ChatGPT Development Rules

## Mission
OROS 1 is a Windows-first custom C++23 game engine. Current milestone: **OROS-009 Living World**. The user wants to say **"Build OROS" in this ChatGPT conversation** and have the assistant implement the next coherent, testable increment through the connected GitHub repository, without routine user-run PowerShell scripts and without occupying their Codex workflow.

## Branch discipline
- Repository: drewweske/OROS-1.
- Verified pre-automation integration branch: `feature/oros-009-living-world` at `c9a95a9`.
- Working branch: `automation/oros-009-chat-build`; never overwrite or force-push the integration branch.
- Inspect branch head and unmerged changes before each increment. Prefer small commits and a draft PR for review. Do not auto-merge.
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

## When the user says "Build OROS"
1. Read architecture, current branch, code, tests, and prior CI outcomes.
2. Choose one bounded coherent next milestone. Implement source plus meaningful automated tests directly on the working GitHub branch.
3. Let GitHub Actions compile the current commit on Windows, read build logs, and fix errors iteratively. Do not infer success from older CI runs.
4. State exactly what was changed, the commit/PR, test counts and any limitations. Ask only when a real blocker or external decision requires it.
5. Do not spend user Codex usage. Do not send lengthy copy-paste PowerShell for routine changes.
6. Never claim to have launched the end-user's native window or to have synchronized their laptop. PR merging needs deliberate review.

## October 2026 handoff
- Baseline `c9a95a9`: live NPC visual perception is invoked during fixed simulation ticks; in a previous interactive run the NPC logged visible and then out_of_range while preserving 1 remembered observation. 117/117 automated tests passed before local actor vision work.
- User subsequently created **uncommitted, locally verified**: `engine/ai/include/oros/ai/actor_vision_state.hpp`, `tests/ai/actor_vision_state_tests.cpp`, CMake test registration (118 tests), and `apps/bootstrap/live_ai_actor_demo.hpp` optional actor vision field. Their File 7 compilation and 2/2 focused tests passed, but these files were not pushed.
- Carefully reconstruct this work in the automation branch, without pretending to have copied local filesystem bytes. Next: initialize vision during actor composition in `apps/bootstrap/live_ai_actor_demo.cpp`, test persistence across fidelity transitions; then pass actor-owned parameters to the fixed simulation perception call in `apps/bootstrap/main.cpp`.
