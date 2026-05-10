/* Abandon all hope, all ye who enter.  Here be AI code that even the author doesn't understand.  You've been warned.

                          ud$$$**$$$$$$$bc.
                       u@**"        4$$$$$$$Nu
                     J                ""#$$$$$$r
                    @                       $$$$b
                  .F                        ^*3$$$
                 :% 4                         J$$$N
                 $  :F                       :$$$$$
                4F  9                       J$$$$$$$
                4$   k             4$$$$bed$$$$$$$$$
                $$r  'F            $$$$$$$$$$$$$$$$$r
                $$$   b.           $$$$$$$$$$$$$$$$$N
                $$$$$k 3eeed$$b    $$$Euec."$$$$$$$$$
 .@$**N.        $$$$$" $$$$$$F'L $$$$$$$$$$$  $$$$$$$
 :$$L  'L       $$$$$ 4$$$$$$  * $$$$$$$$$$F  $$$$$$F         edNc
@$$$$N  ^k      $$$$$  3$$$$*%   $F4$$$$$$$   $$$$$"        d"  z$N
$$$$$$   ^k     '$$$"   #$$$F   .$  $$$$$c.u@$$$          J"  @$$$$r
$$$$$$$b   *u    ^$L            $$  $$$$$$$$$$$$u@       $$  d$$$$$$
 ^$$$$$$.    "NL   "N. z@*     $$$  $$$$$$$$$$$$$P      $P  d$$$$$$$
    ^"*$$$$b   '*L   9$E      4$$$  d$$$$$$$$$$$"     d*   J$$$$$r
         ^$$$$u  '$.  $$$L     "#" d$$$$$$".@$$    .@$"  z$$$$*"
           ^$$$$. ^$N.3$$$       4u$$$$$$$ 4$$$  u$*" z$$$"
             '*$$$$$$$$ *$b      J$$$$$$$b u$$P $"  d$$P
                #$$$$$$ 4$ 3*$"$*$ $"$'c@@$$$$ .u@$$$P
                  "$$$$  ""F~$ $uNr$$$^&J$$$$F $$$$#
                    "$$    "$$$bd$.$W$$$$$$$$F $$"
                      ?k         ?$$$$$$$$$$$F'*
                       9$$bL     z$$$$$$$$$$$F
                        $$$$    $$$$$$$$$$$$$
                         '#$$c  '$$$$$$$$$"
                          .@"#$$$$$$$$$$$$b
                        z*      $$$$$$$$$$$$N.
                      e"      z$$"  #$$$k  '*$$.
                  .u*      u@$P"      '#$$c   "$$c
           u@$*"""       d$$"            "$$$u  ^*$$b.
         :$F           J$P"                ^$$$c   '"$$$$$$bL
        d$$  ..      @$#                      #$$b         '#$
        9$$$$$$b   4$$                          ^$$k         '$
         "$$6""$b u$$                             '$    d$$$$$P
           '$F $$$$$"                              ^b  ^$$$$b$
            '$W$$$$"                                'b@$$$$"
                                                     ^$$$*
*/

(function () {
  "use strict";

  const settingType = {
    Integer: 0,
    PositiveBigInteger: 1,
    Float: 2,
    Boolean: 3,
    String: 4,
    Palette: 5,
    Color: 6,
    Slider: 7
  };

  const state = {
    staticStats: null,
    dynamicStats: null,
    settings: null,
    settingsSpecs: [],
    timezones: null,
    timezonesLoading: false,
    unifiedSettings: null,
    unifiedSchema: null,
    effects: null,
    effectDraft: {},
    deviceDraft: {},
    deviceErrors: new Set(),
    effectDialog: {
      open: false,
      effectIndex: null,
      effectName: "",
      specs: [],
      values: {},
      draft: {},
      errors: new Set()
    },
    autoRefresh: true,
    statsRefreshSeconds: Number(localStorage.getItem("nd.statsRefreshSeconds") || 3),
    // True from the moment the user types into the hero's effect-interval
    // input until the next successful save, so the periodic effects refresh
    // doesn't snap their unsaved edit back to the device's current value.
    effectIntervalDirty: false,
    timers: {
      stats: null,
      effects: null,
      countdown: null
    },
    preview: {
      socket: null,
      connected: false,
      frame: null,
      latestFrame: null,
      dirty: false,
      animationFrameId: 0,
      lastRenderMs: 0,
      shouldReconnect: false,
      reconnectTimer: null
    },
    drag: {
      effectIndex: null,
      dropIndex: null
    },
    activeTab: localStorage.getItem("nd.activeTab") || "effects"
  };

  const COUNTRY_OPTIONS = buildCountryOptions();

  const els = {};

  document.addEventListener("DOMContentLoaded", init);

  async function init() {
    bindElements();
    bindEvents();
    initializeStaticShell();
    await loadAll();
    void ensureTimezonesLoaded();
    restartPolling();
  }

  function bindElements() {
    const ids = [
      "connectionStatus", "connectionStatusText", "hostValue", "webPortValue",
      "prevEffectButton", "nextEffectButton", "refreshEffectsButton",
      "effectIntervalInput", "saveIntervalButton", "statsRefreshInput", "autoRefreshToggle",
      "rebootButton", "resetDeviceConfigButton", "resetEffectsConfigButton",
      "summaryCurrentEffect", "summaryEffectStatus", "summaryInterval", "summaryIntervalRemaining",
      "summaryTopology", "summaryDriver", "summaryLedFps", "summaryAudioFps", "summaryCpu",
      "summaryCpuCores", "summaryHeap", "summaryPsram",
      "effectsMeta", "effectsTableBody", "reloadSettingsButton", "applySettingsButton",
      "applySettingsRebootButton", "deviceSettingsForm", "statsTimestamp", "statsGrid",
      "previewConnectButton", "previewDisconnectButton", "previewStatus", "previewWrap", "previewCanvas",
      "tabEffectsButton", "tabSettingsButton", "tabStatisticsButton",
      "tabEffectsPane", "tabSettingsPane", "tabStatisticsPane", "tabBodyPanel",
      "effectSettingsDialog", "effectDialogTitle", "effectSettingsForm", "closeEffectDialogButton",
      "cancelEffectDialogButton", "applyEffectDialogButton", "toastStack"
    ];
    ids.forEach((id) => {
      els[id] = document.getElementById(id);
    });
  }

  function bindEvents() {
    els.prevEffectButton.addEventListener("click", () => postForm("/previousEffect").then(loadEffectsOnly));
    els.nextEffectButton.addEventListener("click", () => postForm("/nextEffect").then(loadEffectsOnly));
    els.refreshEffectsButton.addEventListener("click", () => loadEffectsOnly());
    els.saveIntervalButton.addEventListener("click", applyEffectInterval);
    els.effectIntervalInput.addEventListener("input", () => { state.effectIntervalDirty = true; });
    els.reloadSettingsButton.addEventListener("click", () => loadSettingsOnly());
    els.applySettingsButton.addEventListener("click", () => applyDeviceSettings(false));
    els.applySettingsRebootButton.addEventListener("click", () => applyDeviceSettings(true));
    els.rebootButton.addEventListener("click", () => resetDevice({ board: 1 }));
    els.resetDeviceConfigButton.addEventListener("click", () => resetDevice({ board: 1, deviceConfig: 1 }));
    els.resetEffectsConfigButton.addEventListener("click", () => resetDevice({ board: 1, effectsConfig: 1 }));
    els.statsRefreshInput.addEventListener("change", () => {
      state.statsRefreshSeconds = clampInt(els.statsRefreshInput.value, 1, 60, 3);
      localStorage.setItem("nd.statsRefreshSeconds", String(state.statsRefreshSeconds));
      restartPolling();
    });
    els.autoRefreshToggle.addEventListener("change", () => {
      state.autoRefresh = !!els.autoRefreshToggle.checked;
      restartPolling();
    });
    els.previewConnectButton.addEventListener("click", connectPreviewSocket);
    els.previewDisconnectButton.addEventListener("click", disconnectPreviewSocket);
    els.tabEffectsButton.addEventListener("click", () => setActiveTab("effects"));
    els.tabSettingsButton.addEventListener("click", () => setActiveTab("settings"));
    els.tabStatisticsButton.addEventListener("click", () => setActiveTab("statistics"));
    els.closeEffectDialogButton.addEventListener("click", closeEffectDialog);
    els.cancelEffectDialogButton.addEventListener("click", closeEffectDialog);
    els.applyEffectDialogButton.addEventListener("click", applyEffectSettings);
    window.addEventListener("resize", () => {
      updateActiveTabCutout();
      drawPreviewFrame();
    });
  }

  function initializeStaticShell() {
    els.hostValue.textContent = window.location.hostname || "--";
    els.webPortValue.textContent = window.location.port || (window.location.protocol === "https:" ? "443" : "80");
    els.statsRefreshInput.value = state.statsRefreshSeconds;
    els.autoRefreshToggle.checked = state.autoRefresh;
    setActiveTab(state.activeTab);
    setConnectionState("Starting", "pending");
  }

  function setActiveTab(tabName) {
    state.activeTab = tabName;
    localStorage.setItem("nd.activeTab", tabName);

    const themeClassByTab = {
      effects: "card-green",
      settings: "card-blue",
      statistics: "card-purple"
    };

    const tabs = [
      { name: "effects", button: els.tabEffectsButton, pane: els.tabEffectsPane },
      { name: "settings", button: els.tabSettingsButton, pane: els.tabSettingsPane },
      { name: "statistics", button: els.tabStatisticsButton, pane: els.tabStatisticsPane }
    ];

    tabs.forEach((tab) => {
      const active = tab.name === tabName;
      tab.button.classList.toggle("active", active);
      tab.button.setAttribute("aria-selected", active ? "true" : "false");
      tab.pane.classList.toggle("active", active);
      tab.pane.hidden = !active;
    });

    Object.values(themeClassByTab).forEach((className) => {
      els.tabBodyPanel.classList.remove(className);
    });
    els.tabBodyPanel.classList.add(themeClassByTab[tabName] || "card-green");
    updateActiveTabCutout();

    if (tabName === "settings") {
      void ensureTimezonesLoaded();
    }
  }

  function updateActiveTabCutout() {
    const activeButton = [els.tabEffectsButton, els.tabSettingsButton, els.tabStatisticsButton]
      .find((button) => button && button.classList.contains("active"));

    if (!activeButton) {
      return;
    }

    const cutLeft = 18 + activeButton.offsetLeft;
    const cutWidth = activeButton.offsetWidth;
    els.tabBodyPanel.style.setProperty("--tab-cut-left", `${cutLeft}px`);
    els.tabBodyPanel.style.setProperty("--tab-cut-width", `${cutWidth}px`);
  }

  async function loadAll() {
    setConnectionState("Loading", "pending");
    const requests = [
      ["/statistics/static", "staticStats"],
      ["/statistics/dynamic", "dynamicStats"],
      ["/settings", "settings"],
      ["/settings/specs", "settingsSpecs"],
      ["/api/v1/settings", "unifiedSettings"],
      ["/api/v1/settings/schema", "unifiedSchema"],
      ["/effects", "effects"]
    ];

    const results = await Promise.allSettled(requests.map(([path]) => fetchJson(path)));
    let anySuccess = false;
    const failures = [];

    results.forEach((result, index) => {
      const [path, stateKey] = requests[index];
      if (result.status === "fulfilled") {
        state[stateKey] = result.value;
        anySuccess = true;
      } else {
        failures.push(`${path}: ${result.reason && result.reason.message ? result.reason.message : "failed"}`);
      }
    });

    state.deviceDraft = {};
    state.deviceErrors.clear();

    safeRenderAfterLoad();

    if (state.activeTab === "settings") {
      void ensureTimezonesLoaded();
    }

    if (failures.length > 0) {
      handleError("Some data failed to load", new Error(failures.join(" | ")));
    }

    setConnectionState(anySuccess ? "Connected" : "Offline", anySuccess ? "online" : "offline");
  }

  async function loadEffectsOnly() {
    try {
      state.effects = await fetchJson("/effects");
      renderEffects();
      renderSummaries();
    } catch (error) {
      handleError("Failed to refresh effects", error);
    }
  }

  async function loadSettingsOnly() {
    try {
      const [settings, settingsSpecs, unifiedSettings, unifiedSchema, staticStats] = await Promise.all([
        fetchJson("/settings"),
        fetchJson("/settings/specs"),
        fetchJson("/api/v1/settings"),
        fetchJson("/api/v1/settings/schema"),
        fetchJson("/statistics/static")
      ]);
      state.settings = settings;
      state.settingsSpecs = settingsSpecs;
      state.unifiedSettings = unifiedSettings;
      state.unifiedSchema = unifiedSchema;
      state.staticStats = staticStats;
      state.deviceDraft = {};
      state.deviceErrors.clear();
      renderSettingsForm();
      renderSummaries();
      renderStats();
      void ensureTimezonesLoaded();
    } catch (error) {
      handleError("Failed to reload settings", error);
    }
  }

  function safeRenderAfterLoad() {
    try {
      renderSettingsForm();
    } catch (error) {
      handleError("Failed to render settings", error);
    }

    try {
      renderEffects();
    } catch (error) {
      handleError("Failed to render effects", error);
    }

    try {
      renderSummaries();
    } catch (error) {
      handleError("Failed to render summary", error);
    }

    try {
      renderStats();
    } catch (error) {
      handleError("Failed to render statistics", error);
    }

    try {
      drawPreviewFrame();
    } catch (error) {
      handleError("Failed to render preview frame", error);
    }
  }

  async function loadDynamicStatsOnly() {
    if (!state.autoRefresh) {
      return;
    }
    try {
      state.dynamicStats = await fetchJson("/statistics/dynamic");
      renderSummaries();
      renderStats();
    } catch (error) {
      handleError("Failed to refresh statistics", error);
    }
  }

  function restartPolling() {
    clearTimer("stats");
    clearTimer("effects");
    clearTimer("countdown");

    if (state.autoRefresh) {
      state.timers.stats = window.setInterval(loadDynamicStatsOnly, state.statsRefreshSeconds * 1000);
      state.timers.effects = window.setInterval(loadEffectsOnly, 3000);
    }

    state.timers.countdown = window.setInterval(renderSummaries, 250);
  }

  function clearTimer(key) {
    if (state.timers[key]) {
      window.clearInterval(state.timers[key]);
      state.timers[key] = null;
    }
  }

  function setConnectionState(text, stateClass) {
    els.connectionStatusText.textContent = text;
    els.connectionStatus.classList.remove("online", "offline");
    if (stateClass === "online" || stateClass === "offline") {
      els.connectionStatus.classList.add(stateClass);
    }
  }

  function renderSummaries() {
    const effects = state.effects;
    const staticStats = state.staticStats;
    const dynamicStats = state.dynamicStats;

    if (!effects || !staticStats || !dynamicStats) {
      return;
    }

    const effectList = Array.isArray(effects.Effects) ? effects.Effects : [];
    const currentIndex = Number(effects.currentEffect || 0);
    const currentEffect = effectList[currentIndex];
    const intervalMs = Number(effects.effectInterval || 0);
    const pinned = !!effects.eternalInterval || intervalMs === 0;
    const remainingMs = Number(effects.millisecondsRemaining || 0);
    const intervalScale = getIntervalDisplayScale("effectInterval");

    els.summaryCurrentEffect.textContent = currentEffect ? currentEffect.name : "--";
    els.summaryEffectStatus.textContent = currentEffect ? (currentEffect.enabled ? "Enabled" : "Disabled") : "No effect";
    els.summaryInterval.textContent = pinned
      ? (intervalScale.offLabel || "Off")
      : formatIntervalDisplay(intervalMs, intervalScale);
    els.summaryIntervalRemaining.textContent = pinned
      ? (intervalScale.offLabel || "Off")
      : `${Math.max(0, Math.ceil(remainingMs / intervalScale.divisor))} ${intervalScale.unitLabel}`.trim();
    els.summaryTopology.textContent = `${staticStats.ACTIVE_MATRIX_WIDTH}x${staticStats.ACTIVE_MATRIX_HEIGHT} / ${staticStats.ACTIVE_NUM_LEDS} leds`;
    els.summaryDriver.textContent = `${staticStats.ACTIVE_OUTPUT_DRIVER} / ${staticStats.ACTIVE_NUM_CHANNELS} ch`;
    els.summaryLedFps.textContent = formatNumber(dynamicStats.LED_FPS);
    els.summaryAudioFps.textContent = `Audio ${formatNumber(dynamicStats.AUDIO_FPS)} / Serial ${formatNumber(dynamicStats.SERIAL_FPS)}`;
    els.summaryCpu.textContent = `${formatPercent(dynamicStats.CPU_USED)}%`;
    els.summaryCpuCores.textContent = `C0 ${formatPercent(dynamicStats.CPU_USED_CORE0)}% / C1 ${formatPercent(dynamicStats.CPU_USED_CORE1)}%`;
    els.summaryHeap.textContent = formatBytes(dynamicStats.HEAP_FREE);
    els.summaryPsram.textContent = `PSRAM ${formatBytes(dynamicStats.PSRAM_FREE)}`;

    // Don't clobber the user's in-progress edit. The dirty flag is set on
    // every keystroke and cleared only by a successful save, so a refresh
    // tick that fires while the user is typing — or after they've blurred
    // but before they hit Save — leaves their value alone.
    if (!state.effectIntervalDirty && document.activeElement !== els.effectIntervalInput) {
      els.effectIntervalInput.value = String(Math.round(intervalMs / intervalScale.divisor));
    }
    els.effectsMeta.textContent = `${effectList.length} effects / active ${currentIndex}`;
  }

  function renderEffects() {
    const effects = state.effects;
    if (!effects || !Array.isArray(effects.Effects)) {
      els.effectsTableBody.innerHTML = '<tr><td colspan="6" class="empty-cell">No effects loaded.</td></tr>';
      return;
    }

    const rows = effects.Effects.map((effect, index) => {
      const tr = document.createElement("tr");
      const isCurrent = index === Number(effects.currentEffect || 0);
      tr.dataset.effectIndex = String(index);

      const gripCell = document.createElement("td");
      gripCell.className = "grip-cell";
      const grip = document.createElement("span");
      grip.className = "drag-grip";
      grip.draggable = true;
      grip.title = "Drag to reorder";
      grip.addEventListener("dragstart", (event) => handleEffectDragStart(event, index, tr));
      grip.addEventListener("dragend", () => clearEffectDragState());
      gripCell.appendChild(grip);
      tr.appendChild(gripCell);

      tr.addEventListener("dragover", (event) => handleEffectDragOver(event, index, tr));
      tr.addEventListener("drop", (event) => handleEffectDrop(event, index));

      const onCell = document.createElement("td");
      const toggle = document.createElement("input");
      toggle.type = "checkbox";
      toggle.checked = !!effect.enabled;
      toggle.addEventListener("change", () => {
        postForm(effect.enabled ? "/disableEffect" : "/enableEffect", { effectIndex: index }).then(loadEffectsOnly).catch((error) => {
          toggle.checked = !!effect.enabled;
          handleError("Failed to toggle effect", error);
        });
      });
      onCell.appendChild(toggle);
      tr.appendChild(onCell);

      const nameCell = document.createElement("td");
      nameCell.innerHTML = `<span class="effect-name">${escapeHtml(effect.name || `Effect ${index}`)}</span>`;
      tr.appendChild(nameCell);

      const statusCell = document.createElement("td");
      statusCell.className = isCurrent ? "effect-active" : "effect-disabled";
      statusCell.textContent = isCurrent ? "Active" : (effect.enabled ? "Queued" : "Disabled");
      tr.appendChild(statusCell);

      const coreCell = document.createElement("td");
      coreCell.textContent = effect.core ? "Core" : "User";
      tr.appendChild(coreCell);

      const actionsCell = document.createElement("td");
      actionsCell.className = "row-actions";
      actionsCell.appendChild(miniIconButton("▶", "Trigger effect", () => postForm("/currentEffect", { currentEffectIndex: index }).then(loadEffectsOnly), !effect.enabled));
      actionsCell.appendChild(miniIconButton("⚙", "Effect settings", () => openEffectDialog(index)));
      actionsCell.appendChild(miniIconButton("🗑", "Delete effect", () => deleteEffect(index), !!effect.core));
      tr.appendChild(actionsCell);

      return tr;
    });

    els.effectsTableBody.replaceChildren(...rows);
  }

  function handleEffectDragStart(event, effectIndex, row) {
    state.drag.effectIndex = effectIndex;
    state.drag.dropIndex = effectIndex;
    row.classList.add("dragging");
    if (event.dataTransfer) {
      event.dataTransfer.effectAllowed = "move";
      event.dataTransfer.setData("text/plain", String(effectIndex));
    }
  }

  function handleEffectDragOver(event, effectIndex, row) {
    if (state.drag.effectIndex === null) {
      return;
    }
    event.preventDefault();
    state.drag.dropIndex = effectIndex;
    Array.from(els.effectsTableBody.querySelectorAll("tr")).forEach((candidate) => {
      candidate.classList.toggle("drop-target", candidate === row);
    });
  }

  async function handleEffectDrop(event, effectIndex) {
    event.preventDefault();
    const fromIndex = state.drag.effectIndex;
    clearEffectDragState();
    if (fromIndex === null || fromIndex === effectIndex) {
      return;
    }
    try {
      await moveEffect(fromIndex, effectIndex);
    } catch (_error) {
      // moveEffect already reports errors
    }
  }

  function clearEffectDragState() {
    state.drag.effectIndex = null;
    state.drag.dropIndex = null;
    Array.from(els.effectsTableBody.querySelectorAll("tr")).forEach((row) => {
      row.classList.remove("dragging", "drop-target");
    });
  }

  function renderSettingsForm() {
    if (!state.settings || !Array.isArray(state.settingsSpecs)) {
      els.deviceSettingsForm.innerHTML = '<div class="empty-cell">Settings data is unavailable.</div>';
      return;
    }

    const fragment = document.createDocumentFragment();
    getSettingsSections().forEach((section) => {
      if (section.specs.length === 0) {
        return;
      }

      const sectionNode = document.createElement("section");
      sectionNode.className = "settings-section";

      const headerNode = document.createElement("header");
      headerNode.className = "settings-section-header";

      const titleNode = document.createElement("h3");
      titleNode.textContent = section.title;
      headerNode.appendChild(titleNode);

      if (section.description) {
        const descriptionNode = document.createElement("p");
        descriptionNode.textContent = section.description;
        headerNode.appendChild(descriptionNode);
      }

      sectionNode.appendChild(headerNode);

      const bodyNode = document.createElement("div");
      bodyNode.className = "settings-section-body";
      section.specs.forEach((spec) => {
        const currentValue = getCurrentDeviceSettingValue(spec);
        bodyNode.appendChild(buildSettingField(spec, currentValue, state.deviceDraft, state.deviceErrors, false));
      });
      sectionNode.appendChild(bodyNode);
      fragment.appendChild(sectionNode);
    });

    els.deviceSettingsForm.replaceChildren(fragment);
  }

  // Section catalog comes from /api/v1/settings/schema (root.sections). The UI
  // groups settings by spec.section and uses the catalog entries for each
  // section's heading and subtitle. If the schema doesn't carry one (older
  // firmware), fall back to a single bucket so nothing renders blank.
  function getSettingsSections() {
    const catalog = Array.isArray((state.unifiedSchema || {}).sections) && state.unifiedSchema.sections.length > 0
      ? state.unifiedSchema.sections
      : [{ id: "system", title: "Settings", description: "" }];

    const orderedSpecs = state.settingsSpecs
      .filter((spec) => !(spec.writeOnly && !spec.hasValidation))
      .slice()
      .sort(compareSettingSpecs);

    const bySection = new Map();
    orderedSpecs.forEach((spec) => {
      const key = spec.section || "system";
      if (!bySection.has(key)) bySection.set(key, []);
      bySection.get(key).push(spec);
    });

    return catalog.map((section) => ({
      key: section.id,
      title: section.title,
      description: section.description,
      specs: bySection.get(section.id) || []
    }));
  }

  // Compare two specs for display order: spec.priority wins (lower = higher),
  // ties break by friendly name. Specs without an explicit priority sort to
  // the end of their section.
  function compareSettingSpecs(left, right) {
    const leftRank = typeof left.priority === "number" ? left.priority : 100;
    const rightRank = typeof right.priority === "number" ? right.priority : 100;
    if (leftRank !== rightRank) {
      return leftRank - rightRank;
    }
    return String(left.friendlyName || left.name).localeCompare(String(right.friendlyName || right.name));
  }

  // Resolve a setting's current value. Specs with an apiPath read from the
  // unified settings document; specs without one fall back to the legacy flat
  // /settings response. Either way the spec's name is the draft-store key.
  function getCurrentDeviceSettingValue(spec) {
    if (spec.apiPath) {
      const fromUnified = readJsonPath(state.unifiedSettings, spec.apiPath);
      if (fromUnified !== undefined) {
        return fromUnified;
      }
    }

    if (Object.prototype.hasOwnProperty.call(state.settings, spec.name)) {
      return state.settings[spec.name];
    }

    return defaultValueForSpec(spec);
  }

  // Walk a dotted path with optional [N] segments against an object.
  // E.g. readJsonPath({outputs:{ws281x:{pins:[5,7]}}}, "outputs.ws281x.pins[1]") -> 7
  function readJsonPath(root, path) {
    if (!root || typeof path !== "string" || path.length === 0) {
      return undefined;
    }
    let current = root;
    for (const segment of parseJsonPath(path)) {
      if (current == null || typeof current !== "object") {
        return undefined;
      }
      current = current[segment];
    }
    return current;
  }

  function parseJsonPath(path) {
    const segments = [];
    String(path).split(".").forEach((part) => {
      const match = /^([^[]*)((?:\[\d+\])*)$/.exec(part);
      if (!match) return;
      if (match[1].length > 0) {
        segments.push(match[1]);
      }
      const indexMatches = match[2].matchAll(/\[(\d+)\]/g);
      for (const indexMatch of indexMatches) {
        segments.push(Number(indexMatch[1]));
      }
    });
    return segments;
  }

  // Set a value at the same dotted path (creating intermediate objects/arrays
  // as needed). Used when routing draft entries into the unified-settings POST
  // body keyed by spec.apiPath.
  function writeJsonPath(root, path, value) {
    const segments = parseJsonPath(path);
    if (segments.length === 0) {
      return root;
    }

    let cursor = root;
    for (let i = 0; i < segments.length - 1; i += 1) {
      const key = segments[i];
      const nextIsIndex = typeof segments[i + 1] === "number";
      if (cursor[key] == null) {
        cursor[key] = nextIsIndex ? [] : {};
      }
      cursor = cursor[key];
    }
    cursor[segments[segments.length - 1]] = value;
    return root;
  }

  function renderEffectDialogForm() {
    const dialog = state.effectDialog;
    if (!dialog.open) {
      return;
    }
    const fragment = document.createDocumentFragment();
    dialog.specs.forEach((spec) => {
      const currentValue = Object.prototype.hasOwnProperty.call(dialog.values, spec.name)
        ? dialog.values[spec.name]
        : defaultValueForSpec(spec);
      fragment.appendChild(buildSettingField(spec, currentValue, dialog.draft, dialog.errors, true));
    });
    els.effectDialogTitle.textContent = `${dialog.effectName} Settings`;
    els.effectSettingsForm.replaceChildren(fragment);
  }

  function buildSettingField(spec, currentValue, draftStore, errorSet, isEffectDialog) {
    const widget = spec.widget || {};
    const widgetKind = widget.kind || "default";

    const wrapper = document.createElement("div");
    const needsStackedLayout = spec.type === settingType.Palette;
    wrapper.className = "setting-row" + (needsStackedLayout ? " stacked" : "");

    const meta = document.createElement("div");
    meta.className = "setting-meta";
    const title = document.createElement("div");
    title.className = "setting-name";
    title.textContent = spec.friendlyName || spec.name;
    meta.appendChild(title);
    wrapper.appendChild(meta);

    const valueWrap = document.createElement("div");
    valueWrap.className = "setting-value";
    wrapper.appendChild(valueWrap);

    const key = spec.name;
    const currentDraft = Object.prototype.hasOwnProperty.call(draftStore, key) ? draftStore[key] : currentValue;
    const readOnly = !!spec.readOnly;

    const setDraftValue = (value) => {
      if (valuesEqual(value, currentValue)) {
        delete draftStore[key];
      } else {
        draftStore[key] = value;
      }
    };

    const setFieldError = (hasError, message) => {
      const help = wrapper.querySelector(".field-help");
      if (hasError) {
        errorSet.add(key);
        help.textContent = message;
        help.classList.add("field-error");
      } else {
        errorSet.delete(key);
        help.innerHTML = spec.description || "";
        help.classList.remove("field-error");
      }
    };

    const ctx = { spec, widget, valueWrap, currentDraft, currentValue, readOnly, setDraftValue, setFieldError };

    if (widgetKind === "intervalToggle") {
      renderIntervalToggleWidget(ctx);
    } else if (widgetKind === "select") {
      renderSelectWidget(ctx);
    } else if (widgetKind === "slider" || (widgetKind === "default" && spec.type === settingType.Slider)) {
      renderSliderWidget(ctx);
    } else if (spec.type === settingType.Boolean) {
      renderBooleanWidget(ctx);
    } else if (spec.type === settingType.Color || widgetKind === "color") {
      renderColorWidget(ctx);
    } else if (spec.type === settingType.Palette) {
      renderPaletteWidget(ctx);
    } else {
      renderDefaultInputWidget(ctx);
    }

    const help = document.createElement("div");
    help.className = "field-help";
    help.innerHTML = spec.description || "";
    meta.appendChild(help);

    if (isEffectDialog) {
      wrapper.dataset.dialogField = "1";
    }

    return wrapper;
  }

  // Composite "verb-toggle + numeric value" widget. The unitDivisor and labels
  // come from spec.widget.interval. Raw value 0 means "off" (the numeric input
  // is disabled and the stored value stays at 0); when off, the on-label
  // describes the action the toggle would perform if enabled, and the off-label
  // is the noun used elsewhere to display the off state ("Pinned").
  function renderIntervalToggleWidget(ctx) {
    const { spec, widget, valueWrap, currentDraft, currentValue, readOnly, setDraftValue, setFieldError } = ctx;
    const interval = widget.interval || {};
    const divisor = Number(interval.unitDivisor) || 1;
    const unitLabel = interval.unitLabel || "";
    const onLabel = interval.onLabel || "Enable";

    const rawDraft = Number(currentDraft || 0);
    const rawCurrent = Number(currentValue || 0);
    const fallbackDisplay = Math.max(1, Math.round(Math.max(rawCurrent, divisor * 60) / divisor));
    const initialDisplay = Math.max(1, Math.round((rawDraft > 0 ? rawDraft : Math.max(rawCurrent, divisor * 60)) / divisor));

    const row = document.createElement("div");
    row.className = "interval-row";

    const switchLabel = document.createElement("label");
    switchLabel.className = "mac-switch";
    const toggle = document.createElement("input");
    toggle.type = "checkbox";
    toggle.checked = rawDraft > 0;
    toggle.disabled = readOnly;
    const track = document.createElement("span");
    track.className = "mac-switch-track";
    track.appendChild(Object.assign(document.createElement("span"), { className: "mac-switch-thumb" }));
    switchLabel.appendChild(toggle);
    switchLabel.appendChild(track);

    const verbLabel = document.createElement("span");
    verbLabel.className = "interval-switch-label";
    verbLabel.textContent = onLabel;

    const valueField = document.createElement("label");
    valueField.className = "inline-field";
    const valueInput = document.createElement("input");
    valueInput.type = "number";
    valueInput.min = "1";
    valueInput.step = "1";
    valueInput.inputMode = "numeric";
    valueInput.value = String(initialDisplay || fallbackDisplay);
    valueInput.disabled = readOnly || !toggle.checked;
    valueField.appendChild(valueInput);
    if (unitLabel) {
      const suffix = document.createElement("span");
      suffix.textContent = unitLabel;
      valueField.appendChild(suffix);
    }

    const sync = () => {
      const display = clampInt(valueInput.value, 1, 2147483, fallbackDisplay);
      valueInput.value = String(display);
      valueInput.disabled = readOnly || !toggle.checked;
      setDraftValue(toggle.checked ? display * divisor : 0);
      setFieldError(false, "");
    };

    toggle.addEventListener("change", sync);
    valueInput.addEventListener("change", sync);

    row.appendChild(switchLabel);
    row.appendChild(verbLabel);
    row.appendChild(valueField);
    valueWrap.appendChild(row);
  }

  // Generic select widget. Options are resolved via getWidgetSelectOptions(),
  // which handles all four sources (inline, schemaPath, intl country codes,
  // external timezones). PositiveBigInteger and Integer specs coerce their
  // selected value back to Number so the device receives correct types.
  function renderSelectWidget(ctx) {
    const { spec, widget, valueWrap, currentDraft, readOnly, setDraftValue, setFieldError } = ctx;
    const control = document.createElement("select");
    const options = getWidgetSelectOptions(spec, widget);
    options.forEach((option) => {
      const optionEl = document.createElement("option");
      optionEl.value = option.value;
      optionEl.textContent = option.label;
      optionEl.selected = String(option.value) === String(currentDraft);
      control.appendChild(optionEl);
    });
    control.disabled = readOnly;
    const numeric = spec.type === settingType.Integer || spec.type === settingType.PositiveBigInteger || spec.type === settingType.Float;
    control.addEventListener("change", () => {
      setDraftValue(numeric ? Number(control.value) : control.value);
      setFieldError(false, "");
    });
    valueWrap.appendChild(control);
  }

  // Slider widget with optional displayScale (raw <-> display linear remap).
  // Pure-numeric sliders without a displayScale operate on raw min/max.
  function renderSliderWidget(ctx) {
    const { spec, widget, valueWrap, currentDraft, currentValue, readOnly, setDraftValue, setFieldError } = ctx;
    const scale = widget.displayScale || null;
    const toDisplay = (raw) => scale ? clampDisplay(scale, mapRawToDisplay(scale, raw)) : raw;
    const toRaw = (display) => scale ? Math.round(mapDisplayToRaw(scale, clampDisplay(scale, display))) : Math.round(Number(display));

    const sliderMin = scale ? scale.displayMin : (spec.minimumValue ?? 0);
    const sliderMax = scale ? scale.displayMax : (spec.maximumValue ?? 255);

    const control = document.createElement("input");
    control.type = "range";
    control.min = sliderMin;
    control.max = sliderMax;
    control.value = String(toDisplay(currentDraft));
    control.disabled = readOnly;

    const output = document.createElement("input");
    output.type = "number";
    output.value = String(toDisplay(currentDraft));
    output.disabled = readOnly;

    const handleDisplay = (display) => {
      const intValue = clampInt(display, Number(control.min), Number(control.max), toDisplay(currentValue));
      output.value = String(intValue);
      control.value = String(intValue);
      setDraftValue(toRaw(intValue));
      setFieldError(false, "");
    };

    output.addEventListener("change", () => handleDisplay(output.value));
    control.addEventListener("input", () => handleDisplay(control.value));

    const row = document.createElement("div");
    row.className = "slider-row";
    row.appendChild(control);
    row.appendChild(output);
    if (scale && scale.suffix) {
      const suffix = document.createElement("span");
      suffix.className = "slider-suffix";
      suffix.textContent = scale.suffix;
      row.appendChild(suffix);
    }
    valueWrap.appendChild(row);
  }

  function renderBooleanWidget(ctx) {
    const { valueWrap, currentDraft, readOnly, setDraftValue, setFieldError } = ctx;
    const switchLabel = document.createElement("label");
    switchLabel.className = "mac-switch";
    const control = document.createElement("input");
    control.type = "checkbox";
    control.checked = !!currentDraft;
    control.disabled = readOnly;
    control.addEventListener("change", () => {
      setDraftValue(!!control.checked);
      setFieldError(false, "");
    });
    const track = document.createElement("span");
    track.className = "mac-switch-track";
    track.appendChild(Object.assign(document.createElement("span"), { className: "mac-switch-thumb" }));
    switchLabel.appendChild(control);
    switchLabel.appendChild(track);
    const toggle = document.createElement("div");
    toggle.className = "toggle setting-toggle";
    toggle.appendChild(switchLabel);
    valueWrap.appendChild(toggle);
  }

  function renderColorWidget(ctx) {
    const { valueWrap, currentDraft, currentValue, readOnly, setDraftValue, setFieldError } = ctx;
    const row = document.createElement("div");
    row.className = "color-row";
    const control = document.createElement("input");
    control.type = "color";
    control.value = colorIntToHex(currentDraft);
    control.disabled = readOnly;
    const numeric = document.createElement("input");
    numeric.type = "number";
    numeric.className = "color-value";
    numeric.value = String(currentDraft);
    numeric.disabled = readOnly;
    control.addEventListener("input", () => {
      const intValue = hexToColorInt(control.value);
      numeric.value = String(intValue);
      setDraftValue(intValue);
      setFieldError(false, "");
    });
    numeric.addEventListener("change", () => {
      const intValue = clampInt(numeric.value, 0, 16777215, currentValue);
      numeric.value = String(intValue);
      control.value = colorIntToHex(intValue);
      setDraftValue(intValue);
      setFieldError(false, "");
    });
    row.appendChild(control);
    row.appendChild(numeric);
    valueWrap.appendChild(row);
  }

  function renderPaletteWidget(ctx) {
    const { valueWrap, currentDraft, readOnly, setDraftValue, setFieldError } = ctx;
    const row = document.createElement("div");
    row.className = "palette-row";
    const palette = Array.isArray(currentDraft) ? currentDraft : [];
    palette.forEach((entry, index) => {
      const box = document.createElement("div");
      box.className = "palette-entry";
      const color = document.createElement("input");
      color.type = "color";
      color.value = colorIntToHex(entry);
      color.disabled = readOnly;
      const value = document.createElement("input");
      value.type = "number";
      value.className = "color-value";
      value.value = String(entry);
      value.disabled = readOnly;
      const updatePalette = (intValue) => {
        const next = palette.slice();
        next[index] = intValue;
        setDraftValue(next);
        setFieldError(false, "");
      };
      color.addEventListener("input", () => {
        const intValue = hexToColorInt(color.value);
        value.value = String(intValue);
        updatePalette(intValue);
      });
      value.addEventListener("change", () => {
        const intValue = clampInt(value.value, 0, 16777215, entry);
        value.value = String(intValue);
        color.value = colorIntToHex(intValue);
        updatePalette(intValue);
      });
      box.appendChild(color);
      box.appendChild(value);
      row.appendChild(box);
    });
    valueWrap.appendChild(row);
  }

  function renderDefaultInputWidget(ctx) {
    const { spec, valueWrap, currentDraft, readOnly, setDraftValue, setFieldError } = ctx;
    const control = document.createElement("input");
    control.type = spec.type === settingType.Float || spec.type === settingType.Integer || spec.type === settingType.PositiveBigInteger
      ? "number"
      : "text";
    if (spec.minimumValue !== undefined) control.min = spec.minimumValue;
    if (spec.maximumValue !== undefined) control.max = spec.maximumValue;
    if (spec.type === settingType.Integer || spec.type === settingType.PositiveBigInteger) control.step = "1";
    control.value = currentDraft ?? "";
    control.readOnly = readOnly;
    control.addEventListener("change", () => {
      const validation = validateFieldValue(spec, control.value);
      if (!validation.valid) {
        setFieldError(true, validation.message);
        return;
      }
      const coerced = coerceFieldValue(spec, control.value);
      setDraftValue(coerced);
      setFieldError(false, "");
    });
    valueWrap.appendChild(control);
  }

  // Resolve a select widget's option list. Sources:
  //   "inline"             - widget.options.values (and optional .labels)
  //   "schemaPath"         - resolve a path against state.unifiedSchema. If the
  //                          target is an array, use it directly. If it's a
  //                          number, generate 1..N (used for compiledMaxChannels).
  //                          Optional widget.options.values/labels provide label
  //                          overrides for specific raw values.
  //   "intlCountryCodes"   - the prebuilt COUNTRY_OPTIONS list
  //   "externalTimeZones"  - the lazily-fetched time zone list
  function getWidgetSelectOptions(spec, widget) {
    const source = (widget.options && widget.options.source) || "inline";
    const options = widget.options || {};
    const values = Array.isArray(options.values) ? options.values : [];
    const labels = Array.isArray(options.labels) ? options.labels : [];

    // Build a value->label map from the parallel values/labels arrays.
    const labelMap = {};
    values.forEach((v, i) => { if (i < labels.length) labelMap[String(v)] = labels[i]; });

    // Apply the label map to a raw array, falling back to the raw value as label.
    function applyLabelMap(rawValues) {
      return rawValues.map((value) => ({
        value: String(value),
        label: Object.prototype.hasOwnProperty.call(labelMap, String(value)) ? labelMap[String(value)] : String(value)
      }));
    }

    if (source === "intlCountryCodes") {
      return COUNTRY_OPTIONS.map((option) => ({ value: option.value, label: option.label }));
    }
    if (source === "externalTimeZones") {
      // The URL is carried on the spec's widget metadata so the UI doesn't
      // bake the document path. ensureTimezonesLoaded() reads the same field.
      return applyLabelMap(getTimeZoneOptions());
    }
    if (source === "schemaPath") {
      const target = readJsonPath(state.unifiedSchema, options.schemaPath);
      if (Array.isArray(target)) {
        return applyLabelMap(target);
      }
      return [];
    }
    // Inline (default)
    return applyLabelMap(values);
  }

  // Linearly remap a raw value into the displayed range, then clamp.
  function mapRawToDisplay(scale, raw) {
    const rawMin = Number(scale.rawMin);
    const rawMax = Number(scale.rawMax);
    const displayMin = Number(scale.displayMin);
    const displayMax = Number(scale.displayMax);
    if (rawMax === rawMin) return displayMin;
    return ((Number(raw) - rawMin) / (rawMax - rawMin)) * (displayMax - displayMin) + displayMin;
  }

  function mapDisplayToRaw(scale, display) {
    const rawMin = Number(scale.rawMin);
    const rawMax = Number(scale.rawMax);
    const displayMin = Number(scale.displayMin);
    const displayMax = Number(scale.displayMax);
    if (displayMax === displayMin) return rawMin;
    return ((Number(display) - displayMin) / (displayMax - displayMin)) * (rawMax - rawMin) + rawMin;
  }

  function clampDisplay(scale, value) {
    return Math.max(Number(scale.displayMin), Math.min(Number(scale.displayMax), Math.round(Number(value))));
  }

  function validateFieldValue(spec, rawValue) {
    if (spec.type === settingType.String) {
      if (!spec.emptyAllowed && rawValue === "") {
        return { valid: false, message: "Empty value is not allowed." };
      }
      return { valid: true, message: "" };
    }

    if (spec.type === settingType.Integer || spec.type === settingType.PositiveBigInteger || spec.type === settingType.Float || spec.type === settingType.Slider) {
      if (rawValue === "") {
        return { valid: false, message: "Value is required." };
      }
      const numeric = Number(rawValue);
      if (Number.isNaN(numeric)) {
        return { valid: false, message: "Value must be numeric." };
      }
      if (spec.minimumValue !== undefined && numeric < spec.minimumValue) {
        return { valid: false, message: `Value must be at least ${spec.minimumValue}.` };
      }
      if (spec.maximumValue !== undefined && numeric > spec.maximumValue) {
        return { valid: false, message: `Value must be at most ${spec.maximumValue}.` };
      }
    }

    return { valid: true, message: "" };
  }

  function coerceFieldValue(spec, rawValue) {
    switch (spec.type) {
      case settingType.Integer:
      case settingType.PositiveBigInteger:
      case settingType.Slider:
        return Math.trunc(Number(rawValue));
      case settingType.Float:
        return Number(rawValue);
      default:
        return rawValue;
    }
  }

  // Resolve display unit metadata for an interval-style spec. Falls back to a
  // 1:1 / "ms" pair when the spec isn't loaded yet, so the hero can still
  // render meaningfully on the very first call.
  function getIntervalDisplayScale(specName) {
    const spec = (state.settingsSpecs || []).find((entry) => entry && entry.name === specName);
    const interval = spec && spec.widget && spec.widget.interval;
    return {
      divisor: Math.max(1, Number(interval && interval.unitDivisor) || 1),
      unitLabel: (interval && interval.unitLabel) || "ms",
      offLabel: interval && interval.offLabel
    };
  }

  function formatIntervalDisplay(rawValue, scale) {
    const display = Math.round(Number(rawValue || 0) / scale.divisor);
    return scale.unitLabel ? `${display} ${scale.unitLabel}` : String(display);
  }

  async function applyEffectInterval() {
    // The standalone "rotate every N seconds" control on the effects tab posts
    // through the same path the full settings form does: it looks up the
    // effectInterval spec, uses its widget metadata to convert displayed
    // units to the raw stored value, and writes via the spec's apiPath.
    const spec = (state.settingsSpecs || []).find((entry) => entry && entry.name === "effectInterval");
    if (!spec) {
      toast("Effect interval spec is not available yet.", "error");
      return;
    }
    const interval = (spec.widget && spec.widget.interval) || {};
    const divisor = Number(interval.unitDivisor) || 1;
    const displayedValue = clampInt(els.effectIntervalInput.value, 0, 2147483, 0);
    const rawValue = displayedValue * divisor;

    try {
      if (spec.apiPath) {
        const body = {};
        writeJsonPath(body, spec.apiPath, rawValue);
        await postJson("/api/v1/settings", body);
      } else {
        await postForm("/settings", { [spec.name]: rawValue });
      }
      // Edit successfully landed; release the input back to auto-refresh.
      state.effectIntervalDirty = false;
      const unit = interval.unitLabel || "";
      toast(`Effect interval set to ${displayedValue} ${unit}.`.trim(), "success");
      await Promise.all([loadEffectsOnly(), loadSettingsOnly()]);
    } catch (error) {
      handleError("Failed to set effect interval", error);
    }
  }

  async function resetDevice(payload) {
    try {
      await postForm("/reset", payload);
      toast("Reset request sent.", "success");
    } catch (error) {
      handleError("Failed to send reset request", error);
    }
  }


  async function moveEffect(effectIndex, newIndex) {
    try {
      await postForm("/moveEffect", { effectIndex, newIndex });
      await loadEffectsOnly();
    } catch (error) {
      handleError("Failed to move effect", error);
    }
  }

  async function deleteEffect(effectIndex) {
    if (!window.confirm("Delete this effect?")) {
      return;
    }
    try {
      await postForm("/deleteEffect", { effectIndex });
      await loadEffectsOnly();
    } catch (error) {
      handleError("Failed to delete effect", error);
    }
  }

  async function applyDeviceSettings(rebootAfter) {
    if (state.deviceErrors.size > 0) {
      toast("Fix invalid settings before applying.", "error");
      return;
    }

    const { legacyPayload, unifiedPayload } = splitDeviceDraftPayloads(state.deviceDraft);
    if (Object.keys(legacyPayload).length === 0 && Object.keys(unifiedPayload).length === 0) {
      toast("No device setting changes to apply.", "success");
      return;
    }

    if (!rebootAfter) {
      const rebootNames = collectRequiresRebootSettings(state.deviceDraft);
      if (rebootNames.length > 0) {
        const friendly = rebootNames.join(", ");
        const noun = rebootNames.length === 1 ? "this setting does" : "these settings do";
        const shouldContinue = window.confirm(
          `Changes to ${friendly} ${noun} not take effect until the device reboots.\n\n` +
          "Click OK to apply the change now and reboot later, or Cancel to use Apply + Reboot instead."
        );
        if (!shouldContinue) {
          return;
        }
      }
    }

    try {
      if (Object.keys(unifiedPayload).length > 0) {
        await postJson("/api/v1/settings", unifiedPayload);
      }
      if (Object.keys(legacyPayload).length > 0) {
        await postForm("/settings", legacyPayload);
      }
      toast("Device settings applied.", "success");
      await loadSettingsOnly();
      if (rebootAfter) {
        await resetDevice({ board: 1 });
      }
    } catch (error) {
      handleError("Failed to apply device settings", error);
    }
  }

  async function openEffectDialog(effectIndex) {
    try {
      const [specs, values] = await Promise.all([
        fetchJson(`/settings/effect/specs?effectIndex=${encodeURIComponent(effectIndex)}`),
        fetchJson(`/settings/effect?effectIndex=${encodeURIComponent(effectIndex)}`)
      ]);
      const effect = state.effects && Array.isArray(state.effects.Effects) ? state.effects.Effects[effectIndex] : null;
      state.effectDialog = {
        open: true,
        effectIndex,
        effectName: effect ? effect.name : `Effect ${effectIndex}`,
        specs,
        values,
        draft: {},
        errors: new Set()
      };
      renderEffectDialogForm();
      if (!els.effectSettingsDialog.open) {
        els.effectSettingsDialog.showModal();
      }
    } catch (error) {
      handleError("Failed to load effect settings", error);
    }
  }

  function closeEffectDialog() {
    state.effectDialog.open = false;
    state.effectDialog.draft = {};
    state.effectDialog.errors = new Set();
    if (els.effectSettingsDialog.open) {
      els.effectSettingsDialog.close();
    }
  }

  async function applyEffectSettings() {
    const dialog = state.effectDialog;
    if (!dialog.open) {
      return;
    }
    if (dialog.errors.size > 0) {
      toast("Fix invalid effect settings before applying.", "error");
      return;
    }
    const payload = { effectIndex: dialog.effectIndex, ...dialog.draft };
    if (Object.keys(dialog.draft).length === 0) {
      closeEffectDialog();
      return;
    }
    try {
      await postForm("/settings/effect", payload);
      toast("Effect settings applied.", "success");
      closeEffectDialog();
      await loadEffectsOnly();
    } catch (error) {
      handleError("Failed to apply effect settings", error);
    }
  }

  function renderStats() {
    const staticStats = state.staticStats;
    const dynamicStats = state.dynamicStats;
    const unifiedSettings = state.unifiedSettings;
    if (!staticStats || !dynamicStats) {
      els.statsGrid.innerHTML = '<div class="empty-cell">Statistics data is unavailable.</div>';
      return;
    }

    const cards = [];
    cards.push(statCard("Output", [
      ["Compiled driver", staticStats.COMPILED_OUTPUT_DRIVER],
      ["Active driver", staticStats.ACTIVE_OUTPUT_DRIVER],
      ["Compiled order", staticStats.COMPILED_WS281X_COLOR_ORDER || "--"],
      ["Active order", staticStats.CONFIGURED_WS281X_COLOR_ORDER || "--"],
      ["Configured", `${staticStats.CONFIGURED_MATRIX_WIDTH}x${staticStats.CONFIGURED_MATRIX_HEIGHT}`],
      ["Active", `${staticStats.ACTIVE_MATRIX_WIDTH}x${staticStats.ACTIVE_MATRIX_HEIGHT}`],
      ["LEDs", `${staticStats.ACTIVE_NUM_LEDS} / ${staticStats.COMPILED_NUM_LEDS}`],
      ["Channels", `${staticStats.ACTIVE_NUM_CHANNELS} / ${staticStats.COMPILED_NUM_CHANNELS}`]
    ]));

    cards.push(statCard("Audio", [
      ["Mode", staticStats.AUDIO_INPUT_MODE],
      ["Configured pin", staticStats.CONFIGURED_AUDIO_INPUT_PIN],
      ["Compiled pin", staticStats.COMPILED_AUDIO_INPUT_PIN],
      ["Audio FPS", formatNumber(dynamicStats.AUDIO_FPS)],
      ["Frames socket", truthy(staticStats.FRAMES_SOCKET)],
      ["Effects socket", truthy(staticStats.EFFECTS_SOCKET)]
    ]));

    cards.push(statCard("CPU", [
      ["Total", `${formatPercent(dynamicStats.CPU_USED)}%`],
      ["Core 0", `${formatPercent(dynamicStats.CPU_USED_CORE0)}%`],
      ["Core 1", `${formatPercent(dynamicStats.CPU_USED_CORE1)}%`],
      ["LED FPS", formatNumber(dynamicStats.LED_FPS)],
      ["Serial FPS", formatNumber(dynamicStats.SERIAL_FPS)]
    ], dynamicStats.CPU_USED));

    cards.push(statCard("Memory", [
      ["Heap free", formatBytes(dynamicStats.HEAP_FREE)],
      ["Heap size", formatBytes(staticStats.HEAP_SIZE)],
      ["DMA free", formatBytes(dynamicStats.DMA_FREE)],
      ["DMA size", formatBytes(staticStats.DMA_SIZE)],
      ["PSRAM free", formatBytes(dynamicStats.PSRAM_FREE)],
      ["PSRAM size", formatBytes(staticStats.PSRAM_SIZE)]
    ], memoryUsagePercent(dynamicStats.HEAP_FREE, staticStats.HEAP_SIZE)));

    cards.push(statCard("Package", [
      ["Chip", staticStats.CHIP_MODEL],
      ["Cores", staticStats.CHIP_CORES],
      ["Clock", `${staticStats.CHIP_SPEED} MHz`],
      ["Program", formatBytes(staticStats.PROG_SIZE)],
      ["Flash", formatBytes(staticStats.FLASH_SIZE)],
      ["Code free", formatBytes(staticStats.CODE_FREE)]
    ]));

    if (unifiedSettings && unifiedSettings.topology && unifiedSettings.outputs && unifiedSettings.device && unifiedSettings.device.audio && unifiedSettings.device.remote) {
      cards.push(statCard("Schema", [
        ["Topology live", truthy(unifiedSettings.topology.liveApply)],
        ["Output live", truthy(unifiedSettings.outputs.liveApply)],
        ["Audio live", truthy(unifiedSettings.device.audio.liveApply)],
        ["Audio reboot", truthy(unifiedSettings.device.audio.requiresReboot)],
        ["Remote enabled", truthy(unifiedSettings.device.remote.enabled)],
        ["Remote pin", unifiedSettings.device.remote.pin]
      ]));
    }

    els.statsGrid.replaceChildren(...cards);
    els.statsTimestamp.textContent = `Updated ${new Date().toLocaleTimeString()}`;
  }

  function statCard(title, rows, meterPercent) {
    const card = document.createElement("div");
    card.className = "stat-card";
    const heading = document.createElement("h3");
    heading.textContent = title;
    card.appendChild(heading);
    if (meterPercent !== undefined) {
      const meter = document.createElement("div");
      meter.className = "meter";
      const fill = document.createElement("span");
      fill.style.width = `${Math.max(0, Math.min(100, meterPercent))}%`;
      meter.appendChild(fill);
      card.appendChild(meter);
    }
    const list = document.createElement("div");
    list.className = "stat-list";
    rows.forEach(([label, value]) => {
      const row = document.createElement("div");
      row.className = "stat-row";
      row.innerHTML = `<span>${escapeHtml(String(label))}</span><strong>${escapeHtml(String(value))}</strong>`;
      list.appendChild(row);
    });
    card.appendChild(list);
    return card;
  }

  function connectPreviewSocket() {
    state.preview.shouldReconnect = true;
    if (state.preview.reconnectTimer) {
      window.clearTimeout(state.preview.reconnectTimer);
      state.preview.reconnectTimer = null;
    }

    if (state.preview.socket) {
      return;
    }

    if (state.staticStats && !state.staticStats.FRAMES_SOCKET) {
      toast("Frame preview socket is not enabled in this build.", "error");
      return;
    }

    const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
    const socket = new WebSocket(`${protocol}//${window.location.host}/ws/frames`);
    socket.binaryType = "arraybuffer";
    socket.onopen = function () {
      state.preview.connected = true;
      state.preview.lastRenderMs = 0;
      if (state.preview.reconnectTimer) {
        window.clearTimeout(state.preview.reconnectTimer);
        state.preview.reconnectTimer = null;
      }
      els.previewStatus.textContent = "Preview connected";
      toast("Frame preview connected.", "success");
      refreshPreviewVisibility();
    };
    socket.onclose = function () {
      state.preview.connected = false;
      state.preview.socket = null;
      state.preview.frame = null;
      state.preview.latestFrame = null;
      state.preview.dirty = false;
      state.preview.lastRenderMs = 0;
      stopPreviewRenderLoop();
      els.previewStatus.textContent = state.preview.shouldReconnect ? "Preview reconnecting..." : "Preview offline";
      refreshPreviewVisibility();
      schedulePreviewReconnect();
    };
    socket.onerror = function () {
      console.warn("Preview socket error");
    };
    socket.onmessage = function (event) {
      try {
        state.preview.latestFrame = new Uint8Array(event.data);
        state.preview.dirty = true;
        refreshPreviewVisibility();
        ensurePreviewRenderLoop();
      } catch (error) {
        handleError("Failed to render preview frame", error);
      }
    };
    state.preview.socket = socket;
  }

  function disconnectPreviewSocket() {
    state.preview.shouldReconnect = false;
    if (state.preview.reconnectTimer) {
      window.clearTimeout(state.preview.reconnectTimer);
      state.preview.reconnectTimer = null;
    }
    if (state.preview.socket) {
      state.preview.socket.close();
      state.preview.socket = null;
    }
    state.preview.connected = false;
    state.preview.frame = null;
    state.preview.latestFrame = null;
    state.preview.dirty = false;
    state.preview.lastRenderMs = 0;
    stopPreviewRenderLoop();
    els.previewStatus.textContent = "Preview offline";
    refreshPreviewVisibility();
  }

  function schedulePreviewReconnect() {
    if (!state.preview.shouldReconnect || state.preview.socket || state.preview.reconnectTimer) {
      return;
    }

    state.preview.reconnectTimer = window.setTimeout(function () {
      state.preview.reconnectTimer = null;
      if (state.preview.shouldReconnect && !state.preview.socket) {
        connectPreviewSocket();
      }
    }, 1000);
  }

  function ensurePreviewRenderLoop() {
    if (state.preview.animationFrameId) {
      return;
    }

    state.preview.animationFrameId = window.requestAnimationFrame(runPreviewRenderLoop);
  }

  function stopPreviewRenderLoop() {
    if (!state.preview.animationFrameId) {
      return;
    }

    window.cancelAnimationFrame(state.preview.animationFrameId);
    state.preview.animationFrameId = 0;
  }

  function runPreviewRenderLoop(timestamp) {
    const frameIntervalMs = 1000 / 30;
    state.preview.animationFrameId = 0;

    if (!state.preview.connected) {
      return;
    }

    if (state.preview.dirty && timestamp - state.preview.lastRenderMs >= frameIntervalMs) {
      state.preview.frame = state.preview.latestFrame;
      state.preview.dirty = false;
      state.preview.lastRenderMs = timestamp;
      drawPreviewFrame();
    }

    if (state.preview.dirty) {
      ensurePreviewRenderLoop();
    }
  }

  function drawPreviewFrame() {
    const frame = state.preview.latestFrame || state.preview.frame;
    const staticStats = state.staticStats;
    const canvas = els.previewCanvas;
    if (!canvas || !frame || !staticStats) {
      return;
    }

    refreshPreviewVisibility();
    const metrics = getPreviewDisplayMetrics();
    const width = metrics.width;
    const height = metrics.height;
    const dpr = window.devicePixelRatio || 1;
    canvas.classList.toggle("preview-canvas-thin", metrics.displayHeight <= 12);
    canvas.style.width = `${metrics.displayWidth}px`;
    canvas.style.height = `${metrics.displayHeight}px`;
    canvas.width = Math.max(1, Math.round(metrics.displayWidth * dpr));
    canvas.height = Math.max(1, Math.round(metrics.displayHeight * dpr));

    const ctx = canvas.getContext("2d");
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, metrics.displayWidth, metrics.displayHeight);
    ctx.imageSmoothingEnabled = false;

    let offset = 0;
    for (let y = 0; y < height; y += 1) {
      const top = y * metrics.pixelHeight;
      for (let x = 0; x < width; x += 1) {
        const red = frame[offset] || 0;
        const green = frame[offset + 1] || 0;
        const blue = frame[offset + 2] || 0;
        offset += 3;
        ctx.fillStyle = `rgb(${red}, ${green}, ${blue})`;
        ctx.fillRect(x * metrics.pixelWidth, top, metrics.pixelWidth, metrics.pixelHeight);
      }
    }
  }

  function getPreviewDisplayMetrics() {
    const staticStats = state.staticStats;
    const canvas = els.previewCanvas;
    if (!canvas || !staticStats) {
      return {
        width: 1,
        height: 1,
        displayWidth: 1,
        displayHeight: 10,
        pixelWidth: 1,
        pixelHeight: 10
      };
    }

    const width = Number(staticStats.ACTIVE_MATRIX_WIDTH || staticStats.CONFIGURED_MATRIX_WIDTH || 1);
    const height = Number(staticStats.ACTIVE_MATRIX_HEIGHT || staticStats.CONFIGURED_MATRIX_HEIGHT || 1);
    const parent = canvas.parentElement;
    let parentContentWidth = 0;
    if (parent) {
      const ps = getComputedStyle(parent);
      parentContentWidth = parent.clientWidth - parseFloat(ps.paddingLeft) - parseFloat(ps.paddingRight);
    }
    const availableWidth = Math.max(1, parentContentWidth || width);
    const pixelWidth = availableWidth / Math.max(1, width);
    const pixelHeight = Math.max(pixelWidth, 10);
    return {
      width,
      height,
      displayWidth: Math.max(1, Math.round(availableWidth)),
      displayHeight: Math.max(1, Math.round(height * pixelHeight)),
      pixelWidth,
      pixelHeight
    };
  }

  function refreshPreviewVisibility() {
    const currentFrame = state.preview.latestFrame || state.preview.frame;
    const hasFrame = !!(state.preview.connected && currentFrame && currentFrame.length > 0);
    if (els.previewWrap) {
      els.previewWrap.classList.toggle("is-empty", !hasFrame);
    }
  }

  function miniIconButton(label, title, handler, disabled) {
    const button = document.createElement("button");
    button.type = "button";
    button.className = "mini-button mini-icon-button";
    button.textContent = label;
    button.title = title;
    button.setAttribute("aria-label", title);
    button.disabled = !!disabled;
    button.addEventListener("click", handler);
    return button;
  }

  async function fetchJson(path, options) {
    const response = await fetch(path, options);
    if (!response.ok) {
      throw await buildHttpError(response);
    }
    return parseJsonResponse(response);
  }

  async function postJson(path, payload) {
    const response = await fetch(path, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload || {})
    });
    if (!response.ok) {
      throw await buildHttpError(response);
    }
    const contentType = response.headers.get("content-type") || "";
    if (contentType.includes("application/json") || contentType.includes("text/json")) {
      return parseJsonResponse(response);
    }
    return null;
  }

  async function postForm(path, payload) {
    const body = new URLSearchParams();
    Object.entries(payload || {}).forEach(([key, value]) => {
      if (value === undefined || value === null) {
        return;
      }
      if (Array.isArray(value)) {
        body.append(key, JSON.stringify(value));
        return;
      }
      if (typeof value === "object") {
        body.append(key, JSON.stringify(value));
        return;
      }
      body.append(key, String(value));
    });
    const response = await fetch(path, {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8" },
      body
    });
    if (!response.ok) {
      throw await buildHttpError(response);
    }
    const contentType = response.headers.get("content-type") || "";
    if (contentType.includes("application/json") || contentType.includes("text/json")) {
      return parseJsonResponse(response);
    }
    return null;
  }

  async function parseJsonResponse(response) {
    const text = await response.text();
    const sanitized = text.replace(/\u0000+$/g, "").trim();
    return JSON.parse(sanitized);
  }

  async function buildHttpError(response) {
    let message = `HTTP ${response.status}`;
    try {
      const contentType = response.headers.get("content-type") || "";
      if (contentType.includes("json")) {
        const payload = await response.json();
        if (payload && payload.message) {
          message = payload.message;
        }
      } else {
        const text = await response.text();
        if (text) {
          message = text;
        }
      }
    } catch (_error) {
      // Ignore parse failure
    }
    return new Error(message);
  }

  function toast(message, level) {
    const node = document.createElement("div");
    node.className = `toast ${level || ""}`.trim();
    node.textContent = message;
    els.toastStack.appendChild(node);
    window.setTimeout(() => node.remove(), 4200);
  }

  function handleError(context, error) {
    const message = error && error.message ? error.message : String(context || "Unexpected error");
    console.error(context, error);
    toast(context ? `${context}: ${message}` : message, "error");
  }

  function clampInt(value, min, max, fallback) {
    const numeric = Number(value);
    if (Number.isNaN(numeric)) {
      return fallback;
    }
    return Math.max(min, Math.min(max, Math.trunc(numeric)));
  }

  function formatNumber(value) {
    return Number(value || 0).toFixed(0);
  }

  function formatPercent(value) {
    return Number(value || 0).toFixed(1);
  }

  function formatBytes(bytes) {
    const value = Number(bytes || 0);
    if (value >= 1024 * 1024) {
      return `${(value / (1024 * 1024)).toFixed(1)} MB`;
    }
    if (value >= 1024) {
      return `${(value / 1024).toFixed(1)} KB`;
    }
    return `${value} B`;
  }

  function memoryUsagePercent(free, size) {
    const total = Number(size || 0);
    if (!total) {
      return 0;
    }
    const used = total - Number(free || 0);
    return (used / total) * 100;
  }

  function truthy(value) {
    return value ? "Yes" : "No";
  }

  function valuesEqual(left, right) {
    return JSON.stringify(left) === JSON.stringify(right);
  }

  function defaultValueForSpec(spec) {
    if (spec.type === settingType.Boolean) {
      return false;
    }
    if (spec.type === settingType.Palette) {
      return [];
    }
    if (spec.type === settingType.Color) {
      return 0;
    }
    return "";
  }

  function getTimeZoneOptions() {
    const options = normalizeTimeZoneOptions(state.timezones);
    if (options.length > 0) {
      return options;
    }

    const currentTimeZone = state.settings && state.settings.timeZone ? state.settings.timeZone : "";
    return currentTimeZone ? [currentTimeZone] : [];
  }

  function normalizeTimeZoneOptions(source) {
    if (!source) {
      return [];
    }

    let values = [];
    if (Array.isArray(source)) {
      values = source.slice();
    } else if (Array.isArray(source.timezones)) {
      values = source.timezones.slice();
    } else if (typeof source === "object") {
      values = Object.keys(source);
    }

    return values
      .filter((value) => typeof value === "string" && value.length > 0)
      .filter((value) => value.includes("/") || value === "UTC" || value.startsWith("Etc/"))
      .sort((left, right) => left.localeCompare(right));
  }

  // Return the friendly names of any drafted settings whose spec is flagged
  // RequiresReboot, so the apply flow can warn the user before a non-reboot
  // commit. Returns [] when nothing in the draft needs a reboot.
  function collectRequiresRebootSettings(draft) {
    const specsByName = new Map((state.settingsSpecs || []).map((spec) => [spec.name, spec]));
    return Object.keys(draft)
      .map((name) => specsByName.get(name))
      .filter((spec) => spec && spec.requiresReboot)
      .map((spec) => spec.friendlyName || spec.name);
  }

  // Split a device-settings draft into the right wire shape per setting:
  // any setting whose spec carries an apiPath becomes a write at that path
  // inside the unified-settings JSON document; everything else stays in the
  // flat legacy payload keyed by name. Pin paths like "outputs.ws281x.pins[0]"
  // are merged with the chip's currently-configured pin array so partial
  // updates don't blank out untouched channels.
  function splitDeviceDraftPayloads(draft) {
    const legacyPayload = {};
    const unifiedPayload = {};
    const specsByName = new Map((state.settingsSpecs || []).map((spec) => [spec.name, spec]));
    const seedPaths = new Map();           // path prefix -> seeded base array

    Object.keys(draft).forEach((key) => {
      const spec = specsByName.get(key);
      if (spec && spec.apiPath) {
        seedDraftPathContext(spec.apiPath, unifiedPayload, seedPaths);
        writeJsonPath(unifiedPayload, spec.apiPath, draft[key]);
      } else {
        legacyPayload[key] = draft[key];
      }
    });

    return { legacyPayload, unifiedPayload };
  }

  // For paths that target an array element (e.g. outputs.ws281x.pins[2]) we
  // need to seed the array with the chip's current full contents the first
  // time we touch the array, otherwise serializing { pins: [, , 7] } would
  // clear pins 0 and 1. Once seeded, subsequent index writes layer on top.
  function seedDraftPathContext(apiPath, unifiedPayload, seedPaths) {
    const segments = parseJsonPath(apiPath);
    let pathSoFar = "";
    let cursor = unifiedPayload;
    for (let i = 0; i < segments.length - 1; i += 1) {
      const segment = segments[i];
      pathSoFar = pathSoFar.length === 0 ? String(segment) : `${pathSoFar}.${segment}`;
      const nextIsIndex = typeof segments[i + 1] === "number";
      if (nextIsIndex && !seedPaths.has(pathSoFar)) {
        const seedValue = readJsonPath(state.unifiedSettings, pathSoFar);
        if (Array.isArray(seedValue)) {
          cursor[segment] = seedValue.slice();
          seedPaths.set(pathSoFar, true);
        }
      } else if (cursor[segment] == null) {
        cursor[segment] = nextIsIndex ? [] : {};
      }
      cursor = cursor[segment];
    }
  }

  function colorIntToHex(value) {
    const numeric = clampInt(value, 0, 16777215, 0);
    return `#${numeric.toString(16).padStart(6, "0")}`;
  }

  function hexToColorInt(value) {
    return parseInt(String(value || "#000000").replace("#", ""), 16) || 0;
  }

  function escapeHtml(value) {
    return String(value)
      .replaceAll("&", "&amp;")
      .replaceAll("<", "&lt;")
      .replaceAll(">", "&gt;")
      .replaceAll('"', "&quot;")
      .replaceAll("'", "&#39;");
  }

  function buildCountryOptions() {
    const options = [];
    let displayNames = null;

    if (typeof Intl !== "undefined" && typeof Intl.DisplayNames === "function") {
      try {
        displayNames = new Intl.DisplayNames(["en"], { type: "region" });
      } catch (_error) {
        displayNames = null;
      }
    }

    for (let first = 65; first <= 90; first += 1) {
      for (let second = 65; second <= 90; second += 1) {
        const code = String.fromCharCode(first) + String.fromCharCode(second);
        let label = code;
        if (displayNames) {
          try {
            label = displayNames.of(code) || code;
          } catch (_error) {
            label = code;
          }
        }
        if (label === code) {
          continue;
        }
        options.push({ value: code, label: `${code} — ${label}` });
      }
    }

    return options.sort((left, right) => left.label.localeCompare(right.label));
  }

  // Resolve the URL of any spec advertising itself as the external timezone
  // source. The location of the document is firmware-defined — the UI only
  // knows the result is parseable by normalizeTimeZoneOptions().
  function findExternalTimeZonesUrl() {
    if (!Array.isArray(state.settingsSpecs)) {
      return null;
    }
    const spec = state.settingsSpecs.find((entry) => {
      const widget = entry && entry.widget;
      const options = widget && widget.options;
      return options && options.source === "externalTimeZones" && typeof options.url === "string" && options.url.length > 0;
    });
    return spec ? spec.widget.options.url : null;
  }

  async function ensureTimezonesLoaded() {
    if (state.timezones || state.timezonesLoading) {
      return;
    }

    const url = findExternalTimeZonesUrl();
    if (!url) {
      return;
    }

    state.timezonesLoading = true;
    try {
      state.timezones = await fetchJson(url);
      if (state.settings && state.settingsSpecs) {
        renderSettingsForm();
      }
    } catch (error) {
      handleError("Failed to load timezones", error);
    } finally {
      state.timezonesLoading = false;
    }
  }
})();
