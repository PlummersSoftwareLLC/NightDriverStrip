# On-board REST-like API <!-- omit in toc -->

## Table of Contents <!-- omit in toc -->

- [Introduction](#introduction)
- [Endpoints](#endpoints)
  - [Get effect list information](#get-effect-list-information)
  - [Set current effect](#set-current-effect)
  - [Next effect](#next-effect)
  - [Previous effect](#previous-effect)
  - [Disable effect](#disable-effect)
  - [Enable effect](#enable-effect)
  - [Move effect](#move-effect)
  - [Copy effect](#copy-effect)
  - [Delete effect](#delete-effect)
  - [Get effect configuration information](#get-effect-configuration-information)
  - [Get device statistics](#get-device-statistics)
  - [Get device setting specifications](#get-device-setting-specifications)
  - [Device settings](#device-settings)
  - [Set setting with validation](#set-setting-with-validation)
  - [Unified device settings](#unified-device-settings)
  - [Get effect setting specifications](#get-effect-setting-specifications)
  - [Effect settings](#effect-settings)
  - [Reset configuration and/or device](#reset-configuration-andor-device)
- [Postman collection](#postman-collection)
- [WebSockets](#websockets)
  - [Effect events](#effect-events)
  - [Color data](#color-data)

## Introduction

On devices with WiFi and the webserver enabled, NightDriverStrip publishes a REST-like API. The API includes a number of endpoints to:

- Retrieve information about the device and the effects it runs
- Change the effect that's running
- Disable and enable effects
- Move, copy and delete effects
- Retrieve setting specifications
- Retrieve and change settings
- Reset configuration and/or the device itself

A subset of the endpoints is used by the NightDriverStrip [web UI](site/README.md).

In the sections below:

- Parameters for GET requests are query parameters
- Parameters for POST requests are HTTP POST body key/value pairs
- Boolean parameters are considered to be true if their value is the text `true` (lower-case only) or any whole number other than 0 (zero).

Besides a REST-like API with endpoints that are to be called by the client, the webserver can also be configured to publish one or two WebSockets. These are discussed at the end of this document.

## Endpoints

### Get effect list information

This endpoint returns a JSON document with basic information about the effects on the device.

| Property | Value | Explanation |
| - | - | - |
| URL | `/effects` | |
| Method | GET | |
| Parameters | | |
| Response | 200 (OK) | A JSON blob with information about the device's effect list. The zero-based effect indexes used in other endpoints correspond with the indexes in this list. |

### Set current effect

This endpoint can be used to set the effect that the device is currently showing.

| Property | Value | Explanation |
| - | - | - |
| URL | `/currentEffect` | |
| Method | POST | |
| Parameters | `currentEffectIndex` | The (zero-based) integer index of the effect to activate in the device's effect list. |
| Response | 200 (OK) | An empty OK response. |

### Next effect

This endpoint can be used to activate the next effect.

| Property | Value | Explanation |
| - | - | - |
| URL | `/nextEffect` | |
| Method | POST | |
| Parameters | | |
| Response | 200 (OK) | An empty OK response. |

### Previous effect

This endpoint can be used to activate the previous effect.

| Property | Value | Explanation |
| - | - | - |
| URL | `/previousEffect` | |
| Method | POST | |
| Parameters | | |
| Response | 200 (OK) | An empty OK response. |

### Disable effect

With this endpoint an effect can be disabled. This means it will no longer be activated, and skipped when it's its turn to be shown.

| Property | Value | Explanation |
| - | - | - |
| URL | `/disableEffect` | |
| Method | POST | |
| Parameters | `effectIndex` | The (zero-based) integer index of the effect to disable in the device's effect list. |
| Response | 200 (OK) | An empty OK response. |

### Enable effect

With this endpoint a previously disabled effect can be enabled. From that moment on, it will once again be shown.

| Property | Value | Explanation |
| - | - | - |
| URL | `/enableEffect` | |
| Method | POST | |
| Parameters | `effectIndex` | The (zero-based) integer index of the effect to enable in the device's effect list. |
| Response | 200 (OK) | An empty OK response. |

### Move effect

With this endpoint an effect can be moved within the effect list, changing its place in the effect visualisation cycle.

| Property | Value | Explanation |
| - | - | - |
| URL | `/moveEffect` | |
| Method | POST | |
| Parameters | `effectIndex` | The (zero-based) integer index of the effect to move in the device's effect list. |
| | `newIndex` | The (zero-based) integer index of the place in the device's effect list the effect should be moved to. |
| Response | 200 (OK) | An empty OK response. |

### Copy effect

With this endpoint an effect in the effect list can be copied. The created copy will be added to the end of the effect list. While copying the effect, supported effect settings on the copy can be set within the same call.

| Property | Value | Explanation |
| - | - | - |
| URL | `/copyEffect` | |
| Method | POST | |
| Parameters | `effectIndex` | The (zero-based) integer index of the effect of which a copy should be made. |
| | | Zero, one or more settings that have been returned by the [Get effect setting specifications endpoint](#get-effect-setting-specifications); also refer to the [Change effect settings endpoint](#change-effect-settings) for more information. |
| Response | 200 (OK) | A JSON blob with the values for the copied effect's configuration settings, after applying the values in the request's POST parameters. |

### Delete effect

With this endpoint an effect from the effect list can be deleted. Only effects that are copies of the default set (see [Copy effect](#copy-effect)) can be deleted.

| Property | Value | Explanation |
| - | - | - |
| URL | `/deleteEffect` | |
| Method | POST | |
| Parameters | `effectIndex` | The (zero-based) integer index of the effect that should be deleted from the effect list. |
| Response | 200 (OK) | An empty OK response if the effect was successfully deleted or the `effectIndex` was out of bounds. |
| | 400 (Bad Request) | `effectIndex` points to an effect in the default set, that being an effect marked as `"core": true` in the output of the [Get effect list endpoint](#get-effect-list-information). |

### Get effect configuration information

This endpoint returns a JSON document with information about the detailed configuration of the effects on the device. Note that this document currently has an internal purpose, and is as such not optimized for human inspection.

| Property | Value | Explanation |
| - | - | - |
| URL | `/effectsConfig` | |
| Method | GET | |
| Parameters | | |
| Response | 200 (OK) | A JSON blob with detailed configuration information about the device's effects. |

### Get device statistics

This set of endpoints can be used to retrieve device statistics from the device.

#### Static values

| Property | Value | Explanation |
| - | - | - |
| URL | `/statistics/static` | |
| Method | GET | |
| Parameters | | |
| Response | 200 (OK) | A JSON blob with those device statistics that don't change after device initialization. This includes things like the chip model and number of cores. |

#### Dynamic values

| Property | Value | Explanation |
| - | - | - |
| URL | `/statistics/dynamic` | |
| Method | GET | |
| Parameters | | |
| Response | 200 (OK) | A JSON blob with those device statistics that change as the device runs. This includes things like CPU load and memory usage. |

#### All values

| Property | Value | Explanation |
| - | - | - |
| URL | `/statistics` | |
| Method | GET | |
| Parameters | | |
| Response | 200 (OK) | A JSON blob with all device statistics, i.e. the combination of the static and dynamic ones. |

### Get device setting specifications

This endpoint can be used to retrieve the list of known device configuration settings.

| Property | Value | Explanation |
| - | - | - |
| URL | `/settings/specs` | |
| Method | GET | |
| Parameters | | |
| Response | 200 (OK) | A JSON array with the known device configuration settings. The specifications include for each setting the name, description, type identifier, type name, if validation is available, and lower and upper value boundaries if applicable. |

### Device settings

This endpoint can be used to retrieve and change device configuration settings.

When retrieving settings, sensitive properties (like API keys) are not returned.

When changing settings:

- All parameters are optional. Only settings that are sent in the POST parameters are modified. All other settings are unchanged.
- No validation of values provided takes place. There is a [separate endpoint with which an individual setting can be changed after validation](#set-setting-with-validation).
- Changed settings will be applied immediately, but it takes a few seconds before they are persisted beyond restarts.

#### Retrieve device settings

| Property | Value | Explanation |
| - | - | - |
| URL | `/settings` | |
| Method | GET | |
| Parameters | | |
| Response | 200 (OK) | A JSON blob with the current values for the device's configuration settings. |

#### Change device settings

| Property | Value | Explanation |
| - | - | - |
| URL | `/settings` | |
| Method | POST | |
| Parameters | | One or more settings that have been returned by the [Get device setting specifications endpoint](#get-device-setting-specifications). |
| Response | 200 (OK) | A JSON blob with the current values for the device's configuration settings, after applying the values in the request's POST parameters. |

### Set setting with validation

This endpoint can be used to validate the value for one device configuration setting, and set it if validation succeeds. Note that this endpoint will accept only one of the known configuration settings (listed as the parameters of [the Device settings endpoint](#device-settings).)

Note that validation is not implemented for all settings; the validation step is skipped for settings for which validation is not available.

| Property | Value | Explanation |
| - | - | - |
| URL | `/settings/validated` | |
| Method | POST | |
| Parameters | | Exactly one setting that has been returned by the [Get device setting specifications endpoint](#get-device-setting-specifications). |
| Response | 200 (OK) | Validation succeeded and the provided value has been set. |
| | 400 (Bad Request) | More than one known setting was provided, or validation failed. The applicable message is returned in a JSON blob. |

### Unified device settings

This set of endpoints provides a structured, grouped interface for retrieving and changing device configuration settings. Unlike the [Device settings](#device-settings) endpoint, which uses a flat key/value format, the unified settings endpoints organize values into logical groups (`device`, `topology`, `outputs`, `effects`). They also expose runtime applicability information alongside each group, indicating whether a change can be applied live or requires a reboot.

#### Get unified device settings schema

This endpoint returns compile-time constraints and metadata for the unified settings. It can be used to understand what values and options are valid for the current firmware build before constructing a change request.

| Property | Value | Explanation |
| - | - | - |
| URL | `/api/v1/settings/schema` | |
| Method | GET | |
| Parameters | | |
| Response | 200 (OK) | A JSON blob with compile-time constraints and schema information for the unified settings. Includes topology limits, allowed output drivers, WS281x options (allowed channel counts, color orders, and compiled pin assignments), audio configuration, remote control availability, and a section catalog that can be used by a UI to group settings into labeled categories. |

#### Retrieve unified device settings

| Property | Value | Explanation |
| - | - | - |
| URL | `/api/v1/settings` | |
| Method | GET | |
| Parameters | | |
| Response | 200 (OK) | A JSON blob with the current unified device settings, organized into `device`, `topology`, `outputs`, and `effects` groups. Read-only informational fields (such as compiled defaults and live-apply flags) are included alongside the mutable values. |

#### Change unified device settings

All fields in the request body are optional. Only the values that are included are changed; everything else stays as-is.

Changes to `topology` and `outputs` are applied as a transaction: the new configuration is validated, applied live if the device supports it, and only persisted once live application succeeds. If live application fails, the previous configuration is restored and an error is returned.

For groups where the response (or schema) reports `liveApply: false`, changes to that group require a reboot before they take effect.

| Property | Value | Explanation |
| - | - | - |
| URL | `/api/v1/settings` | |
| Method | POST | |
| Body | JSON object | |
| Parameters | `topology.width` | Matrix width. |
| | `topology.height` | Matrix height. |
| | `topology.serpentine` | Whether the LED matrix is wired in serpentine order. |
| | `outputs.driver` | Output driver name. Must be one of the values from `allowedDrivers` in the schema response. |
| | `outputs.ws281x.channelCount` | Number of active WS281x channels. |
| | `outputs.ws281x.colorOrder` | WS281x color component order. Must be one of the values from `allowedColorOrders` in the schema response. |
| | `outputs.ws281x.pins` | Array of GPIO pin numbers, one per WS281x channel. |
| | `device.hostname` | Device hostname. |
| | `device.location` | Device location (city name or ZIP code). |
| | `device.locationIsZip` | Whether `location` is a ZIP code (`true`/`false`). |
| | `device.countryCode` | ISO country code for the device's location. |
| | `device.timeZone` | IANA time zone name. |
| | `device.use24HourClock` | Whether to display time using a 24-hour clock (`true`/`false`). |
| | `device.useCelsius` | Whether to display temperatures in Celsius (`true`/`false`). |
| | `device.ntpServer` | NTP server hostname. |
| | `device.rememberCurrentEffect` | Whether to resume the last-active effect on reboot (`true`/`false`). |
| | `device.powerLimit` | Power consumption limit in milliwatts. |
| | `device.brightness` | LED brightness (0–255). |
| | `device.audioInputPin` or `device.audio.inputPin` | GPIO pin number for the audio input. Both fields are accepted; providing conflicting values in the same request is an error. |
| | `device.globalColor` | Global LED color override, as a 24-bit integer. |
| | `device.secondColor` | Secondary LED color, as a 24-bit integer. |
| | `device.applyGlobalColors` | Whether the global/secondary color override is active (`true`/`false`). |
| | `device.clearGlobalColor` | Set to `true` to clear the current global color override. |
| | `effects.effectInterval` | Duration in milliseconds that each effect runs before the next is activated. |
| Response | 200 (OK) | A JSON blob with the current unified device settings after applying the changes in the request, in the same format as the GET response. |
| | 400 (Bad Request) | Validation failed for one or more of the provided values. The applicable message is returned in a JSON blob. |

### Get effect setting specifications

This endpoint can be used to retrieve the list of known effect-specific configuration settings for an individual effect.

| Property | Value | Explanation |
| - | - | - |
| URL | `/settings/effect/specs` | |
| Method | GET | |
| Parameters | `effectIndex` | The (zero-based) integer index in the device's effect list of the effect to retrieve the setting specifications for. |
| Response | 200 (OK) | A JSON array with the known effect-specific configuration settings for the effect with index `effectIndex`. The specifications include for each setting the name, description, type identifier, type name, if validation is available, and lower and upper value boundaries if applicable. |

### Effect settings

This endpoint can be used to retrieve and change effect-specific configuration settings.

When retrieving settings, sensitive properties (like API keys) are not returned.

When changing settings:

- All parameters are optional. Only settings that are sent in the POST parameters are modified. All other settings are unchanged.
- No validation of values provided takes place, and isn't currently supported for effect-specific settings.
- Changed settings will be applied immediately, but it takes a few seconds before they are persisted beyond restarts.

#### Retrieve effect settings

| Property | Value | Explanation |
| - | - | - |
| URL | `/settings/effect` | |
| Method | GET | |
| Parameters | `effectIndex` | The (zero-based) integer index in the device's effect list of the effect to retrieve the settings for. |
| Response | 200 (OK) | A JSON blob with the current values for the effect's configuration settings. |

#### Change effect settings

| Property | Value | Explanation |
| - | - | - |
| URL | `/settings/effect` | |
| Method | POST | |
| Parameters | `effectIndex` | The (zero-based) integer index in the device's effect list of the effect to change settings for. |
| | | One or more settings that have been returned by the [Get effect setting specifications endpoint](#get-effect-setting-specifications). |
| Response | 200 (OK) | A JSON blob with the current values for the effect's configuration settings, after applying the values in the request's POST parameters. |

### Reset configuration and/or device

This endpoint can be used to reset effect configuration (see [Get effect configuration information](#get-effect-configuration-information)), device settings (see [Device settings](#device-settings)), and/or the board itself - i.e. restart it.

Any parameters that are not provided are considered to be `false`.

| Property | Value | Explanation |
| - | - | - |
| URL | `/reset` | |
| Method | POST | |
| Parameters | `deviceConfig` | A boolean value indicating if device settings should be reset to defaults (`true`/1) or not (`false`/0). |
| | `effectsConfig` | A boolean value indicating if effect configuration information should be reset to defaults (`true`/1) or not (`false`/0). |
| | `board` | A boolean value indicating if the device should be restarted (`true`/1) or not (`false`/0). |
| Response | 200 (OK) | An empty OK response. |

## Postman collection

To aid in the use and testing of the endpoints discussed in this document - and particularly those not used by the NightDriverStrip web UI - a [Postman collection file](tools/NightDriverStrip.postman_collection.json) has been provided.

It can be used with the Postman API Client, a free version of which can be [downloaded from the Postman website](https://www.postman.com/downloads/).

## WebSockets

### Effect events

This WebSocket pushes events when certain updates to the effects list take place. This facilitates a more accurate way of showing the current state of the effects set than by periodic polling of the effects endpoints. For one, changes to the active effect and/or the effect interval that are triggered by the IR remote control will be communicated to clients through WebSocket events, where these are invisible between polling calls to clients that rely solely on that.

The WebSocket endpoint is: `/ws/effects`

<!-- markdownlint-disable MD033 -->
The payload of the textual event message is a small JSON object with one property, which depends on the event. <br>This is detailed in the following table.

| Event | Property | Value |
| - | - | - |
| Current/active effect changed | `currentEffectIndex` | The zero-based index of the effect that is now active, with regards to the effect list returned by the [`/effects` endpoint](#get-effect-list-information). |
| Effect list contents have changed | `effectListDirty` | The contents of the effects list as returned by the [`/effects` endpoint](#get-effect-list-information) have changed in a way that warrant a reload of that list. Acting on later webSocket events without reloading the effects list may lead to a misrepresentation of the actual status. |
| Enabled state for an effect has changed | `effectsEnabledState` | The property will contain an array with one entry. That entry is a JSON object with two properties:<br>- `index`: the zero-based index of the effect of which the enabled state has changed<br>- `enabled`: boolean that indicates if the effect is enabled (`true`) or disabled (`false`) |
| "Next effect" interval has changed | `interval` | The duration that an effect will be active before the device proceeds to the next enabled effect in the effects list, in seconds. |
<!-- markdownlint-enable MD033 -->

### Color data

This WebSocket pushes color data (frame) packets when the active effect's LED display is updated. To be more precise, it pushes a packet every time the "regular"/TCP color data server pushes out a packet. To be _even_ more precise, it pushes a packet every time that happens, and the WebSocket is ready to process a new outgoing packet.
In practice, this means a packet may be sent between a few times per second, up to a framerate that's close to that of the regular color data server.

The WebSocket endpoint is: `/ws/effectframes`

<!-- markdownlint-disable MD033 -->
The payload of the binary event message is an array of RGB color value byte triples, one per LED in the matrix. It matches the `colors` member of the `ColorDataPacket` C++ class, as declared in ledviewer.h and used in the color data server implementation in network.cpp (the `ColorDataTaskEntry()` function, to be specific). <br>Please refer to the source code files mentioned for more information.
<!-- markdownlint-enable MD033 -->
