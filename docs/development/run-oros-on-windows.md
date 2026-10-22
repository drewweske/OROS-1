# Run OROS 1 on Windows — Playtest Guide (Build OROS #7)

OROS is currently a Windows-first C++23/Direct3D12 engine graybox. The following method downloads a **Windows executable compiled by your own GitHub Actions workflow**. It does **not** edit or overwrite your existing local, uncommitted OROS source files, and requires no PowerShell or Codex.

## Method 1 — Download and play the verified CI executable (recommended)

1. Open [OROS Windows CI Actions](https://github.com/drewweske/OROS-1/actions/workflows/oros-windows-ci.yml).
2. Click the newest **green check-mark** run for **`automation/oros-009-chat-build`**. You can inspect the commit SHA in the run header.
3. Scroll down to **Artifacts**. Click **`OROS-1-Windows-Debug`**. You may need to sign in to GitHub.
4. Extract the downloaded ZIP to a **new separate folder**, such as `Documents\OROS-Playtest`. Never extract over the existing OROS repository checkout.
5. Open the extracted folder in File Explorer and double-click **`Oros1.exe`**. Keep its console window open for diagnostics.
6. You should see a Direct3D12 window with a perspective ground grid and two colored NPC debug posts, not only a blue background.

If an unsigned-executable warning appears, verify the artifact is from a successful run of your own repository; do not run unknown third-party binaries. Windows graphics support for Direct3D12 is required.

## Controls

- **W / A / S / D:** move relative to the camera direction
- **Left / Right arrow keys:** rotate the camera view
- **Escape** or window close: quit

The camera follows the World-owned player from behind/above, rotating at a deterministic rate on fixed simulation ticks. This remains a wireframe development scene, **not** a textured terrain level or a free-look shooter camera.

## Acceptance checklist

- [ ] Native window opens and shows a perspective grid plus two NPC posts.
- [ ] W moves forward and visible grid shifts; A/S/D strafe/backpedal/strafe.
- [ ] Left/Right rotates the visible camera.
- [ ] After turning, W moves in the new forward direction.
- [ ] Console prints NPC perception transitions as you change distance/angle.
- [ ] Escape closes the application cleanly with no crash.

If any step fails, send ChatGPT a screenshot of the window and/or the last 20–40 console lines. GitHub CI confirms software compilation and hidden-window graphics tests, but **cannot confirm appearance on your personal GPU**.

## Method 2 — Build from a separate fresh copy (optional)

If you want the source and local debugger, use GitHub Desktop to **clone** `drewweske/OROS-1` into a **new folder**, switch to `automation/oros-009-chat-build`, and use Visual Studio 2022 with the "Desktop development with C++" workload. In Visual Studio, open the cloned repository folder containing `CMakeLists.txt`, select x64 Debug, use **Build > Build All**, choose `oros_bootstrap` / `Oros1.exe` as startup target, then **Ctrl+F5**. Your existing local checkout, with uncommitted changes, remains untouched.

## Reference details

- Build system: root `CMakeLists.txt`, MSVC 2022, C++23.
- CMake executable target: `oros_bootstrap`; binary filename: `Oros1.exe`.
- Windows CI binary output: `build/bin/Debug/Oros1.exe`.
- GitHub Actions workflow: `.github/workflows/oros-windows-ci.yml`.
- Automated test suite: CTest including D3D12 hidden-window renderer tests and pure camera/grid geometry tests.
