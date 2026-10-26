# Build OROS — Run Ledger

This file makes the ChatGPT-only development cadence verifiable across sessions.

**Counting:** One explicit user request containing "Build OROS" = one run. AI retries, CMake builds, commits and GitHub Actions runs do not increment the count.

**Review checkpoint:** every fifth run (#5, #10, #15, ...). Also run a requested out-of-cycle audit.

| Run | Date | Before -> after | Scope | Review |
| --- | --- | --- | --- | --- |
| 1 | 2026-10-21 | `7be2a33` -> `4dc8322` | Initialize actor-owned vision, fixed-tick adapter, persistence tests | Not due |
| 2 | 2026-10-21 | `4dc8322` -> `0a112bc` and follow-up audit commit | Entity-sorted multi-NPC vision registry, comprehensive baseline structural/code architecture review, whole-tree CI source audit | **Extra baseline completed** |
| 3 | 2026-10-22 | `87018bc` -> `5738c69`, plus exception-safety finalization | Deterministic, transactional multi-NPC fixed-tick perception batch with actor-independent memories; Windows/MSVC 120/120 CTest passed on initial implementation | Not due |
| 4 | 2026-10-22 | `1d86b66` -> `bf83a85`, test hardening | Wire two real World-owned NPCs to the fixed-tick perception batch, preserve separate actor memories, add bootstrap group integration tests; Windows CI verification in PR #6 | Not due |
| 5 | 2026-10-22 | `62a708a` -> run-5 audit and budget commit | Mandatory whole-codebase architecture review, compare baseline, add bounded perception requests with regression tests, Windows build and test verification | **Mandatory full review completed** |
| 6 | 2026-10-22 | `4ef9e17` -> `e70812b` | First D3D12 wireframe graybox, two actor posts, World-relative player view; Windows CI 122/122 CTest passed | Not due |
| 7 | 2026-10-22 | `e70812b` -> run #7 camera/artifact integration | Fixed-tick camera yaw, view-relative WASD movement, near-plane line clipping, and downloadable GitHub Actions Windows executable | Not due |
| 8 | 2026-10-22 | `d6d8731` -> `6e948ab` | Screenshot-guided graybox: three World-anchored decorative wireframe outpost landmarks; Windows/MSVC 122/122 tests passed | Not due |
| 9 | 2026-10-22 | `6e948ab` -> `69dce00` | Authored outpost layout connects three loaded box colliders to renderer; Windows/MSVC 123/123 CTest passed, playtest artifact published | Not due |
| 10 | 2026-10-22 | `69dce00` -> `d6c876b` | Mandatory full-engine architecture review, regression for AI sight through the same loaded outpost walls; Windows CI 123/123 passed | **Mandatory review completed** |
| 11 | 2026-10-22 | `d6c876b` -> `821d52b` | Solid D3D12 ground and pitched-roof buildings with GPU depth testing; Windows CI 123/123 tests passed | Not due |
| 12 | 2026-10-22 | `821d52b` -> `594cdce` | World-relative scene extraction with origin-anchored outpost and World-cell boundary tests; Windows CI 124/124 passed | Not due |
| 13 | 2026-10-22 | `594cdce` -> `ab2a8af` | Explicit NPC retirement and visual-memory cleanup; Windows/MSVC CI 125/125 CTest passed after test-fixture corrections | Not due |
| 14 | 2026-10-22 | `ab2a8af` -> `fa6cae0` | Lazy memory staging on visible ticks only; pointer-stable inactive memory across 64 repeated ticks; Windows/MSVC 125/125 CTest passed | Not due |
| 15 | 2026-10-22 | `fa6cae0` -> `9705e6e` -> official `main` merge `31937ea` | Mandatory full architecture review #15 and atomic canonical memory import; 125/125 Windows CTest passed; owner-authorized PR #8 merged preserving history | **Mandatory review and 15-build canonization completed** |
| 16 | 2026-10-22 | `31937ea` -> `b527655` | World entity capacity-growth fix, 10,000-entity lifecycle regression and mainline 15-build canonization policy; Windows/MSVC 126/126 CTest passed | Not due |
| 17 | 2026-10-22 | `b527655` -> `eb73508` | All-distant NPC fast path with strict World/request validation; Windows/MSVC 126/126 CTest passed | Not due |
| 18 | 2026-10-22 | `eb73508` -> `4c9cf35` | Canonical owned NPC memory batching, invariant tests; exact-SHA Windows CI 126/126 passed | Not due |
| 19 | 2026-10-26 | `4c9cf35` -> `0c17fcf` | Restore existing World entity IDs using authoritative issued-sequence watermark; duplicate/foreign/malformed/overflow rejection, 127th CTest; **exact-SHA Windows 2022/MSVC Debug 127/127 passed**, source inventory 367 files/241 translation units, zero CMake gaps ([PR CI](https://github.com/drewweske/OROS-1/actions/runs/40298447657), [push CI](https://github.com/drewweske/OROS-1/actions/runs/40298439151)). No interactive GPU/Release/large-population benchmark | Not due |

**Latest user command:** #19  
**Next command:** #20  
**Next required five-run audit:** **#20**  
**Next official mainline canonization evaluation:** **#30** (after exact-SHA verification and deliberate review).

When a new "Build OROS" request arrives, read this ledger before choosing work, advance the count once when the run is recorded, and do not skip the review at multiples of five. If no code change was possible, still report that execution was blocked instead of inventing a completed run.
