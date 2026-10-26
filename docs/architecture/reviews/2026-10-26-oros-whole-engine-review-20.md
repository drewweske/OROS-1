# OROS 1 — Mandatory Whole-Repository Architecture Review #20

**Date:** 2026-10-26. **Checkpoint invocation:** Build OROS **#20** (every fifth run).  
**Baseline reviewed:** `automation/oros-016-world-growth` SHA `f332bdda3903404365a67541cc9965dc82008346` prior to #20 changes. **Prior checkpoint:** [review #15](2026-10-22-oros-whole-engine-review-15.md), committed post-run SHA `9705e6e7af05b7d66ae00c0d642e458c3281cadc`. Official `main` remains `31937ead`; [draft PR #9](https://github.com/drewweske/OROS-1/pull/9) remains unmerged.

## Method and verification boundary

Compared **all tracked paths and blob hashes** in both full recursive Git trees (both `truncated=false`). Inspected current `AGENTS.md`, `fromthecreator.txt`, exact branch/pr status, #15 architectural review, run ledger, engineering backlog, CMake registration and Windows GitHub Actions workflow. Reviewed representative critical source/API/test contracts across all layers, including World, AI, streaming, collider registry and segment queries, renderer, runtime, assets and bootstrap; unchanged code was classified by its unchanged Git blob hash and prior review. This is **whole-repository structural and risk-targeted architecture review, not a line-by-line security audit**.

**Pre-run verified exact SHA:** `f332bdd` ([PR](https://github.com/drewweske/OROS-1/actions/runs/40298976073), [push](https://github.com/drewweske/OROS-1/actions/runs/40298969406)), Windows Server 2022/Visual Studio 2022/MSVC C++23, Debug, strict OROS warnings. **127/127 CTest executables passed**, zero failures; 367 C++ source/header files, 241 translation units, 151,421 source lines, **zero missing/stale CMake registrations**. These numbers describe registered tests, not assertion totals or interactive GPU/Release/stress outcomes. The new #20 commit must pass its *own* exact-SHA CI before being called verified.

## Full untruncated source-tree comparison (#15 → #20 pre-run)

| Metric | #15 post-run `9705e6e` | #20 pre-run `f332bdd` | Delta |
| --- | ---: | ---: | ---: |
| Tracked blobs | 380 | 383 | +3 |
| Git tracked bytes | 4,148,299 | 4,185,411 | +37,112 |
| C++ source/header files | 365 | 367 | +2 |
| C++ translation units | 239 | 241 | +2 |
| Registered CTest executables | 125 | 127 | +2 |
| CI source lines | 150,562 in #15 pre-run review | 151,421 | +859 vs reported pre-run baseline |
| Missing/stale CMake entries | 0 / 0 | 0 / 0 | no regression |

**Full-tree path inventory diff:** 17 changed or added tracked paths (14 modified, three added); **366 unchanged Git blobs; no deletions**. Added: `docs/development/oros-canonization.md`, `tests/world/world_population_growth_tests.cpp`, `tests/world/world_restore_tests.cpp`. Modified paths comprise `.github/workflows/oros-windows-ci.yml`, `AGENTS.md`, `CMakeLists.txt`, bootstrap NPC perception, run ledger, backlog, AI memory API/implementation/tests, World identity API/implementation and group tests. No production `engine/*` module was added or removed.

## Whole-repository subsystem inventory

Counts below include tracked `*.cpp`/`*.hpp`/`*.h` in production and test `*.cpp` files. Each subsystem is inventoried; findings below are based on risk-targeted inspection, not proof of every statement in 151k source lines.

| Layer | Production files | Test units | Capability / limitation |
| --- | ---: | ---: | --- |
| Foundation | 7 | 2 | Typed Result/Status, logging and clocks; full telemetry absent |
| Platform | 3 | 2 | Win32 window/input; storage platform APIs remain in streaming |
| Runtime | 4 | 2 | Fixed-step scheduler, lifecycle; bootstrap orchestrates main gameplay |
| World | 8 | 7 | World-owned identity, position, lifecycle; versioned object graph not implemented |
| Assets | 40 | 25 | Versioned import/cook/cache/resource handles; renderer not yet consuming textured cooked assets |
| Streaming | 21 | 9 | Cell snapshots, revision/manifest/residency and asynchronous I/O; crash-atomic object save not proven |
| Physics | 28 | 18 | Mathematical collision/broad/narrow phase; no full rigid-body dynamics |
| Physical World | 27 | 14 | Shared resident collision, character movement, LOS; all-cell scan cost risk |
| AI | 81 | 41 | Vision, schedules, faction, navigation, memory and fidelity types; mass actor work unmeasured |
| Rendering | 4 | 2 | D3D12 opaque/depth/debug geometry; no asset-driven world graphics |
| Bootstrap | 16 | 6 | Camera/actor/outpost demos; large centralized `main.cpp`, not reusable full game |
| Source review tooling | 1 Python | — | Inventory and CMake registration audit, not formal security audit |

Unavailable/planned (not milestone complete): RPG inventory/crafting/survival and provenance, Forge editor, replay/journal, server replication, audio/animation/UI, mod sandbox, Vulkan, world-scale profiling.

## Critical cross-layer findings and repairs

**1 — P1 identity continuity, World↔Persistence.** `World::create_entity` and `EntityIdGenerator` maintain monotonic namespace+sequence IDs. #16 removed repeated `unordered_set::reserve(size+1)`; #19 added `World::restore(world_namespace,last_issued_sequence,live_entities)` with duplicate, invalid, future, foreign and UINT64_MAX failure paths. **Observed gap at checkpoint:** World has no public way for future save code to read the *issued* high-water mark after the highest live ID was destroyed. The highest living or currently resident EntityId is not safe to use. **#20 repair:** `EntityIdGenerator::last_issued_sequence()` and `World::last_issued_sequence()`; direct regression verifies 0 before issuance, increments, deleted-highest still retained, round-trip restore and 64-bit exhaustion. This closes a source-contract gap, **not a durable transactional save**. A falsely low persisted mark can still allow reuse; save writers must record global issue state and schema/revision atomically alongside object ownership, provenance and life-cycle events.

**2 — P1 active object identity and retirement contracts.** Three outpost buildings in `apps/bootstrap/outpost_layout.hpp` are collider slots on a *single shared floor environment* EntityId, not separately persistent entities. `apps/bootstrap/live_ai_actor_lifecycle_demo.cpp` cleans active vision/fidelity/memory after a World destroy but not all future schedule/faction/save/network references. Live World identity is separate from future nonresident existence only as an architectural intent, not yet an end-to-end serialized object/actor graph. Implement one independently owned authored object and end-to-end save/reload/unload identity/provenance regression before growth in interactive objects.

**3 — P1 streaming crash recovery and platform boundary.** `engine/streaming/src/world_cell_persistence_store.cpp` (~1,726 lines) handles immutable snapshot and manifest files, checksums, temporary writes and on Windows `MoveFileExW(MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)`. File stream flushing and manifest publication do **not** prove durable multi-file transaction across power loss. Existing tests include corrupt bytes and schema/version limits but not systematic fault injection across publication phases. Need explicit old-valid/new-valid revision recovery and later narrow platform file replacement abstraction; never silently weaken atomicity.

**4 — P1 AI and physics/LOS scale.** #17 all-distant fast path and #18 single canonical memory vector improve algorithmic work, but in mixed `deep_local` ticks the bootstrap still builds combined memories/registries and the perception tick stages a transactional copy on visibility. `WorldCellColliderRegistry::colliders_relative_to()` walks **all resident cells**, transforms colliders and sorts a vector per segment query, including AI line-of-sight. This is an observed complexity hazard, **not measured FPS regression**. Test nearest-hit vs unavailable precedence, cell edges and bounded request counts before filtering; benchmark 2/100/1000 NPCs and 1/10/100 resident cells with allocation/timing methodology and real hardware identity.

**5 — P1/P2 runtime, assets and D3D12 presentation.** `apps/bootstrap/main.cpp` still centralizes world composition and scheduling, even though `engine/runtime` has fixed-step primitives and `engine/ai` holds reusable state and navigation contracts. `engine/assets` exposes cooked artifacts with versions/resource handles but `engine/rendering/src/d3d12/renderer.cpp` continues a debug/solid pipeline, device/swapchain/resizing and untextured outpost. Do not let camera/renderer handles become persistent World identity or infer complete terrain/material support. Personal GPU evidence confirms an earlier grid/turning revision only.

**6 — P2 CI/quality gates.** Only Windows/MSVC Debug CMake/CTest source audit is automated. No separate Release/benchmark/stress CI or crash-fault replay tests; GitHub's headless D3D12 test is not the user's GPU playtest. No global AVX2 or experimental C++26 requirements observed in current CMake. The source audit counted zero CMake registration gaps, but assertions and load/fault coverage must be separately measured.

## Prioritized remediation and acceptance matrix

| Rank | Priority | Bottleneck | Acceptance gate |
| --- | --- | --- | --- |
| 1 | P1 continuity | B004/B008/B010/B012: partial persistent identity and provenance | One individually identified authored World object across relocation, unload/save/restart/reload, consistent global high-water and correct destruction/tombstone |
| 2 | P1 corruption | B005: save publication and recovery | Inject interrupt/fault at write, flush and manifest swap; recover last committed revision or explicit failure without identity reuse |
| 3 | P1 scale | B011: LOS all-resident-cell conversion per query | Multi-cell invariants and before/after allocation+timing profiles; nonresident never falsely clear |
| 4 | P1 scale | B001/B002: mixed NPC memory copying | Controlled 2/100/1000 NPC workloads with memory histories and unchanged stale/rollback decisions |
| 5 | P1 architecture | B003/B008: bootstrap orchestration/lifecycle | Reusable fixed-tick roster/lifecycle service with WorldTime tests |
| 6 | P2 quality | B007: only Debug CI | Release CMake/CTest, crash and scalable runtime instrumentation, named hardware |
| 7 | P2 playability | B006: no asset-driven D3D12 scene | Cooked mesh/material displayed on GPU, World/collider agreement |

## Build #20 bounded implementation and skeptical review contract

**Scope:** World/EntityIdGenerator watermark accessors and tests in existing `tests/world/entity_id_tests.cpp`, `tests/world/world_restore_tests.cpp`; mandatory review, ledger, backlog and AGENTS. No save schema/API generated prematurely. Tests should cover zero/normal/max watermarks, after deletion, restart monotonicity, malformed high-water rejection already covered by #19. No claim of measured performance or saved actors.

**Skeptical failure checks:** UINT64_MAX generator is exhausted without wrap; first issuance yields 1 after zero; highest retired entity does not rewind; World restore only trusts caller-supplied global high-water and cannot detect a *plausibly* lowered mark without durable provenance; error paths never publish partially restored world. No multithreaded live mutation contract is introduced.

**CI gate:** full Windows 2022/MSVC Debug CMake with warnings-as-errors + all registered CTest at the new SHA, inspect logs and retest fixes. Keep PR #9 draft, no merges, no user laptop changes.

**Next Build #21:** integrate a versioned World object/identity snapshot with explicit same-transaction high-water and ownership, and a regression for save/restart/unload/reload, unless a higher-severity failure is discovered. Next formal review #25; mainline canonization evaluation #30.

**Review limitations:** one assistant performed sequential scout, reviewer and engineering roles, not actual parallel workers. This checkpoint is a structural source inventory plus critical-path review, not a formal line-by-line security audit; no personal GPU, Release benchmark or fault-injection stress test performed as part of this review.
