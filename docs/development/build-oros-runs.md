# Build OROS — Run Ledger

This file makes the ChatGPT-only development cadence verifiable across sessions.

**Counting:** One explicit user request containing "Build OROS" = one run. AI retries, CMake builds, commits and GitHub Actions runs do not increment the count.

**Review checkpoint:** every fifth run (#5, #10, #15, ...). Also run a requested out-of-cycle audit.

| Run | Date | Before -> after | Scope | Review |
| --- | --- | --- | --- | --- |
| 1 | 2026-10-21 | `7be2a33` -> `4dc8322` | Initialize actor-owned vision, fixed-tick adapter, persistence tests | Not due |
| 2 | 2026-10-21 | `4dc8322` -> `0a112bc` and follow-up audit commit | Entity-sorted multi-NPC vision registry, comprehensive baseline structural/code architecture review, whole-tree CI source audit | **Extra baseline completed** |
| 3 | 2026-10-22 | `87018bc` -> pending CI verification | Deterministic transactional multi-NPC fixed-tick visual perception batch, tests and CMake registration | Not due |

**Latest user command:** #3 (verification pending)  
**Next command:** #4 after run #3 verification  
**Next required five-run audit:** **#5**

When a new "Build OROS" request arrives, read this ledger before choosing work, advance the count once when the run is recorded, and do not skip the review at multiples of five. If no code change was possible, still report that execution was blocked instead of inventing a completed run.
