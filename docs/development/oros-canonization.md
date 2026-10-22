# OROS official mainline canonization cadence

**Authorized by repository owner:** October 22, 2026.

A `Build OROS` invocation advances the development ledger by **exactly one**. Every **fifth** run (#5, #10, #15, #20, …) requires a whole-repository architecture review. Separately, every **fifteenth** run (#15, #30, #45, …) requires explicit release-readiness assessment and a safe **canonization merge to `main`**, following the user's standing authorization. Reviews do not imply automatic merges if there is a blocker.

## Mandatory canonization gates

1. Read actual Git branch HEADs, `AGENTS.md`, Constitution, current review, run ledger and unmerged PRs; confirm the source branch descends from (or has a conflict-free reviewed merge with) the current `main`.
2. Compare all changed files, architecture invariants and relevant findings from the last five-run review. Reject data-loss, actor identity, fidelity, coordinate, stale memory and persistent schema regressions.
3. Pass full fresh Windows/MSVC C++23 CI with all registered CTest targets and the CMake source inventory on the **exact proposed branch SHA**. Inspect logs; new tests must not be skipped or removed to force green.
4. Open or update an ordinary PR to `main`, confirm GitHub reports `mergeable_state=clean` and expected head SHA, then use a **normal history-preserving merge commit**. Do not force, rewrite history, delete user branches or bypass protections.
5. Verify actual `main` HEAD and PR merge SHA/parents; record exact merge, test URLs, review links, remaining issues and next cadence in the ledger and PR. A post-merge main CI run is preferable when configured; do not falsely claim that pre-merge CI ran on the merge commit.
6. Continue next Build OROS on a new isolated branch from official `main`, with a draft development PR.

If conflicts, failing tests, missing authorizations for destructive actions or platform restrictions block safe merging, report the exact gate and preserve a handoff. The policy authorizes evaluated merges, not reckless automatic merges or background work.

## Canonization ledger

| Builds | Release gate | Source head | Official merge commit | Verified evidence |
| --- | --- | --- | --- | --- |
| #1–15 | **Completed 2026-10-22** | `9705e6e7af05b7d66ae00c0d642e458c3281cadc` | `31937ead3ab3e5781d26ebb5af3426857702d427` | [PR #8](https://github.com/drewweske/OROS-1/pull/8), [push Windows CI](https://github.com/drewweske/OROS-1/actions/runs/39759422965), [PR Windows CI](https://github.com/drewweske/OROS-1/actions/runs/39759429845), **125/125 CTest**, [Architecture Review #15](../architecture/reviews/2026-10-22-oros-whole-engine-review-15.md) |
| #16–30 | **Future evaluation at #30** | Not yet established | Not merged | Requires new exact-SHA CI, review #30 and safe PR to `main` |

The user's local checkout and personal GPU are separate from GitHub Actions. A GitHub merge does not sync the laptop automatically. The PR #8 merge included the existing proven Living World code and is not a complete RPG or persistence system.
