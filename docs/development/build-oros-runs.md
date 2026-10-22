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
| 14 | 2026-10-22 | `ab2a8af` -> lazy memory staging | Avoid full tick memory staging and bootstrap repartition on nonvisible/distant/unavailable requests; verify memory storage stays unchanged over repeated ticks; CI pending | Not due |

**Latest user command:** #14  
**Next command:** #15 — mandatory whole-engine review  
**Next required five-run audit:** **#15**

When a new "Build OROS" request arrives, read this ledger before choosing work, advance the count once when the run is recorded, and do not skip the review at multiples of five. If no code change was possible, still report that execution was blocked instead of inventing a completed run.
