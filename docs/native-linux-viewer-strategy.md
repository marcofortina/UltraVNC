# Native Linux viewer strategy

The existing native UltraVNC viewer is a Windows application. A Linux port should
not start by compiling Win32 windows, dialogs or message handling behind a large
compatibility layer.

## Strategy

1. Keep the existing Windows viewer stable.
2. Reuse portable RFB protocol, decoding, encoding and stream code where possible.
3. Extract viewer-neutral connection/session logic only when it can be tested on
   Linux without depending on Win32 UI objects.
4. Add a Linux viewer frontend separately from the Windows UI. Candidate frontend
   layers include a headless/test frontend first, then a toolkit-specific UI.
5. Keep keyboard, pointer, clipboard and file-transfer mappings behind explicit
   platform boundaries.

## Initial milestones

- Build portable RFB/client transport pieces natively on Linux.
- Add headless viewer/session smoke tests that connect to a test endpoint.
- Add platform-neutral clipboard/input abstractions.
- Add a Linux UI only after the headless/session layer is stable.

## Non-goals for the first Linux viewer milestones

- Replacing the Windows viewer UI.
- Rewriting the whole viewer at once.
- Hiding Win32 dependencies behind broad fake compatibility headers.
