# OROS 1 — Whole-Repository Architecture Baseline (Build OROS #2)

Date: 2026-10-21  
Review target: `automation/oros-009-chat-build`, implementation checkpoint `0a112bc832e35e035a7abfca54493a5115b4830c`  
Planning basis: repository creator vision, `docs/architecture/OROS_1_Current_CPP_Architecture_Through_OROS-008.docx` (reference document), and the OROS 1 project Architecture Constitution (C++26 destination, C++23 stable subset). The DOCX reference is older than OROS-009 and is not assumed to describe new AI code.

## Review method and limitations

- Enumerated the **entire tracked repository tree** (352 files and about 3.9 MB before this run's three new source/test files): bootstrap application, all engine and test modules, CMake, GitHub workflow, planning documents and creator vision.
- Reviewed build-target structure and representative implementation/contracts from world coordinates and entity storage, foundation errors, runtime scheduling, D3D12 renderer, platform, assets, streaming persistence, physical-world colliders, AI faction/focus/fidelity/perception/vision, the bootstrap loop and relevant integration tests.
- Verified the **full Windows/MSVC Debug build and 119/119 CTest test targets** on this checkpoint: https://github.com/drewweske/OROS-1/actions/runs/39592155574 .
- Added a full-tree C++ source inventory script on this review branch: it enumerates every engine/app/test `.cpp/.hpp/.h`, hashes contents, derives module include relationships, inventories TODO/FIXME/HACK, and checks all C++ translation units against CMake registration. This inventory runs at every CI build. The first run counted **348 C++ source/header files, 228 translation units and 146,583 source lines**; it found **zero** missing and **zero** stale CMake registrations. Two bootstrap sources are intentionally compiled in multiple targets, which is permissible.
- **Scope qualification:** the full inventory and test/build checks cover the repository mechanically; the manual review is architectural and risk-prioritized, not a formal line-by-line security proof of 3.9 MB or a dynamic test of every path. Actual D3D12 window behavior was not rerun here.

## Whole-engine system inventory

These were tracked file counts at the pre-implementation review snapshot (include headers, implementation, tests; new registry adds two AI source files and one AI test).

| System | Engine files | Test files | Status against constitution |
| --- | ---: | ---: | --- |
| Foundation | 7 | 2 | Result/ErrorCode, assertions, logging, clocks; deliberately small |
| Platform | 3 | 2 | Windows window and input wrapper; Windows-first |
| Runtime | 4 | 2 | Engine lifecycle plus fixed-step scheduler; deterministic baseline |
| World | 8 | 5 | World-owned EntityIds, positions and component storage |
| Assets | 40 | 25 | Import, cook, load, catalog, caching and artifact serialization foundation |
| Streaming | 21 | 9 | Cell residency, asynchronous I/O, snapshots and persistence foundation |
| Physics | 28 | 18 | Contact geometry, broad/narrow phase |
| Physical world | 27 | 14 | World-cell collider authority, first-person capsule controller and traversal |
| AI | 77 | 39 | Navigation, schedule, factions, focus policy, fidelity, vision and memory; now adding multi-actor vision registry |
| Rendering | 2 | 1 | Direct3D12 initialization and clear-color renderer, not a content-rich scene renderer |
| Bootstrap app | 9 | 2 | Integrates existing systems; not yet a reusable production game runtime |

No implemented engine folders yet for the Constitution's future **audio, editor, gameplay, networking, replay, advanced rendering, modding, or Vulkan** layers. This is expected at OROS-009; do not claim those systems are complete.

## Detailed architecture findings and priorities

### High priority — bootstrap app is over-centralized
`apps/bootstrap/main.cpp` is approximately 82 KB and owns substantial demonstration setup, proof logic, simulation stepping, renderer integration and diagnostics. It is functional and tested at executable level, but creates risk of future subsystem coupling. **Action:** extract a reusable actor-perception tick orchestration with explicit World, collider and actor inputs; later split bootstrap wiring from core runtime. No premature rewrite.

### High priority — multi-NPC work is only a foundation
Actor-owned `ActorVisionState` and the new canonical `ActorVisionStateRegistry` supply independent observer configurations; the running bootstrap still creates a single live NPC and stores its state in `LiveAiActorDemo::vision_state`. **Action:** add a validated World-owned multi-actor composition/service adapter, explicit target discovery and scheduling; test cross-actor memory isolation and deep-local budget rules before hooking a live simulation.

### Medium priority — simulation fidelity vs persistent continuity
AI fidelity uses actor-sorted registries. Existing visual-memory tests preserve last-observed positions through loss of sight and fidelity changes, while expensive sight queries are permitted only at deep-local fidelity. **Action:** add multi-NPC transition and observation tests; ensure actor destruction cleans per-actor state; avoid conflating unloaded collider authority with negative observations.

### Medium priority — World and persistence identity integration
EntityId uses a namespace/sequence; World owns live positions; streaming has revisioned cell payload/snapshot/persistence components. These are compatible beginnings for Persistent Continuity Architecture, **not yet** a universal identity/history spanning saving, networking, replay and editor. **Action:** design stable cross-session identity and serialized actor vision/memory only when the world persistence layer can own it safely. Do not imply actor vision persists to disk today.

### Medium priority — CI breadth and diagnostics
Windows Server 2022/MSVC Debug builds with strict OROS warnings and all CTest targets. It does **not** currently validate Release performance, alternative compilers, runtime D3D12 interactions, threading stress, deterministic replay or fuzzed streaming boundaries. **Action:** add Release and targeted stress/fuzz/determinism checks incrementally without blocking AI milestone delivery on speculative cross-platform code.

### Medium priority — monolithic CMake registration
`CMakeLists.txt` is approximately 38 KB and registers 10 engine library targets, bootstrap targets and 119 CTest executables. It is transparent but becomes hard to audit as the engine grows. **Action:** split registration by module with stable target ownership after CI inventory can validate the migration.

### Low priority — C++23 vs C++26
The Architecture Constitution targets C++26, but using the proven C++23 subset with Visual Studio 2022 is the deliberate, correct current engineering choice; do not depend prematurely on incomplete standardized reflection/contracts/execution support.

### Low priority — D3D12 readiness
The current bootstrap draws a clear-color window. The planned editor, render graph, GPU scene, virtualized geometry, advanced lighting and Vulkan backend are future milestones. Runtime behavior still needs occasional user-assisted Windows interactive smoke tests.

## Architecture boundaries that should remain invariant

1. Foundation should remain platform-independent; world should own EntityId and authoritative WorldPosition.
2. Streaming and physical collision should expose loaded-authority facts; missing cells are neither clear visibility nor negative perception.
3. Local perception reads collision/world facts and produces validated observations; deterministic WorldTime advances from fixed simulation ticks.
4. NPC memory, orientation, schedules and factions survive fidelity changes; no routine AI allocation/fidelity logic belongs inside a monolithic application loop.
5. Tests should assert behavior, invalid input handling, determinism, actor identity and lifecycle, not only successful compilation.

## Review cadence

This **extra baseline review at run #2** does not replace the required review at run **#5**, then **#10**, **#15**, etc. Every fifth run must repeat the whole-tree inventory, review architecture boundaries, identify changes since the last review, prioritize risks, and save another report. Source-inventory CI is run on **every commit**, as the mechanical companion to the five-run architectural analysis.
