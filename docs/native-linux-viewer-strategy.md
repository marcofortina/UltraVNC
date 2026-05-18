# Native Linux viewer strategy

The existing native UltraVNC viewer is a Windows application. A Linux port should
not start by compiling Win32 windows, dialogs or message handling behind a large
compatibility layer.

## Strategy

1. Keep the existing Windows viewer stable.
2. Reuse portable RFB protocol, decoding, encoding and stream code where possible.
3. Extract viewer-neutral connection/session logic only when it can be tested on
   Linux without depending on Win32 UI objects.
4. Add a Linux viewer frontend separately from the Windows UI. The selected
   frontend toolkit for the native Linux viewer path is Qt.
5. Keep keyboard, pointer, clipboard and file-transfer mappings behind explicit
   platform boundaries.


## Current boundary

Repeater Linux and the WinVNC portable core are closed milestones, but native
Linux viewer support is still open, but the first Qt-based viewer shell is the
selected frontend direction. Future viewer work should reuse the closed portable
protocol/framebuffer/session pieces instead of introducing a broad Win32
compatibility layer.

## Initial milestones

- Build portable RFB/client transport pieces natively on Linux.
- Add headless viewer/session smoke tests that connect to a test endpoint.
- Add platform-neutral clipboard/input abstractions.
- Add the Qt viewer shell first, then wire tested viewer/session logic into it.

## Non-goals for the first Linux viewer milestones

- Replacing the Windows viewer UI.
- Rewriting the whole viewer at once.
- Hiding Win32 dependencies behind broad fake compatibility headers.
