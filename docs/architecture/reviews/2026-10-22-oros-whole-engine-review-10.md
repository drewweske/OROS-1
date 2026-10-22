# OROS 1 — Mandatory Whole-Engine Architecture Review #10

**Date:** October 22, 2026  
**Review checkpoint:** `69dce00055fe0a6615332ac181bdf19c55e60d9b` (the last verified revision before this run)  
**Working branch:** `automation/oros-009-chat-build`; draft PR #6, **no merge**  
**Schedule:** mandatory review at invocation **#10**; next **#15**  
**Source of engineering goals:** the project's `fromthecreator.txt` Architecture Constitution and existing OROS reports, particularly `docs/architecture/reviews/2026-10-22-oros-whole-engine-review-05.md`. The long-term goal remains a persistent, scalable first-person open-world simulation engine with future RPG and creator systems. C++26 is a destination; the build intentionally uses MSVC C++23.

## Scope and method

1. Enumerated the **entire Git tree recursively**, without truncation, and compared each tracked blob's path and SHA against mandatory review #5 (`62a708a`). Counted all engine and test modules, source units and bytes, documents, workflow and build files.
2. Reviewed the current whole-source CI inventory, which reads and hashes *every* `.cpp`, `.hpp` and `.h` in `engine/`, `apps/`, `tests/`, reports include relationships and checks CMake registrations. This automatic source-by-source mechanical inventory runs with Windows CI and is **not a substitute** for manual architectural judgment.
3. Conducted a risk-prioritized cross-layer source and API inspection: `World` entity storage and coordinates; foundation result/error; Windows platform; fixed-step runtime; assets load pipeline; streaming persistence and its Windows replacement call; physics shape/segment queries; streamed cell collider residency and first-person controller; actor fidelity, vision, memory and multi-NPC batch; HLSL+D3D12 renderer; CPU graybox projection; bootstrap scene wiring and outpost collision; CI and tests.
4. Reviewed user-provided **real Windows screenshots** of the perspective grid and camera rotation after #7, and recent successful MSVC/CMake/CTest run #9. No interactive user screenshot yet validates the new solid buildings.

**Review precision:** full-repository *structural* coverage and representative critical source/API review, not a formal line-by-line security certification, exhaustive performance profile or test of the user's current GPU at this revision. Claims about the playable scene are restricted to source behavior and the observed older screenshots.

## Objective comparison: review #5 → #10

| Metric | Review #5 (`62a708a`) | Review #10 (`69dce00`) | Delta |
| --- | ---: | ---: | ---: |
| Tracked blobs | 364 | 371 | +7 |
| Tracked repository bytes | 3,992,289 | 4,064,861 | +72,572 |
| C++ `.cpp` files | 232 | 235 | +3 |
| C++ headers (`.h/.hpp`) | 122 | 124 | +2 |
| Total C++ source+header files | 354 | 359 | +5 |
| Whole-source line count (latest CI) | 147,855 | 149,341 | +1,486 |
| CTest executables | 121 | 123 | +2 |
| Missing/stale CMake source registrations | 0/0 | 0/0 | unchanged |

All changes since #5 fall within 21 paths, chiefly graphics (`engine/rendering`), bootstrap composition/outpost collision, two new rendering tests, one outpost regression, the perception-request limit, CMake, workflow and documentation. All existing engine subsystem directories remained in place. Files outside the changed set have identical Git blob IDs; this does not establish their correctness.

**Verified pre-run CI:** [Windows/MSVC Debug push](https://github.com/drewweske/OROS-1/actions/runs/39750097743) and [draft PR](https://github.com/drewweske/OROS-1/actions/runs/39750108261), 123/123 CTest, 0 failures, complete source registration audit. **Run #10's new LOS assertions require separate CI verification**, not an extrapolation from earlier green runs.

## Detailed engine/module review

| Layer | Current engine files | Test files | Implementation status and boundary |
| --- | ---: | ---: | --- |
| Foundation | 7 | 2 | Platform-neutral errors, results, timing/logging; preserve stable contracts |
| Platform | 3 | 2 | Win32 window/input; OS-specific code in platform is expected |
| Runtime | 4 | 2 | Fixed-step scheduler/engine lifecycle; tick ownership remains split with bootstrap |
| World | 8 | 5 | World-owned EntityId namespace/sequence and normalized cell-local WorldPosition; no finalized saved cross-session actor graph |
| Assets | 40 | 25 | Cook/load/cache/hash/resource handles foundation; content pipeline not yet delivering textured world |
| Streaming | 21 | 9 | Snapshot/revision/residency/I/O/persistence exists; Windows atomic replace still resides inside streaming |
| Physics | 28 | 18 | Geometry and narrow-phase, segment tests; build out stress and instrumentation |
| Physical world | 27 | 14 | Authoritative resident cell collision, ray authority, capsule controller; outpost now uses this registry |
| AI | 81 | 41 | NPC schedule, factions, vision/memory, deep-local fidelity, deterministic batch; high copy/allocation overhead remains |
| Rendering | 4 | 2 | New D3D12 HLSL line PSO, CPU-projected world grid, actor posts, pitched-roof building silhouettes; **not** a mesh/terrain renderer |
| Bootstrap application | 12 | 4 | 2 NPCs, player movement, camera yaw, outpost layout & colliders; approximately 2,702-line `main.cpp`, far from minimal |
| Additional tools | 1 source inventory script | — | CI whole-C++-tree inventory/registration verification |

Architectural constitution's audio, editor, RPG gameplay, save-game actor graph, networking, replay, advanced geometry/lighting, Vulkan and modding remain **future designs**, not compiled production systems.

## Findings and prioritized remediation

### P0 — Actor perception memory is copied per fixed tick

The live group adapter combines all NPC memories, invokes the AI batch, then reconstructs each actor's memory on success; the AI batch also stages a full memory copy. This is deterministic and transactional but allocates/copies proportional to *known memories*, not just observations. A request-count cap prevents runaway sight-pair counts, not this cost. **Next:** stage only validated observation deltas, commit atomically with reusable scratch storage, add deterministic behavior and allocation benchmarks before hundreds of NPCs.

### P0 — Platform-independent runtime still grows inside bootstrap

`apps/bootstrap/main.cpp` is ~94.6 KB / ~2,702 lines and coordinates the simulation, camera, physics, rendering, logging, and actor updates. This is reasonable for a proof executable, not for a large game. **Next:** pull World-view camera and per-tick orchestration behind narrow reusable services; preserve World-authoritative positions. Keep the bootstrap as composition and diagnostics.

### P1 — Environment colliders share the floor's EntityId

All three building static BoxShape colliders in `apps/bootstrap/physical_world_demo.cpp` are indexed by the **floor environment EntityId** with distinct shape slots, rather than three distinct World entities. This is valid for a single authored static environment cell, but deleting/changing the floor owner risks conflating unrelated building lifetime and interactions. The buildings cannot yet be individually selected, modified or saved as game objects. **Next:** introduce static-geometry ownership metadata and then first-class building identity with versioned serialization when needed. Keep collider IDs immutable in saved revisions.

### P1 — Visual and physical spawn anchors must remain aligned

Rendering offsets every outpost landmark from a *copy of the player's initial WorldPosition*; the physical cell always bakes the same layout into `WorldCell{0,0,0}` around local origin. This is aligned for today's spawn at local (0,0,0) but will diverge if spawn becomes arbitrary without a unified authored anchor/cell transform. **Next:** eliminate this origin-only assumption and test non-origin spawns/cell transitions before general placement; do not claim robust world placement yet.

### P1 — Collider/world sight continuity lacked outpost-specific tests

The existing tower regression tested capsule movement. It did not test that the actual resident tower collider causes a ray hit and `ActorVisualPerceptionState::occluded`, or that a clear lane remains visible. **Run #10 change:** add both segment and AI perception integration assertions using the same loaded cell and authored tower collider ID. Preserve the distinction between blocked, clear and unavailable streaming authority.

### P1 — Graphics are visible, but only line-rendered, camera-relative geometry

Users confirmed actual GPU output and view turning through screenshots of #7. Since then, the outpost acquired landmarks and box collision; user visual confirmation for those revisions is still pending. `renderer.cpp` now approaches ~1,781 lines and owns shader compilation, buffers, swapchain and draw. The world renderer currently has no proper depth buffer, materials, shadows, scene mesh system, dynamic LOD or textured asset pipeline. **Next:** introduce a basic opaque geometry/depth pass for truly solid cubes/ground and test GPU resource lifetime; maintain rendering API boundaries.

### P1 — Windows persistence internals leak into streaming

`engine/streaming/src/world_cell_persistence_store.cpp` directly includes `<Windows.h>` and uses `MoveFileExW`. It is guarded and works for Windows-first development, but crossing that layer weakens the long-term platform abstraction. Move atomic replacement behind a narrow platform service **after** failure/crash consistency tests, not as an unrelated refactor during graphics work.

### P2 — CI is strong on Debug correctness, limited on runtime assurance

Windows 2022, MSVC 2022, C++23, warnings as errors and **123 CTest executables** are valuable safeguards; CI packages a playtest EXE. Missing verification: Release correctness/performance, sanitizer/fuzz targets, end-to-end World bootstrap movement+render smoke on Windows GPU, CI-based memory-allocation metrics, NPC scheduling stress, save/reload compatibility and determinism across platforms. Do not treat 123 unit-test executables as 123 gameplay features.

### P2 — Streaming and persistence under fault

The asynchronous I/O service and persistence store carry versioned cell snapshots, but expanded moving-world sessions need unload/reload/failure injection, cancellation, asset-revision and crash-consistency testing. A single resident outpost cell is not yet an open-world streaming gameplay pipeline.

### P2 — Toolchain and repo maintenance

`CMakeLists.txt` is around 40 KB with individual test registrations and a whole-source audit; keep stable test naming, eventually modularize CMake by engine layer. `AGENTS.md` had a stale 2026 file-by-file handoff: the run #10 change refreshes the GitHub CI handoff and audit checkpoint. Do not prematurely switch to C++26 when MSVC support is incomplete.

## Implementation added in run #10

- Expanded `tests/bootstrap/outpost_collision_tests.cpp` with resident tower vs open-lane line segments and NPC perception assertions. A World ray crossing the tower must report `blocked` with the green tower's exact `ColliderId`; an unobstructed parallel ray must report `clear`; NPC-facing target queries must return `occluded` behind the tower and `visible` in the open lane.
- Linked this existing bootstrap CTest to `oros_ai`. No runtime duplicate collider representation introduced. The outpost still has 4 loaded collider volumes (floor + 3 buildings), and this remains a bootstrap-specific integration test, not a World-wide AI event system.
- Refreshed `AGENTS.md` and the development run ledger; mandatory audit now rolls forward to **#15**.

## Runs #11–#15: intended incremental roadmap

1. **#11** Render solid, depth-tested ground/building geometry (or establish a smaller mesh pass first), while keeping collision layouts in sync.
2. **#12** Extract camera/world-scene composition from bootstrap; validate stable view anchors across non-origin spawn coordinates and different WorldCells.
3. **#13** Add durable actor/environment lifecycle cleanup and focused spatial identity tests.
4. **#14** Implement more efficient transactional actor-perception memory updates and a targeted allocation/determinism benchmark, or reprioritize based on profiled data.
5. **#15** Mandatory next entire-codebase architecture review with whole-source inventory comparison, changes, CI and test delta, risk reassessment and recommendations.

The schedule is an engineering plan, not a statement that unbuilt features are complete. No automatic merging, no laptop changes and no user Codex or PowerShell are needed for GitHub CI development.
