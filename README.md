# RearVitaKillerSwitch

A PS Vita plugin that lets the **front touchscreen emulate the rear touchpad**, designed especially for systems with a broken or ghosting rear touch panel.

Developed and tested on a jailbroken **PS Vita PCH-1004 / PCH-1000 series**.

## Features

- Completely suppresses physical rear-touch input, preventing ghost touches from a faulty rear panel.
- **VOL-** switches to Rear Touch Mode.
  - The front touchscreen is mapped to the rear touchpad.
  - Normal front-touch input is suppressed while this mode is active.
- **VOL+** switches back to Front Touch Mode.
  - The front touchscreen works normally again.
  - Physical rear touch remains disabled.
- VOL+ and VOL- are consumed by the plugin, so switching modes does not change the system volume.
- Uses a kernel companion plugin to capture the system volume buttons.
- Tested successfully with VitaTester and God of War Collection.

## Installation

Copy both files to:

```text
ur0:tai/
```

Files:

```text
RearVitaKillerSwitch.suprx
RearVitaKillerSwitchKernel.skprx
```

Add the kernel plugin under the existing `*KERNEL` section of `ur0:tai/config.txt`:

```text
*KERNEL
ur0:tai/RearVitaKillerSwitchKernel.skprx
```

Add the user plugin under `*ALL` so it works in games and applications:

```text
*ALL
ur0:tai/RearVitaKillerSwitch.suprx
```

Do not create duplicate `*KERNEL` or `*ALL` sections if they already exist. Add the corresponding plugin line to the existing section instead.

Reboot the PS Vita after installing or updating the kernel plugin.

## Usage

| Button | Function |
| --- | --- |
| VOL- | Enable Rear Touch Mode — front touchscreen emulates rear touch |
| VOL+ | Enable Front Touch Mode — front touchscreen behaves normally |

The physical rear panel remains disabled in both modes.

## How it works

The project consists of two plugins:

- `RearVitaKillerSwitch.suprx` intercepts the touch APIs, suppresses faulty rear-touch reports, and maps front-touch coordinates to the rear panel when Rear Touch Mode is enabled.
- `RearVitaKillerSwitchKernel.skprx` hooks controller input in the kernel so VOL-/VOL+ can be used as mode switches. Their normal volume-button events are consumed after switching modes.

## Compatibility

Confirmed working on:

- PS Vita PCH-1004 / PCH-1000 series
- VitaTester
- God of War Collection

Other models, firmware versions, games, and plugin combinations have not yet been extensively tested.

## v1.0.0

First stable release.

- Rear ghost-touch suppression
- Front-to-rear touch emulation
- VOL- / VOL+ mode switching
- Volume-button event suppression
- Global `*ALL` application support
- Confirmed working in VitaTester and God of War Collection

## Warning

This project includes a kernel plugin. An incompatible kernel plugin or configuration can prevent the PS Vita from booting normally. Keep a recovery method available and edit `config.txt` carefully.

## License

No license has been selected yet.
