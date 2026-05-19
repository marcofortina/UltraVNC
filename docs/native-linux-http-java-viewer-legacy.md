# Native Linux server HTTP Java viewer policy

The HTTP Java viewer is the legacy WinVNC path that serves Java applet resources
from the server over HTTP so a browser can start the bundled Java VNC viewer.
That model belongs to the historical Windows server runtime and is not a native
Linux production feature.

The native Linux server intentionally treats this path as legacy-only:

- it does not serve the bundled Java `.class` resources;
- it does not start an HTTP listener for the Java viewer;
- it rejects `--http-java-viewer` explicitly instead of silently doing nothing;
- it keeps the old resources untouched for the Windows server path;
- it avoids documenting the applet endpoint as a supported Linux deployment
  option.

Rationale:

- browser Java applets are obsolete deployment technology;
- the existing implementation is tied to the historical WinVNC HTTP-serving
  path and bundled applet resources;
- exposing a half-ported HTTP viewer would mislead operators into believing the
  Linux server has a reviewed web viewer surface;
- the native Linux server already has a safer direction: real VNC clients,
  VeNCrypt/TLS transport security, and a future separately reviewed web/static
  viewer endpoint if one is needed.

Recommended Linux direction:

- use native VNC viewers for normal operation;
- keep browser/web access as a future dedicated feature, not a carry-over Java
  applet port;
- design any future HTTP/web viewer endpoint as an explicit Linux feature with
  authentication, transport security, content security policy, packaging, and
  tests.
