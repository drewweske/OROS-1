# OROS 1 — Mandatory Whole-Engine Architecture Review #15

**Checkpoint date:** October 22, 2026  
**Snapshot inspected (verified pre-run baseline):** `fa6cae03b8e0a8f4d1e4f251734330d51e0c51a7`  
**Working branch:** `automation/oros-009-chat-build`, Draft PR #6 onto `feature/oros-009-living-world`; not merged  
**Compared to review #10:** `69dce00055fe0a6615332ac181bdf19c55e60d9b`  
**Next mandatory review:** Build OROS #20  
**Guiding authority:** `fromthecreator.txt` OROS Architecture Constitution and OROS-000–020 roadmap; implementation authority is the Git source and exact-SHA test logs, not speculative reference designs.

## Method, evidence, and limitations

- Inspected the **complete recursive Git tree** for both revisions (both GitHub trees reported `truncated=false`), counted each tracked blob, C++ unit, header and module, then compared path/blob hashes. Inspected all changed path names; paths with unchanged blob hashes retain identical content.
- Reviewed current `AGENTS.md`, run ledger, dedicated prioritized backlog, review #10, CI workflow and CMake registration. Reviewed critical API/source behavior across World, AI, Physical World, Physics, streaming/persistence, runtime scheduler, assets, rendering and bootstrap. Review of other subsystems uses their inventory, previous report findings and unchanged blob hashes.
- Verified **pre-run** Windows 2022/MSVC Debug builds at [push](https://github.com/drewweske/OROS-1/actions/runs/39758279687) and [PR](https://github.com/drewweske/OROS-1/actions/runs/39758272826): **125/125 CTest targets**, source audit 365 C++ files, 239 translation units, 150,562 source lines and no missing or stale CMake registrations.
- This is a **whole-repository structural and risk-targeted architecture review**, not a formal line-by-line security audit, stress/performance measurement, crash-fault validation or new interactive GPU playtest. The user’s screenshots confirmed an earlier D3D12 graybox and turning but cannot validate the solid-renderer revision on their own GPU.

## Objective comparison (#10 → #15, pre-change)

| Tracked metric | #10 `69dce00` | #15 baseline `fa6cae0` | Difference |
| --- | ---: | ---: | ---: |
| Git tracked blobs | 371 | 379 | +8 |
| Git tracked byte sum | 4,064,861 | 4,129,807 | +64,946 |
| C++ `.cpp` translation units | 235 | 239 | +4 |
| C++ headers (`.hpp/.h`) | 124 | 126 | +2 |
| C++ total sources and headers | 359 | 365 | +6 |
| CTest registered executables | 123 | 125 | +2 |
| Latest CI source lines | 149,341 (#10 baseline) | 150,562 | +1,221 |
| Missing or stale CMake registrations | 0/0 | 0/0 | unchanged |

28 tracked paths changed or were added since review #10; zero tracked files were removed. Eight net new paths comprise two bootstrap components (scene composition, NPC lifecycle; each header + implementation), two regression translation units, a #10 review document and a development bottleneck backlog. No other engine module changed outside AI and rendering. The current engine C++ source inventory remains bounded and fully registered.

## Whole-repository module inventory

Counts include `.cpp` and `.hpp/.h` under engine/apps/tests; the root CMake, workflows, documents and source-audit script are counted separately.

| Subsystem | Production files | Test translation units | Current responsibility & assessment |
| --- | ---: | ---: | --- |
| Foundation | 7 | 2 | Typed Result/error/status, time/logging; small cross-platform base, minimal runtime instrumentation |
| Platform | 3 | 2 | Win32 window/input; correct Windows boundary for interactive demo, future portable services incomplete |
| Runtime | 4 | 2 | Fixed-step scheduler and lifecycle; authoritative sequencing remains composed by bootstrap |
| World | 8 | 5 | EntityId namespace/sequence, World-owned normalized positions and despawn; saves do not yet preserve full actor lifecycle |
| Assets | 40 | 25 | Versioned cooked resource/cache/handle pipeline; not yet feeding a textured world scene |
| Streaming | 21 | 9 | Cell snapshots, revision/residency and disk I/O; direct `Windows.h` dependency inside persistence storage |
| Physics | 28 | 18 | Collider geometry, mathematical intersections/queries; physical-authority and streamed boundaries need larger stress |
| Physical World | 27 | 14 | Resident-cell collider sets and LOS/controller; blocked vs unavailable separated, outpost uses shared collision authority |
| AI | 81 | 41 | 2-NPC fidelity/vision/memory/schedule/faction foundation; duplicate per-tick bootstrap memory staging remains costly |
| Rendering | 4 | 2 | D3D12 swapchain/opaque triangle depth path + line grid/NPC debug markers; no asset-driven mesh/material/terrain |
| Bootstrap | 16 | 6 | Scene camera, NPC demos, explicit retirement and authored outpost; still centrally orchestrated by large `main.cpp` |
| Source-audit tool | 1 script | — | Whole C++ source inventory/CMake registration audit on each Windows CI run |

**Absent planned production layers:** no complete Forge editor, game save/replay actor graph, Nexus networking, RPG survival/crafting systems, audio/animation stack, Vulkan renderer, or mod sandbox. Do not imply milestone completion from graybox output.

## Cross-layer critical findings

### P0/P1: Per-tick AI memory work scales with all known observations
**Observed.** `engine/ai/src/actor_visual_perception_tick.cpp` now makes a *lazy* staged copy only if one target is visible, and the bootstrap avoids repartitioning back on fully nonvisible ticks. Tests exercise 64 repeated inactive ticks. However `apps/bootstrap/live_ai_perception_group_demo.cpp` still repopulates a global combined-memory buffer using `remember()` for every actor on every tick, plus fresh vision/fidelity registries and sorted requests. This creates work proportional to remembered history even when no NPC is in deep-local detail. Need instrumentation before forecasting capacity.

**#15 repair:** expose `ActorVisualObservationMemory::replace_canonical(span)` to validate a sorted unique batch once, copy it once into replacement storage and noexcept-swap it. The bootstrap splits committed combined memory into each actor in one canonical copy per slice rather than repeatedly invoking `remember()`. Invalid, duplicate and out-of-order input must be rejected without losing the prior state; tests also check self-borrowing input.

**Residual:** still copies every actor's prior memory to assemble temporary combined input, and visible-tick staged copying remains; benchmarks for 2/100/1000 NPCs under fixed workload with allocation counters are the next meaningful scale gate. Do not claim an FPS gain from this code path alone.

### P1: World identity, lifecycle and persistence incomplete
`World::destroy_entity` owns lifetime for live EntityId and position; #13 added explicit retirement cleanup of vision/fidelity/memory. Schedules, faction relations, mod/replay/network references and saved actor graph do not share a complete lifecycle bus. Static outpost buildings still share a floor environment owner with different collider slots, not independent persistent IDs. **Action:** design first-class authored/persisted object identity and entity teardown contracts; test non-reused IDs, unload/save/reload and provenance before expanding interactive world objects.

### P1: Bootstrap and coordinate authority
#12 extracted `live_debug_world_scene` and corrected the visual anchor to the same origin used by resident outpost colliders, including a cell-edge test. However `apps/bootstrap/main.cpp` remains a monolithic orchestration entry point, and the outpost is authored only in the origin cell. **Action:** continue extracting services into `engine/` when truly reusable, and preserve absolute World-cell+local semantics.

### P1: Streaming save transaction and platform dependency
`engine/streaming/src/world_cell_persistence_store.cpp` remains a roughly 1,726-line module and directly calls Win32 APIs for file replacement. Atomic file replacement's crash behavior needs targeted fault injection and schema version/revision tests before relocating that service behind a platform boundary. Stored cells alone are not a complete persistent actor/item provenance system.

### P1: Physics and actor LOS
The resident collider registry, player capsule movement and NPC sight query share World Cell collision geometry. #9–#10 integrated tower collision and occlusion tests; nonresident authority is reported unavailable rather than clear. Future risk: multiple streamed cells, high-speed movement and position extremes require bounded work and deterministic replay tests. No new physics implementation was needed in #15.

### P1: D3D12 rendering and graphics ownership
#11 added flat-shaded opaque triangles and a D32 depth buffer while preserving a line grid and actor posts. The current `engine/rendering/src/d3d12/renderer.cpp` is ~1,977 lines, combining device lifetime, swapchain, shaders, vertex uploads, draw and resize. Missing textured cooked assets, camera matrices, true meshes, render graph, LOD, performance counters and native screenshots of this newer solid build. **Action:** profile resource lifetimes and extract interfaces before multiplying passes.

### P2: Verification, stress and performance gates
CI on `windows-2022` builds MSVC Debug C++23 with strict warnings; all 125 CTest targets passed at pre-run SHA. CI source audit counted 365 source/header files and zero registration gaps; CTest executable count is not an assertion count. CI still lacks a distinct Release and allocation/performance benchmark, deterministic replay workloads, persisted actor save/reload, crash-fault simulations and personal GPU comparison. **Action:** add comparable CPU benchmark (observer count, memory size, request count, CPU model, timing spread, allocation count) and Release jobs after prioritizing test cost.

## Risk ranking and acceptance matrix

| Priority | Backlog | Acceptance gate |
| --- | --- | --- |
| 1 / P1 | B002 AI bootstrap combined memory churn | Benchmark 2/100/1000 NPCs; remove repeated reconstruction while preserving transactional stale fact rollback |
| 2 / P1 | B008 incomplete NPC lifecycle | Complete schedule/faction/persistence cleanup with stable World identity and failure atomicity |
| 3 / P1 | B004 static outpost entity identity | Independently save/delete/unload/reload one building with same persistent ID |
| 4 / P1 | B005 streaming Win32 leakage/crash transaction | Fault-injection and platform abstraction preserving atomic revisions |
| 5 / P1 | B003 bootstrap monolith | Extract per-tick components; CI verifies WorldTime and authoritative World states |
| 6 / P2 | B007 insufficient Release/performance CI | Release and allocation/throughput stress jobs with retained machine parameters |
| 7 / P2 | B006 renderer feature gap | Asset-driven opaque mesh rendered on user hardware with consistent collider alignment |

## Changes delivered by this run

- `engine/ai/include/oros/ai/actor_visual_observation_memory.hpp`: canonical bulk replacement contract.
- `engine/ai/src/actor_visual_observation_memory.cpp`: full-key validation, single staged vector construction, noexcept commit; rejects invalid/duplicate/unsorted facts without modifying original memory.
- `apps/bootstrap/live_ai_perception_group_demo.cpp`: atomically rebuild each actor's memory from its sorted span in one operation.
- `tests/ai/actor_visual_observation_memory_tests.cpp`: verify preservation, malformed order, duplicate keys, self-borrowed storage and clearing.
- `docs/development/build-oros-runs.md`, `docs/development/oros-bottleneck-backlog.md`, `AGENTS.md`: ledger #15, updated risk backlog and handoff.
- This report is committed to `docs/architecture/reviews/2026-10-22-oros-whole-engine-review-15.md`. The test suite remains **125 registered targets**; new checks expand assertions within an existing target.

**Run #15 verification status at authoring:** New commit requires its own Windows/MSVC/CTest job and log inspection. The last *already verified* SHA is `fa6cae0`, 125/125. Do not cite the old CI result as new-commit verification.

## Handoff: builds #16–#20

- **#16:** establish comparable deterministic AI memory/LOS benchmarks with explicit allocations and scaled population cases.
- **#17:** remove bootstrap temporary combined-memory copying through engine-owned canonical memory or staged per-actor deltas once measurement identifies the cost.
- **#18:** harden World cell persistence under interruption/failure; separate resident collision from persisted authoritative object lifetime.
- **#19:** target next measured scale bottleneck and return to solid-world GPU playtesting with validated debug and frame metrics.
- **#20:** mandatory full-repository review comparing to this checkpoint, record CI and benchmark changes, re-rank architectural risks.

No parallel subagents or personal GPU execution are claimed. Protected integration history and user-local uncommitted files are untouched.
