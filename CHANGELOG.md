# Changelog

## v1.0.0 — 2026-10-22

First stable release of RearVitaKillerSwitch.

### Added

- Suppression of physical rear-touch reports to eliminate ghost input from a faulty rear panel.
- Front touchscreen to rear-touchpad emulation.
- Coordinate mapping from the front panel to the rear touch area.
- VOL- shortcut for enabling Rear Touch Mode.
- VOL+ shortcut for returning to Front Touch Mode.
- Kernel companion plugin for detecting system volume buttons.
- Consumption of VOL-/VOL+ events so switching modes does not alter system volume.
- Support for loading the user plugin globally through the taiHEN `*ALL` section.

### Tested

- PS Vita PCH-1004 / PCH-1000 series
- VitaTester
- God of War Collection
