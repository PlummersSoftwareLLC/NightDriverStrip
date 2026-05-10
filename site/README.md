# NightDriverStrip Web UI

The firmware now embeds a static `HTML/CSS/JS` administration UI from:

- [site/index.html](index.html)
- [site/styles.css](styles.css)
- [site/app.js](app.js)

There is no Node, React, Vite, or external frontend build dependency anymore.

## Build

The PlatformIO pre-build hook runs:

- [tools/bake_site.py](../tools/bake_site.py)

That script gzips the static assets into `site/dist/` for embedding in flash.

## Runtime contract

The web UI is intentionally thin. It talks directly to the device APIs exposed in:

- [src/webserver.cpp](../src/webserver.cpp)

This means frontend changes should prefer preserving the existing API shape unless there is a clear firmware-side reason to change it.
