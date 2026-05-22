# NightDriverStrip Web UI Specification

This document is the regeneration spec for the current on-device web UI. It is intended to be detailed enough to recreate the UI from scratch without treating the current `site/` files as the only source of truth.

The current implementation is a static, dependency-free browser application served by the ESP32 firmware. It lives in:

- `site/index.html`
- `site/styles.css`
- `site/app.js`
- `tools/bake_site.py`
- `src/webserver.cpp`

## Goals

- Provide a compact control surface for a NightDriverStrip device.
- Run directly from the firmware without Node, React, Vite, CDNs, web fonts, or external assets.
- Prefer firmware-driven metadata for settings so the UI does not hardcode every setting.
- Keep the app usable on desktop and mobile browsers.
- Support regeneration at any time by preserving the contracts described here.

## Non-Goals

- Do not create a marketing page, setup wizard, or multi-page site.
- Do not depend on internet access or third-party frontend packages.
- Do not require a frontend build step beyond gzip embedding.
- Do not store device configuration in the browser except for UI preferences.

## Runtime And Build Model

The UI consists of three static assets:

- `/index.html`
- `/styles.css`
- `/app.js`

`tools/bake_site.py` gzips these into:

- `site/dist/index.html.gz`
- `site/dist/styles.css.gz`
- `site/dist/app.js.gz`

PlatformIO embeds these files through `board_build.embed_files`. The firmware serves them from `src/webserver.cpp` when `ENABLE_WEB_UI` is enabled:

- `/`
- `/index.html`
- `/styles.css`
- `/app.js`

The firmware also embeds `config/timezones.json` and serves it at `/timezones.json`.

The generated UI must be plain HTML, CSS, and JavaScript. It should run as a single immediately invoked script with `"use strict"`. It may use modern browser APIs already used by the current UI: `fetch`, `Promise.allSettled`, `URLSearchParams`, `localStorage`, `dialog`, `WebSocket`, `requestAnimationFrame`, `Intl.DisplayNames`, and `CanvasRenderingContext2D`.

## Information Architecture

The application has one screen with these regions, in order:

1. Fixed visual background layers.
2. Main shell.
3. Top status bar.
4. Toolbar.
5. Summary card grid.
6. Tabbed main panel.
7. Effect settings modal dialog.
8. Toast stack.

There are three tabs:

- Effects
- Settings
- Statistics

Only one tab pane is visible at a time. The selected tab persists in `localStorage` under `nd.activeTab`.

## Static DOM Structure

The static HTML must include these elements and IDs because the controller binds by ID.

### Background

- `div.backdrop`
- `div.grid-overlay`

Both are fixed, non-interactive, and cover the viewport.

### Main Shell

`main.shell` wraps all primary content.

### Topbar

`header.topbar.panel.card-blue` contains:

- `div.title-block`
  - `p.eyebrow`: text `ESP32 Control Surface`
  - `h1`: text `NightDriverStrip`
- `div.status-cluster`
  - `div#connectionStatus.status-pill`
    - `span.status-dot`
    - `span#connectionStatusText`: initial text `Starting`
  - host pill:
    - label `Host`
    - `strong#hostValue`: initial `--`
  - port pill:
    - label `Web Port`
    - `strong#webPortValue`: initial `--`

The controller sets host and port from `window.location`.

### Toolbar

`section.toolbar.panel.card-orange` contains four groups:

- Effects group:
  - `button#prevEffectButton`: previous effect
  - `button#nextEffectButton`: next effect
  - `button#refreshEffectsButton`: reload effects
- Interval group:
  - `input#effectIntervalInput[type=number][min=0][step=1]`
  - `button#saveIntervalButton`: apply interval
- Statistics group:
  - `input#statsRefreshInput[type=number][min=1][max=60][step=1]`
  - `input#autoRefreshToggle[type=checkbox]`
- Device group:
  - `button#rebootButton`
  - `button#resetDeviceConfigButton`
  - `button#resetEffectsConfigButton`

The device group is pushed to the right on wide layouts.

### Summary Cards

`section.summary-grid` contains six `article.summary-card.panel` cards:

1. Current Effect, `card-red`
   - `#summaryCurrentEffect`
   - `#summaryEffectStatus`
2. Interval, `card-yellow`
   - `#summaryInterval`
   - `#summaryIntervalRemaining`
3. Topology, `card-green`
   - `#summaryTopology`
   - `#summaryDriver`
4. LED FPS, `card-cyan`
   - `#summaryLedFps`
   - `#summaryAudioFps`
5. CPU Used, `card-purple`
   - `#summaryCpu`
   - `#summaryCpuCores`
6. Heap Free, `card-pink`
   - `#summaryHeap`
   - `#summaryPsram`

### Tabbed Main Panel

`section.main-grid.tabs-grid` contains `div.tab-shell`.

The tab strip is:

- `div.tab-strip[role=tablist][aria-label="Main sections"]`
  - `button#tabEffectsButton.tab-button.tab-effects`
  - `button#tabSettingsButton.tab-button.tab-settings`
  - `button#tabStatisticsButton.tab-button.tab-statistics`

Each tab button has `role=tab`, `aria-selected`, and `aria-controls`.

The tab body is:

- `article#tabBodyPanel.panel.tab-body-panel`

Its color theme changes by active tab:

- Effects: `card-green`
- Settings: `card-blue`
- Statistics: `card-purple`

Each tab pane has `role=tabpanel`. Hidden panes must have the `hidden` attribute and lack `.active`.

#### Effects Pane

`section#tabEffectsPane.tab-pane.active` contains:

- panel header:
  - eyebrow `Runtime Effects`
  - heading `Effects`
  - `#effectsMeta`
- `div.table-wrap`
  - `table.canvas-table`
    - columns: grip, On, Name, Status, Core, Actions
    - `tbody#effectsTableBody`

Initial tbody should show one full-width loading row.

#### Settings Pane

`section#tabSettingsPane.tab-pane[hidden]` contains:

- panel header:
  - eyebrow `Device`
  - heading `Settings`
  - `button#reloadSettingsButton`
  - `button#applySettingsButton`
  - `button#applySettingsRebootButton`
- `form#deviceSettingsForm.settings-grid`

The form is rendered dynamically from `/settings/specs` plus `/api/v1/settings/schema`.

#### Statistics Pane

`section#tabStatisticsPane.tab-pane[hidden]` contains:

- panel header:
  - eyebrow `Runtime`
  - heading `Statistics`
  - `#statsTimestamp`
- `div#statsGrid.stats-grid`
- preview header:
  - eyebrow `Websocket`
  - heading `Frame Preview`
  - `button#previewConnectButton`
  - `button#previewDisconnectButton`
- `div.preview-meta`
  - `span#previewStatus`
- `div#previewWrap.preview-wrap.is-empty`
  - `canvas#previewCanvas[width=640][height=320]`

The preview wrapper is hidden while empty.

### Effect Settings Dialog

Use a native `dialog#effectSettingsDialog.editor-dialog`.

Inside it:

- `form.dialog-shell.panel.card-red[method=dialog]`
  - `header.dialog-header`
    - eyebrow `Effect`
    - `h2#effectDialogTitle`
    - `button#closeEffectDialogButton`
  - `div#effectSettingsForm.settings-grid`
  - `footer.dialog-actions`
    - `button#cancelEffectDialogButton`
    - `button#applyEffectDialogButton`

The dialog is opened with `showModal()` and closed with `close()`.

### Toasts

Use `div#toastStack.toast-stack[aria-live=polite]`. Toasts are transient child `div.toast` nodes with optional `.success` or `.error`.

## Visual Design

The UI is a dark, neon, monospace control surface.

### Typography

- Body font stack: `"SFMono-Regular", "Menlo", "Consolas", monospace`.
- Buttons, inputs, selects, and textareas inherit the body font.
- Main title is uppercase, wide tracked, and responsive.
- Eyebrows, summary labels, summary notes, panel metadata, toolbar labels, field labels, toggle labels, and inline suffixes are small uppercase text with wide letter spacing.

### Core CSS Variables

The root palette must include:

- `--bg: #030712`
- `--bg-soft: #081122`
- `--panel: rgba(7, 14, 29, 0.84)`
- `--panel-strong: rgba(10, 18, 38, 0.94)`
- `--line: rgba(74, 222, 255, 0.24)`
- `--line-strong: rgba(74, 222, 255, 0.54)`
- `--ink: #eff8ff`
- `--muted: #8ea7c8`
- `--accent: #4adeff`
- `--accent-warm: #ffcf40`
- `--accent-hot: #ff5b88`
- `--accent-good: #39ff88`
- `--danger: #ff5b88`
- `--warning: #ffcf40`
- `--good: #39ff88`
- `--shadow: rgba(0, 0, 0, 0.35)`

### Panel Themes

Panels use a custom `--card-rgb` variable to create borders, highlights, radial glow, and shadows.

Theme classes:

- `.card-red`: `255, 40, 40`
- `.card-yellow`: `255, 214, 10`
- `.card-green`: `57, 255, 136`
- `.card-cyan`: `0, 224, 255`
- `.card-blue`: `64, 140, 255`
- `.card-purple`: `191, 90, 242`
- `.card-orange`: `255, 149, 0`
- `.card-pink`: `255, 62, 131`

`.panel` has:

- `position: relative`
- `overflow: hidden`
- 1px themed border
- 18px border radius
- layered radial and linear backgrounds
- inner highlight, drop shadow, outer glow
- `backdrop-filter: blur(10px)`
- a pseudo-element border sheen

### Background

The page background is dark. `.backdrop` uses layered radial gradients and a dark linear gradient. `.grid-overlay` adds a 48px grid, low opacity, and a vertical fade mask.

### Layout Dimensions

- `.shell`: `width: min(1680px, calc(100vw - 24px))`, centered, top/bottom padding.
- Major regions have 16px bottom margin.
- Summary grid is six equal columns on wide screens.
- Statistics grid is two equal columns on wide screens.
- Settings grid is one column of section containers.

Responsive breakpoints:

- At max-width 1220px:
  - summary grid becomes three columns.
  - any old `.main-grid.nds-main-grid` layout collapses to one column if present.
- At max-width 760px:
  - shell uses `calc(100vw - 16px)`.
  - topbar and toolbar padding shrink.
  - summary, stats, and settings grids become one column.
  - setting rows become one column.
  - main title becomes about 1.8rem.

### Controls

Buttons:

- `.action-button` and `.icon-button` have a translucent border, dark gradient background, 12px radius, 10px x 14px padding, pointer cursor, and hover lift.
- `.action-button.accent` uses green border emphasis.
- `.action-button.warning` uses hot pink border emphasis.
- Disabled buttons reduce opacity and remove hover effects.

Inputs/selects/textareas:

- Minimum height 44px.
- 12px radius.
- 1px cyan translucent border.
- dark translucent background.
- `padding: 10px 12px`.
- Checkboxes keep intrinsic dimensions.
- Range inputs have no padding.

Boolean switches use a custom mac-style switch:

- `label.mac-switch`
- hidden checkbox
- `.mac-switch-track` 52px by 32px rounded track
- `.mac-switch-thumb` 24px circular thumb
- checked state is green, thumb translates 20px.

### Effects Table

Use `table.canvas-table`, full width, collapsed borders. Cells have compact vertical padding and cyan separator lines. Headers are uppercase muted text. Drag state:

- `tr.dragging`: opacity 0.4
- `tr.drop-target`: inset top line highlight
- `.drag-grip` is a 24px square grab handle with a list/hamburger glyph via CSS content.

Action cells use `.row-actions`, `.mini-button`, and `.mini-icon-button`. Current action labels are icon-like glyphs:

- Trigger effect: U+25B6 play triangle
- Effect settings: U+2699 gear
- Delete effect: U+1F5D1 wastebasket

A regenerating implementation may use text or SVG/icon equivalents, but the button titles and behavior must remain.

### Settings Form

Settings are grouped into `.settings-section` blocks:

- section border and dark translucent background.
- header with title and optional description.
- rows use `.setting-row`.
- normal row grid: label/meta column and control column.
- palette rows use `.setting-row.stacked`.
- control width is capped at 360px on wide screens.
- help text is muted and supports HTML from firmware descriptions.
- validation errors use `.field-error`.

### Stats Cards

`.stat-card` is a bordered dark card. It has an `h3`, optional `.meter`, and a `.stat-list` of `.stat-row` pairs. Meter fills are linear cyan-to-green.

### Preview Canvas

The preview canvas:

- has pixelated rendering.
- has dark background, cyan border, 14px radius.
- toggles `.preview-canvas-thin` when rendered height is <= 12px to remove border/background for one-row strips.

### Toasts

Toast stack is fixed bottom-right. Toasts have dark background, 12px radius, cyan border. `.toast.error` uses danger border; `.toast.success` uses good border. Toasts auto-remove after 4200ms.

## Client State

The client keeps all mutable UI state in one object equivalent to:

- `staticStats`: result of `/statistics/static`
- `dynamicStats`: result of `/statistics/dynamic`
- `settings`: result of `/settings`
- `settingsSpecs`: result of `/settings/specs`
- `timezones`: parsed external timezone document, initially null
- `timezonesLoading`: guard flag
- `unifiedSettings`: result of `/api/v1/settings`
- `unifiedSchema`: result of `/api/v1/settings/schema`
- `effects`: result of `/effects`
- `effectIntervalInputDirty`: protects user edits in toolbar interval input
- `effectIntervalPendingMs`: interval value posted but not yet reflected by server, or null
- `deviceDraft`: unsaved device settings keyed by setting spec name
- `deviceErrors`: `Map<settingName, message>`
- `effectDialog`: object with `open`, `effectIndex`, `effectName`, `specs`, `values`, `draft`, and `errors`
- `autoRefresh`: boolean
- `statsRefreshSeconds`: loaded from `localStorage["nd.statsRefreshSeconds"]`, default 3
- `timers`: `stats`, `effects`, `countdown`
- `preview`: websocket/render-loop state
- `drag`: effect drag state
- `activeTab`: loaded from `localStorage["nd.activeTab"]`, default `effects`

Persist these browser-only preferences:

- `nd.activeTab`
- `nd.statsRefreshSeconds`

Do not persist drafts in localStorage.

## Initialization Flow

On `DOMContentLoaded`:

1. Bind all required DOM elements by ID.
2. Attach event handlers.
3. Initialize static shell:
   - Host and web port from `window.location`.
   - Stats refresh input from `state.statsRefreshSeconds`.
   - Auto refresh checkbox from `state.autoRefresh`.
   - Restore active tab.
   - Set connection state to `Starting`.
4. `loadAll()`.
5. Start timezone loading if needed.
6. Start polling.

`loadAll()` fetches these endpoints concurrently with `Promise.allSettled`:

- `/statistics/static`
- `/statistics/dynamic`
- `/settings`
- `/settings/specs`
- `/api/v1/settings`
- `/api/v1/settings/schema`
- `/effects`

Any successful response is accepted. Failures are collected and shown as a toast. After load:

- clear `deviceDraft`
- clear `deviceErrors`
- render settings, effects, summaries, stats, and preview independently so one renderer failure does not block the others
- set connection status to `Connected` if any request succeeded, otherwise `Offline`

## Polling

`restartPolling()` clears all timers, then:

- If auto-refresh is on:
  - fetch `/statistics/dynamic` every `state.statsRefreshSeconds` seconds
  - fetch `/effects` every 3000ms
- Always update the countdown/summary display every 250ms.

Changing `#statsRefreshInput` clamps to 1..60 seconds, writes `nd.statsRefreshSeconds`, and restarts polling.

Changing `#autoRefreshToggle` updates `state.autoRefresh` and restarts polling.

## HTTP Helpers

All fetch failures throw an Error built from HTTP status, JSON `message`, or response text.

`fetchJson(path, options)`:

- performs `fetch`
- requires `response.ok`
- reads text, trims trailing NUL bytes and whitespace, parses JSON

`postJson(path, payload)`:

- method POST
- header `Content-Type: application/json`
- body `JSON.stringify(payload || {})`
- returns parsed JSON when response content type contains `application/json` or `text/json`, otherwise null

`postForm(path, payload)`:

- method POST
- header `Content-Type: application/x-www-form-urlencoded;charset=UTF-8`
- serializes primitives with `String(value)`
- serializes arrays/objects with `JSON.stringify(value)`
- skips null/undefined values
- returns parsed JSON when response content type is JSON, otherwise null

## API Contract

### Static Assets

- `GET /`
- `GET /index.html`
- `GET /styles.css`
- `GET /app.js`
- `GET /timezones.json`

The first three app assets may be gzip encoded by firmware. The timezone document is JSON/text JSON.

### Statistics

`GET /statistics/static` returns static/config fields used by the UI:

- `MATRIX_WIDTH`
- `MATRIX_HEIGHT`
- `CONFIGURED_MATRIX_WIDTH`
- `CONFIGURED_MATRIX_HEIGHT`
- `CONFIGURED_NUM_LEDS`
- `ACTIVE_MATRIX_WIDTH`
- `ACTIVE_MATRIX_HEIGHT`
- `ACTIVE_NUM_LEDS`
- `COMPILED_NUM_LEDS`
- `COMPILED_NUM_CHANNELS`
- `ACTIVE_NUM_CHANNELS`
- `COMPILED_OUTPUT_DRIVER`
- `ACTIVE_OUTPUT_DRIVER`
- `COMPILED_WS281X_COLOR_ORDER`
- `CONFIGURED_WS281X_COLOR_ORDER`
- `COMPILED_AUDIO_INPUT_PIN`
- `CONFIGURED_AUDIO_INPUT_PIN`
- `AUDIO_INPUT_MODE`
- `FRAMES_SOCKET`
- `EFFECTS_SOCKET`
- `CHIP_MODEL`
- `CHIP_CORES`
- `CHIP_SPEED`
- `PROG_SIZE`
- `CODE_SIZE`
- `FLASH_SIZE`
- `HEAP_SIZE`
- `DMA_SIZE`
- `PSRAM_SIZE`
- `CODE_FREE`

`GET /statistics/dynamic` returns:

- `LED_FPS`
- `SERIAL_FPS`
- `AUDIO_FPS`
- `HEAP_FREE`
- `HEAP_MIN`
- `DMA_FREE`
- `DMA_MIN`
- `PSRAM_FREE`
- `PSRAM_MIN`
- `CPU_USED`
- `CPU_USED_CORE0`
- `CPU_USED_CORE1`

`GET /statistics` and `GET /getStatistics` return both static and dynamic fields.

### Effects

`GET /effects` and `GET /getEffectList` return:

```json
{
  "currentEffect": 0,
  "millisecondsRemaining": 12345,
  "eternalInterval": false,
  "effectInterval": 60000,
  "Effects": [
    {
      "name": "Effect name",
      "enabled": true,
      "core": false
    }
  ]
}
```

Effect actions are form POSTs:

- `POST /nextEffect`
- `POST /previousEffect`
- `POST /currentEffect` with `currentEffectIndex`
- `POST /setCurrentEffectIndex` with `currentEffectIndex`
- `POST /enableEffect` with `effectIndex`
- `POST /disableEffect` with `effectIndex`
- `POST /moveEffect` with `effectIndex`, `newIndex`
- `POST /copyEffect` with `effectIndex`
- `POST /deleteEffect` with `effectIndex`

The current UI uses all except copy.

### Device Settings

`GET /settings` returns a flat, legacy settings object. It includes device config fields such as:

- `hostname`
- `location`
- `locationIsZip`
- `countryCode`
- `timeZone`
- `use24HourClock`
- `useCelsius`
- `ntpServer`
- `rememberCurrentEffect`
- `powerLimit`
- `brightness`
- `globalColor`
- `applyGlobalColors`
- `secondColor`
- `audioInputPin`
- `matrixWidth`
- `matrixHeight`
- `matrixSerpentine`
- `outputDriver`
- `ws281xChannelCount`
- `ws281xColorOrder`
- `ws281xPins`
- `effectInterval`

Sensitive fields such as the weather API key are omitted from this read response.

`POST /settings` accepts form-encoded legacy fields by name. The current UI only uses this for settings without an `apiPath` and for the toolbar effect interval shortcut.

`GET /settings/specs` returns an array of setting spec objects. Each spec has:

- `name`: stable setting key used in draft maps and legacy posts
- `friendlyName`: label
- `description`: HTML-capable help text
- `type`: numeric setting type
- `typeName`: textual type
- `hasValidation`: optional boolean
- `minimumValue`: optional number
- `maximumValue`: optional number
- `emptyAllowed`: optional boolean
- `readOnly`: optional boolean
- `writeOnly`: optional boolean
- `section`: optional section ID
- `priority`: optional number; lower sorts first
- `requiresReboot`: optional boolean
- `apiPath`: optional dotted path into `/api/v1/settings`
- `widget`: optional widget metadata

Setting type numbers:

- `0`: Integer
- `1`: PositiveBigInteger
- `2`: Float
- `3`: Boolean
- `4`: String
- `5`: Palette
- `6`: Color
- `7`: Slider

Widget kinds:

- omitted or `default`
- `intervalToggle`
- `select`
- `slider`
- `color`

Select option sources:

- `inline`: use `widget.options.values` and optional parallel `labels`
- `schemaPath`: read an array from `/api/v1/settings/schema` at `widget.options.schemaPath`
- `intlCountryCodes`: generate ISO country options in-browser via `Intl.DisplayNames`
- `externalTimeZones`: fetch `widget.options.url` and normalize timezone identifiers

`GET /api/v1/settings` returns structured settings:

```json
{
  "device": {
    "hostname": "name",
    "location": "",
    "locationIsZip": false,
    "countryCode": "US",
    "timeZone": "America/Los_Angeles",
    "use24HourClock": false,
    "useCelsius": false,
    "ntpServer": "",
    "rememberCurrentEffect": true,
    "powerLimit": 0,
    "brightness": 255,
    "globalColor": 16711680,
    "secondColor": 16711680,
    "applyGlobalColors": false,
    "remote": {
      "enabled": false,
      "pin": -1
    },
    "audio": {
      "enabled": true,
      "audioInputPin": 0,
      "compiledDefaultPin": 0,
      "mode": "mode",
      "liveApply": false,
      "requiresReboot": true,
      "supportsPinOverride": true
    }
  },
  "topology": {
    "width": 32,
    "height": 8,
    "serpentine": true,
    "ledCount": 256,
    "liveApply": true
  },
  "outputs": {
    "driver": "ws281x",
    "compiledDriver": "ws281x",
    "liveApply": true,
    "ws281x": {
      "channelCount": 1,
      "compiledMaxChannels": 1,
      "colorOrder": "GRB",
      "compiledColorOrder": "GRB",
      "pins": [5]
    }
  },
  "effects": {
    "effectInterval": 60000
  }
}
```

`POST /api/v1/settings` accepts a partial structured JSON document. The UI sends only changed fields whose specs have `apiPath`. Supported paths include:

- `device.hostname`
- `device.powerLimit`
- `device.location`
- `device.locationIsZip`
- `device.countryCode`
- `device.timeZone`
- `device.use24HourClock`
- `device.useCelsius`
- `device.ntpServer`
- `device.openWeatherApiKey`
- `device.audioInputPin`
- `device.brightness`
- `device.globalColor`
- `device.secondColor`
- `device.applyGlobalColors`
- `device.clearGlobalColor`
- `device.rememberCurrentEffect`
- `topology.width`
- `topology.height`
- `topology.serpentine`
- `outputs.driver`
- `outputs.ws281x.channelCount`
- `outputs.ws281x.colorOrder`
- `outputs.ws281x.pins[N]`
- `effects.effectInterval`

When posting array element paths like `outputs.ws281x.pins[2]`, the UI must seed the outgoing array with the current full array from `state.unifiedSettings` before changing the indexed element. This prevents sparse arrays from clearing other pins.

`GET /api/v1/settings/schema` returns schema/support metadata:

```json
{
  "topology": {
    "compiledMaxWidth": 256,
    "compiledMaxHeight": 256,
    "compiledNominalWidth": 32,
    "compiledNominalHeight": 8,
    "compiledMaxLEDs": 256,
    "liveApply": true,
    "rejectMessage": "recompile needed"
  },
  "outputs": {
    "compiledDriver": "ws281x",
    "liveApply": true,
    "rejectMessage": "recompile needed",
    "allowedDrivers": ["ws281x"],
    "ws281x": {
      "compiledMaxChannels": 1,
      "compiledMaxLEDs": 256,
      "compiledColorOrder": "GRB",
      "allowedChannelCounts": [1],
      "allowedColorOrders": ["RGB", "RBG", "GRB", "GBR", "BRG", "BGR"],
      "compiledPins": [5]
    }
  },
  "device": {
    "remote": {
      "enabled": false,
      "pin": -1
    },
    "audio": {
      "enabled": true,
      "compiledDefaultPin": 0,
      "mode": "mode",
      "liveApply": false,
      "requiresReboot": true,
      "supportsPinOverride": true,
      "rejectMessage": "recompile needed"
    }
  },
  "sections": [
    {
      "id": "system",
      "title": "System",
      "description": "..."
    }
  ]
}
```

The UI must use `sections` to group settings. Unknown sections may be skipped if empty. Specs with no section fall back to `system`.

### Effect Settings

`GET /settings/effect/specs?effectIndex=N` returns setting specs for one effect.

`GET /settings/effect?effectIndex=N` returns current effect setting values. Base effect settings include:

- `_friendlyName`
- `_maximumEffectTime`
- `hasMaximumEffectTime`

Individual effect subclasses may add more fields.

`POST /settings/effect` accepts form fields:

- required `effectIndex`
- changed effect setting fields by spec name

### Reset

`POST /reset` accepts form fields:

- `board=1`: reboot board
- `deviceConfig=1`: reset persisted device config
- `effectsConfig=1`: reset persisted effects config

The current UI sends:

- Reboot: `{ board: 1 }`
- Reset Device Config: `{ board: 1, deviceConfig: 1 }`
- Reset Effect Config: `{ board: 1, effectsConfig: 1 }`

### Frame Preview WebSocket

Connect to `/ws/frames` using `ws://` or `wss://` matching the current page protocol.

The socket sends binary frames. Interpret each message as a `Uint8Array` of packed RGB bytes:

- byte 0: red for pixel 0
- byte 1: green for pixel 0
- byte 2: blue for pixel 0
- then repeat

Pixels are row-major with dimensions from:

- `ACTIVE_MATRIX_WIDTH` or `CONFIGURED_MATRIX_WIDTH`
- `ACTIVE_MATRIX_HEIGHT` or `CONFIGURED_MATRIX_HEIGHT`

The UI throttles canvas rendering to 30 FPS. If more frames arrive, keep the latest and render it on the next eligible animation frame.

If the socket closes while `shouldReconnect` is true, reconnect after 1000ms.

If `FRAMES_SOCKET` is false in static stats, clicking Connect should show an error toast instead of opening the socket.

## Rendering Rules

### Connection Status

`setConnectionState(text, stateClass)`:

- writes `#connectionStatusText`
- removes `.online` and `.offline`
- adds `.online` or `.offline` only for those states

Visual states:

- pending/no class: yellow dot
- `.online`: green dot
- `.offline`: pink/red dot

### Summary Cards

Render summaries only when `effects`, `staticStats`, and `dynamicStats` are all available.

Current effect:

- name from `effects.Effects[effects.currentEffect].name`
- note is `Enabled`, `Disabled`, or `No effect`

Interval:

- pinned when `effects.eternalInterval` is true or `effectInterval` is 0
- use `effectInterval` spec's `intervalToggle` widget metadata when available
- displayed interval is raw milliseconds divided by `unitDivisor`
- remaining time is `ceil(millisecondsRemaining / unitDivisor)`

Topology:

- value: `ACTIVE_MATRIX_WIDTHxACTIVE_MATRIX_HEIGHT / ACTIVE_NUM_LEDS leds`
- note: `ACTIVE_OUTPUT_DRIVER / ACTIVE_NUM_CHANNELS ch`

FPS:

- value: integer `LED_FPS`
- note: `Audio <AUDIO_FPS> / Serial <SERIAL_FPS>`

CPU:

- value: `CPU_USED` with one decimal and `%`
- note: `C0 <CPU_USED_CORE0>% / C1 <CPU_USED_CORE1>%`

Memory:

- value: formatted `HEAP_FREE`
- note: `PSRAM <PSRAM_FREE>`

Formatting:

- `formatNumber`: fixed 0 decimals
- `formatPercent`: fixed 1 decimal
- `formatBytes`: bytes, KB, or MB with one decimal above thresholds

### Toolbar Effect Interval

The toolbar interval input is in seconds. On apply:

1. Clamp seconds to 0..2147483.
2. Convert to milliseconds.
3. Set `effectIntervalPendingMs`.
4. POST `/settings` with `{ effectInterval: intervalMs }`.
5. On success, show toast and reload effects and settings.
6. On failure, clear pending state, mark input dirty, show error.

Periodic refresh must not overwrite a user edit while the input is focused or dirty. If a pending value is later reflected by the server, clear dirty/pending state.

### Effects Table

If effects are unavailable, show `No effects loaded.`

For each effect:

- Add `tr[data-effect-index]`.
- First cell: drag grip.
- Second cell: checkbox bound to enabled state.
- Third cell: escaped effect name in `.effect-name`.
- Fourth cell:
  - current effect: text `Active`, class `.effect-active`
  - enabled non-current: text `Queued`, class `.effect-disabled`
  - disabled: text `Disabled`, class `.effect-disabled`
- Fifth cell: `Core` when `effect.core`, otherwise `User`.
- Sixth cell: row actions.

Toggling enabled:

- If currently enabled, POST `/disableEffect`.
- If disabled, POST `/enableEffect`.
- Payload: `{ effectIndex }`.
- Reload effects on success.
- Revert checkbox and show error on failure.

Trigger:

- POST `/currentEffect` with `{ currentEffectIndex: index }`.
- Disabled when effect is not enabled.

Settings:

- Open effect dialog.

Delete:

- Confirm with `window.confirm("Delete this effect?")`.
- POST `/deleteEffect` with `{ effectIndex }`.
- Disabled for core effects.

Drag reorder:

- Drag starts from grip.
- Store source index in state.
- Drag over a row sets drop target styling.
- Drop calls `POST /moveEffect` with `{ effectIndex: fromIndex, newIndex: targetIndex }`.
- Clear all drag styles after drag end/drop.

### Settings Form

Settings render from specs and current values.

Current value resolution:

1. If spec has `apiPath`, read that path from `state.unifiedSettings`.
2. Otherwise read `state.settings[spec.name]`.
3. Otherwise use `defaultValueForSpec`.

Spec ordering:

- Filter out specs where `writeOnly && !hasValidation`.
- Sort by `priority` ascending, default priority 100.
- Tie break by `friendlyName || name`.

Grouping:

- Use `state.unifiedSchema.sections` as ordered section catalog.
- Each section object has `id`, `title`, `description`.
- Assign specs by `spec.section || "system"`.
- Do not render sections with no specs.

Each setting row:

- `.setting-row`; add `.stacked` for palette settings.
- Meta column:
  - `.setting-name`: `friendlyName || name`
  - `.field-help`: `description || ""`
- Value column:
  - `.setting-value`
  - one widget rendered from `spec.type` and `spec.widget.kind`

Draft behavior:

- Draft map key is always `spec.name`.
- A changed value goes into `deviceDraft` or `effectDialog.draft`.
- If the new value JSON-equals the original value, remove it from the draft.
- Values are compared with `JSON.stringify`.

Validation:

- Strings: if `emptyAllowed` is false and raw value is empty, error.
- Numeric types: value is required, must parse to a number, and must respect `minimumValue` and `maximumValue`.
- Error text goes into `.field-help` and adds `.field-error`.
- Clearing error restores original description HTML.

Topology cross-field validation:

- Determine max LEDs from `unifiedSchema.topology.compiledMaxLEDs` or `staticStats.COMPILED_NUM_LEDS`.
- Read drafted or current `matrixWidth` and `matrixHeight`.
- If width * height exceeds max LEDs, add same error to both fields unless they already have errors.

Apply flow:

1. Collect validation errors.
2. If errors exist, re-render form and show error toast.
3. Split draft:
   - specs with `apiPath` -> structured JSON payload for `/api/v1/settings`
   - specs without `apiPath` -> legacy form payload for `/settings`
4. If no changes, show success toast `No device setting changes to apply.`
5. If not applying with reboot and any drafted spec has `requiresReboot`, show a confirm dialog explaining it will not take effect until reboot.
6. POST unified payload first, then legacy payload.
7. Show `Device settings applied.`
8. Reload settings.
9. If Apply + Reboot was used, POST `/reset` with `{ board: 1 }`.

### Setting Widgets

#### Default Input

Used for String, Integer, PositiveBigInteger, and Float unless another widget applies.

- Numeric types render `input[type=number]`.
- String renders `input[type=text]`.
- Set `min`, `max`, and `step=1` where applicable.
- Use `readOnly` when `spec.readOnly`.
- On both `input` and `change`, validate, coerce, and update draft.
- Integer, PositiveBigInteger, and Slider coerce with `Math.trunc(Number(value))`.
- Float coerces with `Number(value)`.
- String remains raw.

#### Boolean

- Render the custom `.mac-switch`.
- Checkbox checked state is Boolean current value.
- Disable when read-only.
- On change, draft Boolean value.

#### Select

- Render `select`.
- Populate from `getWidgetSelectOptions`.
- For numeric setting types, store `Number(control.value)`.
- Otherwise store string.
- Disable when read-only.

#### Slider

Used when `widget.kind == "slider"` or type is Slider.

- Render a range input and number output in `.slider-row`.
- If `widget.displayScale` exists:
  - map raw to display linearly using `rawMin`, `rawMax`, `displayMin`, `displayMax`
  - clamp display to scale range
  - map display back to raw on changes
  - append suffix when `displayScale.suffix` exists
- Without display scale, range min/max come from spec min/max or 0/255.
- Range updates on `input`.
- Number updates on `change`.

#### Interval Toggle

Used when `widget.kind == "intervalToggle"`.

- Render `.interval-row`.
- Contains custom switch, verb label, numeric input, optional unit suffix.
- Raw value 0 means off.
- Switch checked when raw draft is greater than 0.
- Numeric value displays raw / `unitDivisor`.
- When switched off, numeric input is disabled and draft value is 0.
- Fallback display is at least 1, based on current value or 60 units.
- Clamp display to 1..2147483.

#### Color

Used for type Color or `widget.kind == "color"`.

- Render color input plus numeric input in `.color-row`.
- Store colors as integer RGB values from 0 to 16777215.
- Convert integer to hex with six digits for color input.
- Numeric input clamps to 0..16777215.

#### Palette

Used for type Palette.

- Render `.palette-row`.
- Each palette entry is `.palette-entry` with color input and numeric input.
- Store an array of integer RGB values.
- Current UI supports editing existing entries; it does not add/remove palette entries.

### Effect Settings Dialog

Opening:

1. Fetch specs and values concurrently:
   - `/settings/effect/specs?effectIndex=N`
   - `/settings/effect?effectIndex=N`
2. Set dialog state.
3. Render using the same setting field renderer as device settings.
4. Call `showModal()`.

Applying:

- If errors exist, show error toast.
- If no draft changes, close dialog.
- POST `/settings/effect` as form with `effectIndex` plus draft values.
- On success, show `Effect settings applied.`, close dialog, reload effects.

Closing:

- Clear dialog draft and errors.
- Close native dialog if open.

### Statistics Tab

Render only when static and dynamic stats exist. Otherwise show `Statistics data is unavailable.`

Cards:

1. Output
   - Compiled driver
   - Active driver
   - Compiled order
   - Active order
   - Configured dimensions
   - Active dimensions
   - LEDs active / compiled
   - Channels active / compiled
2. Audio
   - Mode
   - Configured pin
   - Compiled pin
   - Audio FPS
   - Frames socket
   - Effects socket
3. CPU, with meter percentage `CPU_USED`
   - Total
   - Core 0
   - Core 1
   - LED FPS
   - Serial FPS
4. Memory, with meter equal to used heap percentage
   - Heap free
   - Heap size
   - DMA free
   - DMA size
   - PSRAM free
   - PSRAM size
5. Package
   - Chip
   - Cores
   - Clock
   - Program
   - Flash
   - Code free
6. Schema, only when `unifiedSettings` has topology, outputs, device.audio, and device.remote
   - Topology live
   - Output live
   - Audio live
   - Audio reboot
   - Remote enabled
   - Remote pin

After rendering, set `#statsTimestamp` to `Updated <local time>`.

### Frame Preview

Connect button calls `connectPreviewSocket`.

Disconnect button:

- disables reconnect
- closes socket if present
- clears frame buffers
- stops render loop
- sets status to `Preview offline`
- hides preview wrapper

On socket open:

- mark connected
- reset last render time
- clear reconnect timer
- status `Preview connected`
- toast success

On socket close:

- mark disconnected
- clear socket and frames
- stop render loop
- status `Preview reconnecting...` if reconnecting, else `Preview offline`
- hide preview wrapper
- schedule reconnect if needed

On message:

- convert `event.data` to `Uint8Array`
- store as latest frame
- mark dirty
- show preview wrapper
- ensure render loop

Display metrics:

- width = active or configured matrix width, fallback 1
- height = active or configured matrix height, fallback 1
- available display width = parent content width or width
- pixel width = available width / width
- pixel height = max(pixel width, 10)
- display height = height * pixel height

Rendering:

- set canvas CSS dimensions to display width/height
- set canvas backing dimensions multiplied by device pixel ratio
- disable image smoothing
- fill one rectangle per pixel from row-major RGB bytes

## Settings Metadata Produced By Firmware

The current firmware emits these major device setting sections:

- `system`
- `location`
- `clock`
- `audio`
- `appearance`
- `topology`
- `output`

Notable settings:

- Hostname: string, reboot required, path `device.hostname`.
- Power limit: integer, validated, path `device.powerLimit`.
- Location: string, path `device.location`.
- Location is postal code: boolean, path `device.locationIsZip`.
- Country code: string select, generated ISO country options, path `device.countryCode`.
- Time zone: string select, options from `/timezones.json`, path `device.timeZone`.
- 24-hour clock: boolean, path `device.use24HourClock`.
- Celsius: boolean, path `device.useCelsius`.
- NTP server: string, path `device.ntpServer`.
- Open Weather API key: write-only string with validation, path `device.openWeatherApiKey`.
- Audio input pin: integer, min -1, max 48, may require reboot, path `device.audioInputPin`.
- Brightness: integer slider, raw brightness range displayed as 5..100%, path `device.brightness`.
- Global color: color integer, path `device.globalColor`.
- Second color: color integer, path `device.secondColor`.
- Apply global color: write-only boolean, path `device.applyGlobalColors`.
- Clear global color: write-only boolean, path `device.clearGlobalColor`.
- Remember current effect: boolean, path `device.rememberCurrentEffect`.
- Matrix width: positive integer, priority 0, path `topology.width`.
- Matrix height: positive integer, priority 1, path `topology.height`.
- Serpentine layout: boolean, priority 2, path `topology.serpentine`.
- Output driver: select from schema path `outputs.allowedDrivers`, path `outputs.driver`.
- WS281x channel count: select from `outputs.ws281x.allowedChannelCounts`, path `outputs.ws281x.channelCount`.
- WS281x color order: select from `outputs.ws281x.allowedColorOrders`, path `outputs.ws281x.colorOrder`.
- WS281x pin N: integer min -1 max 48, path `outputs.ws281x.pins[N]`.

The current webserver-owned `effectInterval` spec is in the `appearance` section:

- PositiveBigInteger.
- Path `effects.effectInterval`.
- Widget `intervalToggle`.
- Unit divisor 1000.
- On label `Rotate effects`.
- Off label `Off`.
- Unit label `seconds`.

## Accessibility And Semantics

- Tabs use `role=tablist`, `role=tab`, `role=tabpanel`, `aria-selected`, and `aria-controls`.
- Toast stack uses `aria-live=polite`.
- Icon-like buttons must set `title` and `aria-label`.
- Form controls must be wrapped in labels where practical or clearly associated with visible text.
- The native dialog provides modal semantics.
- Hidden tab panes must use the `hidden` attribute.

## Error Handling

- Any failed operation should call a shared error handler that logs to console and shows a toast.
- HTTP error messages should prefer a JSON `message` field when available.
- Partial initial load is acceptable; render what succeeded and toast failures.
- Settings validation failures must not post to firmware.
- Effect setting validation failures must not post to firmware.
- If effect enable/disable fails, restore the checkbox to its previous state.

## Regeneration Checklist

A regenerated UI is compatible when all of the following are true:

- `tools/bake_site.py` can gzip `site/index.html`, `site/styles.css`, and `site/app.js`.
- PlatformIO can embed the gzipped files without new frontend dependencies.
- The static DOM provides every ID consumed by the script.
- Initial load fetches all current endpoints and tolerates partial failure.
- Effects table can enable/disable, trigger, configure, delete, and reorder effects.
- Device settings render from firmware specs and schema, including all current widget kinds.
- Settings with `apiPath` post through `/api/v1/settings`; settings without it post through `/settings`.
- Array-element api paths seed existing arrays before writing.
- Matrix width/height are cross-validated against compiled LED capacity.
- Reboot-required setting changes warn on normal Apply.
- Apply + Reboot posts settings first, then `/reset`.
- Statistics cards use the same static/dynamic fields.
- Preview WebSocket renders packed RGB frames against active matrix dimensions.
- Auto-refresh and selected tab persist in localStorage under the existing keys.
- The app remains usable below 760px width.

## Implementation Notes

- The current code intentionally keeps all frontend behavior in one `site/app.js` file. Regeneration may reorganize internally, but the delivered asset must remain a static script or be bundled into one static script before embedding.
- Avoid hardcoding specific setting lists into the UI renderer. The firmware setting spec and schema are the contract.
- Preserve old endpoint aliases where currently used by firmware, but the UI should prefer `/effects`, `/statistics/static`, `/statistics/dynamic`, `/settings`, `/settings/specs`, `/api/v1/settings`, and `/api/v1/settings/schema`.
- The UI should never require CORS special handling when served from the device, but the firmware currently adds CORS headers to API responses.
- The app should not assume the device is online forever; polling and WebSocket failure paths must remain graceful.
