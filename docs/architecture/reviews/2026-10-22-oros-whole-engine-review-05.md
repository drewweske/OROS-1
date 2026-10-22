# OROS 1 — Mandatory Whole-Engine Architecture Review #5

**Date:** 2026-10-22  
**Checkpoint before review:** `62a708a4db13bcdbbe748507901b3266d7c61dd3`  
**Repository:** `drewweske/OROS-1`, `automation/oros-009-chat-build`  
**Review cadence:** mandatory command **#5**, next **#10** (not per commit or CI build)  
**Planning standard:** OROS 1 Architecture Constitution (provided project document), with stable identity and persistent continuity, fixed-step authority, bounded adaptive AI, C++26 destination / stable C++23 toolchain, platform isolation, versioned persistence.

## Review methodology and accurate scope

The full remote Git tree was enumerated and compared to the run #2 baseline by exact Git blob identity and path; all module counts, translation units, test layout, and build/CI registration were inspected. The automated **whole-source C++ inventory** hashes *each* `.cpp/.hpp/.h`, counts source lines, maps OROS includes, and checks CMake references on every Windows CI run. I additionally inspected representative code, API contracts, error handling and tests in every engine layer, including Windows windowing, D3D12 renderer, the runtime scheduler, World identity/lifetime, assets serialization/loading, streaming file persistence and async I/O, physics narrow-phase, collision/controller, AI navigation and focus/fidelity, perception/memory, and the live bootstrap app.

**Limits:** This is a full-codebase *inventory* and a substantive cross-layer architectural/critical-path review. It is **not** a formal line-by-line correctness/security proof of all 3.99 MB, a sanitizer run, a GPU smoke test, or proof of game completeness. The architectural reference DOCX predates OROS-009 and is treated as reference rather than a statement about implemented code. The project Architecture Constitution explicitly says the master folder map is a future destination, not something to pre-create wholesale.

## Repository delta: run #2 versus run #5

| Measure | Baseline at `0a112bc` | Run #5 at `62a708a` | Change |
| --- | ---: | ---: | ---: |
| Tracked blobs | 355 | 364 | +9 |
| Bytes in tracked blobs | 3,929,760 | 3,992,289 | +62,529 |
| C++ translation units | 228 | 232 | +4 |
| Whole-source CI inventory | 348 C++ source/headers | 354 source/headers | +6 |
| Whole-source lines | 146,583 | 147,855 | +1,272 |
| CTest executables | 119 | 121 | +2 |
| CMake missing/stale translation units | 0/0 | 0/0 | unchanged |

**Current CI basis:** Windows Server 2022, MSVC 2022, C++23, warnings-as-errors, Debug full engine build, complete 121/121 CTest pass: https://github.com/drewweske/OROS-1/actions/runs/39741461953 . Run #5's new budget patch must pass its **own** CI before it can be called validated.

Git-tree comparison found changes since the baseline concentrated in AI multi-observer perception, bootstrap composition, a source-inventory script, documentation/ledger, CMake and CI. Other subsystem source blobs are unchanged. Historical reports are not retroactively considered implementation evidence.

## System-by-system architecture assessment

| Layer | Source files (engine) | Registered test files | Assessment and next contract |
| --- | ---: | ---: | --- |
| Foundation | 7 | 2 | Small platform-neutral `Result/Error`, timing and logging base; preserve no OS headers |
| Platform | 3 | 2 | Windows window and input abstraction exists; widen only through explicit native boundary |
| Runtime | 4 | 2 | Fixed-step scheduler and lifecycle are established; bridge simulation ticks to reusable services instead of growing bootstrap |
| World | 8 | 5 | World owns live EntityIds/positions with cell+local coordinates; lifecycle cleanup for AI registries must be explicit |
| Assets | 40 | 25 | Import/cook/load, hashing, serialization and cache tested; stable cross-session asset/entity identity remains a broader goal |
| Streaming | 21 | 9 | Cell residency, asynchronous IO, revisions and file persistence present; isolate Windows-specific disk operation behind platform adapter |
| Physics | 28 | 18 | Shape bounds, broad/narrow phases and contact math; guard reproducibility and overflow under stress |
| Physical world | 27 | 14 | World-cell collider registry and first-person capsule physics; unloaded cells must yield unavailable rather than fabricated clear sight |
| AI | 81 | 41 | Navigation, schedule, factions, fidelity, vision and per-observer memory; two-NPC batch works in tests, but scale, lifecycle and discovery remain unsolved |
| Rendering | 2 | 1 | Windows/D3D12 clear-color renderer; no final scene graph, GPU scene, virtual geometry or mature rendering pipeline |
| Bootstrap app | 11 | 3 | Two World-owned NPCs now wired to fixed ticks; `main.cpp` has grown to ~2,493 lines and is a demonstration, not a reusable production runtime |

**Future-only architecture:** there is no full engine implementation of audio, editor, gameplay/RPG slice, network replication, replay, modding, Vulkan backend or advanced GPU scene. This is expected before OROS-010 and later milestones. Do not mistake the engine constitution's diagram for compiled systems.

## Priority findings, evidence and remediation

### P0 / architectural scalability — per-tick perception copies memory

`apps/bootstrap/live_ai_perception_group_demo.cpp` constructs fresh actor/fidelity registries, combines **all NPC memories**, and reconstructs per-actor memories per fixed tick. `engine/ai/src/actor_visual_perception_tick.cpp` also copies its combined memory for transaction rollback. This maintains atomicity but allocates/copies each tick, with cost increasing with historical memory rather than only active sensing. **Now:** add a hard caller-supplied request budget with fail-closed validation at both public entry points. **Next:** replace full copies with a two-phase transaction of validated observation deltas, reusable scratch buffers and deterministic per-actor scheduling; benchmark allocations and tail latency. A hard budget bounds *requested pairs*, not total cost of copied memory.

### P0 / operational observability — interactive D3D12 not covered by CI

CI compiles the native app and runs CPU tests but does not interactively launch the D3D12 window. Changes for two live NPCs have passed 121 CTests; this cannot establish their exact in-window log/behavior. **Next:** separate headless integration smoke coverage from occasionally user-assisted native GPU smoke tests, recording version/driver/performance observations.

### P1 / growing bootstrap coordinator and duplicated authority

`apps/bootstrap/main.cpp` is ~84 KB/2,493 lines. It still prepares player motion, authoritative tick times, two NPCs, rendering and console transition reports. Group adapter mirrors actor-owned vision/fidelity into ephemeral registries. **Next:** isolate actor simulation service and lifecycle into reusable modules, keep bootstrap composition thin; avoid new singletons. Validate no mismatch when an actor is destroyed or re-created.

### P1 / platform boundary leakage in streaming persistence

`engine/streaming/src/world_cell_persistence_store.cpp` conditionally includes `<Windows.h>` and calls `MoveFileExW`. Although Windows-first and guarded, file-replacement mechanics are OS concerns inside streaming. The Architecture Constitution directs OS code behind narrow platform interfaces. **Next:** factor atomic file replacement into `engine/platform` (or an equivalent narrow platform service), preserve persistence error contracts, provide failure/crash-injection tests, and only then generalize to a second OS.

### P1 / actor lifecycle, storage and persistence

EntityId uses namespace+sequence; `World` owns live entities/positions. AI registries, schedule state, memory and fidelity are currently tied to in-memory actors. No production durable NPC memory history or persistent world identity through editor/network/replay exists. Deleting an actor without explicit AI cleanup can leave application-owned stale state. **Next:** implement World-observed despawn cleanup and explicit ownership boundaries, then versioned actor-state persistence on the streaming substrate. Maintain distinction between remembered facts and currently visible targets.

### P1 / bounded deterministic work scheduling is incomplete

`ActorVisualPerceptionTickRequest` input sorting ensures repeatable order, and distant fidelity is skipped; there is not yet general target discovery, rotating/fair budgets, visibility prioritization or efficient cross-cell candidate collection. Over-large request arrays could cause excessive allocation/LOS work. **Now:** reject pairs exceeding caller-provided limit *before allocating or touching memory*. **Next:** deterministic budgeted candidate selection with starvation/fairness tests; do not silently drop observations while claiming complete evaluations.

### P2 / build reproducibility and test quality

Current CMake is ~39 KB centralized with 121 targets. Whole-source CI audit catches CMake source omissions, but no Release, ASan/UBSan, fuzzing, allocation benchmark, replay determinism or real D3D12 smoke checks are in the existing Windows CI contract. **Next:** add MSVC Release and targeted stress/failure injection, then modularize CMake when targets can retain audit coverage.

### P2 / persistence concurrency and scale

`engine/streaming/src/world_cell_io_service.cpp` uses threads and synchronization, while persistence store is ~1,726 lines and asset cooked artifact serialization ~1,220 lines. Their boundaries deserve failure-injection and I/O cancellation tests, snapshot checksum/revision migration tests and crash consistency verification before durable AI memory is layered on top. Avoid mixing live runtime World existence with resident collider/asset data.

### P2 / graphics and OS-specific modules are large

Windows platform `window.cpp` (~1,047 lines), D3D12 `renderer.cpp` (~1,458 lines) and physics `narrow_phase.cpp` (~1,133 lines) are sizable modules. Size alone is not a bug; however smaller ownership boundaries and integration probes will make future GPU and physics features safer. C++26 is the target, not a reason to leave the stable C++23 subset prematurely.

## Architecture conformance conclusions

**Preserved in inspected paths:** deterministic simulation time passed by caller; validated World-owned entities and normalized positions; observer/target distinction; collider-residency unavailable vs positive visibility; memory preserving last-known observations; deep_local vs statistical_distant fidelity; result-based errors; strict OROS warnings; test-first incremental integration.

**Still gaps:** unified editor/runtime/save/network ID and event history, scalable multi-NPC perception scheduling, persistent disk-backed NPC memory, cross-platform platform adapter for atomic file replacement, gameplay systems, GPU scene and mature rendering, Release/stress/fuzz/sanitizer test matrix.

## Next 5-command tranche (runs #6–#10)

1. **#6:** two-phase transactional observation deltas to reduce per-tick memory allocations/copies; tests for stale and conflicting facts.
2. **#7:** NPC actor destruction/despawn registry cleanup and identity continuity tests.
3. **#8:** deterministic candidate selection with explicit fairness/priority budget and loaded-cell authority.
4. **#9:** headless runtime multi-NPC smoke and CI Release build plus basic timing/allocation benchmark.
5. **#10:** mandatory new whole-repository inventory and architecture review; compare with this report, reprioritize based on measurements.

Do not treat this sequencing as a promise that previously unverified features are done. Each run must compile and pass CI; adjust priorities if regression or architecture evidence warrants it.

## Audit rule carried forward

This report satisfies the required **run #5** architecture review; the next mandatory audit is **run #10**. Continue whole-source inventory on every CI commit, and avoid claiming full formal line-by-line analysis or GPU window execution solely from CTest.
