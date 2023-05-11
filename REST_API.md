# On-board REST-like API

## Introduction

On devices with WiFi and the webserver enabled, NightDriverStrip publishes a REST-like API. The API includes a number of endpoints to:

- Restrieve information about the device and the effects it runs
- Changing the effect that's running
- Disable and enable effects
- Retrieve and change settings

A subset of the endpoints is used by the NightDriverStrip [web UI](site/README.md).

In the sections below:

- Parameters for GET requests are query parameters
- Parameters for POST requests are HTTP POST body key/value pairs
- Boolean parameters are considered to be true if their value is the text `true` (lower-case only) or any whole number other than 0 (zero).

## Endpoints

### Get effect list information

This endpoint returns a JSON document with basic information about the effects on the device.

| Property| Value | Explanation |
|-|-|-|
| URL | `/effects` |
| Method | GET | |
| Parameters | | |
| Response | 200 (OK) | A JSON blob with information about the device's effect list. The zero-based effect indexes used in other endpoints correspond with the indexes in this list. |

### Set current effect

This endpoint can be used to set the effect that the device is currently showing.

| Property| Value | Explanation |
|-|-|-|
| URL | `/currentEffect` | |
| Method | POST | |
| Parameters | `currentEffectIndex` | The (zero-based) integer index of the effect to activate in the device's effect list. |
| Response | 200 (OK) | An empty OK response. |

### Next effect

This endpoint can be used to activate the next effect.

| Property| Value | Explanation |
|-|-|-|
| URL | `/nextEffect` | |
| Method | POST | |
| Parameters | | |
| Response | 200 (OK) | An empty OK response. |

### Previous effect

This endpoint can be used to activate the previous effect.

| Property| Value | Explanation |
|-|-|-|
| URL | `/previousEffect` | |
| Method | POST | |
| Parameters | | |
| Response | 200 (OK) | An empty OK response. |

### Disable effect

With this endpoint an effect can be disabled. This means it will no longer be activated, and skipped when it's its turn to be shown.

| Property| Value | Explanation |
|-|-|-|
| URL | `/disableEffect` | |
| Method | POST | |
| Parameters | `effectIndex` | The (zero-based) integer index of the effect to disable in the device's effect list. |
| Response | 200 (OK) | An empty OK response. |

### Enable effect

With this endpoint a previously disabled effect can be enabled. From that moment on, it will once again be shown.

| Property| Value | Explanation |
|-|-|-|
| URL | `/enableEffect` |
| Method | POST | |
| Parameters | `effectIndex` | the (zero-based) integer index of the effect to enable in the device's effect list. |
| Response | 200 (OK) | An empty OK response. |

### Get effect configuration information

This endpoint returns a JSON document with information about the detailed configuration of the effects on the device. Note that this document currently has an internal purpose, and is as such not optimized for human inspection.

| Property| Value | Explanation |
|-|-|-|
| URL | `/effectsConfig` |
| Method | GET | |
| Parameters | | |
| Response | 200 (OK) | A JSON blob with detailed configuration information about the device's effects. |

### Settings

This endpoint can be used to retrieve and change device configuration settings.

When retrieving settings, sensitive properties (like API keys) are not returned.

When changing settings:

- All parameters are optional. Only settings that are sent in the POST parameters are modified. All other settings are unchanged.
- No validation of values provided takes place. There is a [separate endpoint with which an individual setting can be changed after validation](#set-setting-with-validation).

#### Retrieve

| Property| Value | Explanation |
|-|-|-|
| URL | `/settings` |
| Method | GET | |
| Parameters | | |
| Response | 200 (OK) | A JSON blob with the current values for the device's configuration settings. |

#### Change

| Property| Value | Explanation |
|-|-|-|
| URL | `/settings` |
| Method | POST | |
| Parameters | `countryCode` | The [ISO 3166-1 alpha-2](https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2) country code for the country that the device is located. Used by the Weather effect. |
| | `location` | The location (city or postal code) where the device is located. Used by the Weather effect. |
| | `locationIsZip` | A boolean indicating if the value in the `location` setting is a postal code (`true`/1) or not (`false`/0). |
| | `openWeatherApiKey` | The API key for the [Weather API provided by Open Weather Map](https://openweathermap.org/api). Used by the Weather effect. |
| | `timeZone` | The timezone the device resides in, in [tz database](https://en.wikipedia.org/wiki/Tz_database) format. The list of available timezone identifiers can be found in the [timezones.json](config/timezones.json) file. Used by effects that show a clock. |
| | `use24HourClock` | A boolean that indicates if time should be shown in 24-hour format (`true`/1) or 12-hour AM/PM format (`false`/0). Used by effects that show a digital clock. |
| | `useCelsius` | A boolean that indicates if temperatures should be shown in degrees Celsius (`true`/1) or degrees Fahrenheit ( `false`/0). Used by the Weather effect. |
| | `youtubeChannelGuid` | The [YouTube Sight](http://tools.tastethecode.com/youtube-sight) GUID of the channel for which the Subscriber effect should show subscriber information. |
| | `youtubeChannelName1` | The name of the channel for which the Subscriber effect should show subscriber information. |
| Response | 200 (OK) | A JSON blob with the current values for the device's configuration settings, after applying the values in the request's POST parameters. |

### Set setting with validation

This endpoint can be used to validate the value for one device configuration setting, and set it if validation succeeds. Note that this endpoint will accept only one of the known configuration settings (listed as the parameters of [the Settings endpoint](#settings).)

Note that validation is not implemented for all settings; the validation step is skipped for settings for which validation is not available.

| Property| Value | Explanation |
|-|-|-|
| URL | `/settings/validated` |
| Method | POST | |
| Parameters | | Exactly one of the known settings may be provided. The known settings are listed as the parameters of [the Settings endpoint](#settings). |
| Response | 200 (OK) | Validation succeeded and the provided value has been set. |
| | 400 (Bad Request) | More than one known setting was provided, or validation failed. The applicable message is returned in a JSON blob. |

### Reset configuration and/or device

This endpoint can be used to reset effect configuration (see [Get effect configuration information](#get-effect-configuration-information)), device settings (see [Settings](#settings)), and/or the board itself - i.e. restart it.

Any parameters that are not provided are considered to be `false`.

| Property| Value | Explanation |
|-|-|-|
| URL | `/settings/validated` |
| Method | POST | |
| Parameters | `deviceConfig` | A boolean value indicating if device settings should be reset to defaults (`true`/1) or not (`false`/0). |
| | `effectsConfig` | A boolean value indicating if effect configuration information should be reset to defaults (`true`/1) or not (`false`/0). |
| | `board` | A boolean value indicating if the device should be restarted (`true`/1) or not (`false`/0). |
| Response | 200 (OK) | An empty OK response. |
