# Architectural Audit — screen-capture-qt

_Audited: 2026-05-01_

---

## Critical

### 1. AppController is a God Class
**Files:** `src/appcontroller.hpp`, `src/appcontroller.cpp`

AppController (~335 lines) owns: both UI windows, worker thread lifecycle, strategy selection, state machine, settings persistence, aspect-ratio resize logic, and the macOS hotkey manager. Violates KISS and DRY. State transitions, worker management, and UI coordination should be separated.

---

### 2. Bidirectional regionChanged Coupling
**Files:** `src/appcontroller.cpp` (signal connections), `src/ui/controlbar.cpp` (mouseMoveEvent)

CaptureWindow emits `regionChanged` → AppController receives it → AppController re-emits `regionChanged` → CaptureWindow applies it. Additionally, `ControlBar::mouseMoveEvent` calls `m_captureWindow->setGeometry()` directly, bypassing AppController entirely. Two independent paths modify the same geometry.

---

### 3. ControlBar Does Too Much
**Files:** `src/ui/controlbar.cpp`

ControlBar is not just a view:
- `mouseMoveEvent` directly resizes CaptureWindow and handles grip zones (business logic in the view layer)
- Preferences dialog is built and managed inline — should be a separate window or delegated to AppController
- Demo mode toggle directly calls platform functions (`setWindowCaptureExcluded`) instead of signalling AppController
- Snap-to-region geometry calculation lives in the view

---

### 4. Strategy Use-After-Free Risk
**Files:** `src/appcontroller.cpp`

Frames are delivered to `m_strategy->onFrame()` via `Qt::QueuedConnection` from the worker thread. When `onRecordingFinished()` is called, `m_strategy->deleteLater()` is issued — but frames already queued in the event loop will execute on the strategy after it's been marked for deletion.

---

### 5. FrameStore TOCTOU — Unenforced Contract
**Files:** `src/capture/framestore.hpp`, `src/capture/framestore.cpp`

The comment states: "During recording the main thread must not call `frameAt()` — only `addFrame()` and `clear()` are safe." There are no assertions, no thread-ID checks, and no mutex between `clear()` and `frameAt()`. The contract is documentation-only.

---

### 6. BufferedStrategy Destructor Blocks
**Files:** `src/bufferedstrategy.cpp`

`~BufferedStrategy()` calls `m_encoderThread->quit()` and `m_encoderThread->wait()` synchronously. Blocking in a destructor is problematic if the destructor is invoked from a non-main-thread context or during teardown.

Also: the encoder thread is quit/waited a second time in the `encodingFinished`/`encodingFailed` signal handlers — double cleanup.

---

## Moderate

### 7. QGraphicsScene Not Actually Used as Compositor
**Files:** `src/ui/capturewindow.cpp`

The architecture doc states QGraphicsScene is the compositor for screen frames, camera PIP, and annotations. In practice, the scene only holds the border rectangle, dimension label, and resize handles. GIF and video encoding bypass the scene entirely and crop/scale raw QImages directly.

### 8. Unused AppState Values
**Files:** `src/appcontroller.hpp`

`AppState::Countdown` and `AppState::Preview` are defined and referenced in `RecordingSettings` and comments but are never entered. No state transition leads to them.

### 9. Aspect Ratio Lock Logic Duplicated
**Files:** `src/ui/capturewindow.cpp`, `src/ui/controlbar.cpp`, `src/appcontroller.cpp` (`applyResizeDelta`)

The aspect-ratio constraint calculation (derive new W/H from delta while holding ratio) appears in three places with slight variations. A bug fix in one place won't propagate to the others.

### 10. State Machine Validation is Scattered
**Files:** `src/appcontroller.cpp`

Each slot handler independently guards with `if (m_state != AppState::Idle) return;`. There is no centralized state transition table; invalid transitions can be introduced by accident.

### 11. Settings Not Validated on Load
**Files:** `src/appcontroller.hpp` (`RecordingSettings::load`)

The saved `outputDir` is restored from QSettings without checking whether the directory still exists or is writable. Recording will fail with a cryptic error if the path has been deleted or moved.

### 12. Backend Threading Pattern is Fragile
**Files:** `src/capture/screencaptureworker.cpp`

The backend is partially constructed on the main thread (for `setExcludedWindowIds` NSView access) and partially on the worker thread. No compile-time guarantee that future backends handle this split correctly.

---

## Minor

| # | File(s) | Issue |
|---|---------|-------|
| 13 | `src/ui/capturewindow.cpp` | `m_suppressSignal` flag set/cleared around `setGeometry()` — should be an RAII guard |
| 14 | Multiple | Magic numbers (`kBarHeight`, `kGripSize`, `kBorderWidth`, etc.) defined per-file rather than in a shared constants header |
| 15 | `src/appcontroller.cpp` | If screen recording permission is denied on first launch, accessibility permission is never requested until the next launch |
| 16 | `src/ui/controlbar.cpp` | Demo mode directly calls `setWindowCaptureExcluded()` — should signal through AppController |
| 17 | `src/appcontroller.cpp` | `onRegionChanged()` does not clamp the rect to screen bounds; dragging off-screen may cause silent capture failure |
| 18 | `src/ui/capturewindow.hpp` | `lockedAspect()` is public but only used internally by ControlBar — encapsulation leak |
| 19 | `src/platform/qtscreenbackend.cpp` | Cursor compositing is `#if 0`'d out — dead code should be removed or tracked as a TODO |
| 20 | `src/bufferedstrategy.cpp` | Encoder thread quit/waited twice (destructor + signal handler) |
| 21 | `src/encoding/videoencoder.cpp` | Audio/video sync is approximate (system audio clock vs. QElapsedTimer) — documented known limitation |
| 22 | `src/capture/framestore.hpp` | No frame buffer size limit; long recordings can OOM. Comment notes ~1.2 GB for 30s at 30fps |

---

## Summary

| Severity | Count |
|----------|-------|
| Critical | 6 |
| Moderate | 6 |
| Minor | 10 |

## Recommended Priority Order

1. **Fix strategy race condition** (#4) — active crash risk
2. **Enforce FrameStore contract** (#5) — add assertions or a concurrent queue
3. **Split AppController** (#1) — extract `AppStateMachine` and delegate worker lifecycle
4. **Refactor ControlBar** (#3) — move geometry logic to AppController; move prefs to a dedicated window
5. **Centralize aspect ratio logic** (#9) — single utility function
6. **Validate settings on load** (#11) — fallback to Movies dir if saved path is invalid
7. **Implement QGraphicsScene compositor** (#7) — required before camera PIP or annotations can be added
