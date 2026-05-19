# Native Linux server protocol extensions

The native Linux server now records selected UltraVNC/RFB extension state instead
of silently ignoring those messages:

- `rfbSetScale` and PalmVNC scale factor messages update per-client scale state.
- `rfbEncodingLastRect` is tracked as a supported client preference.
- `rfbEncodingQualityLevel0..9` is tracked as the requested quality level.
- `rfbEncodingCompressLevel0..9` is tracked as the requested compression level.

The server still only emits encodings that are implemented in the portable update
encoder. Tracking these preferences is intentional groundwork for future encoder
selection; it is not a fake advertisement of unimplemented UltraVNC encodings.
