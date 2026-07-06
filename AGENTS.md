# AGENTS.md

## Project Purpose

This project builds a standalone Windows MFC GUI for SRecord based firmware image conversion.

- Users can load BIN or Intel HEX files.
- Users can reload the same input path after rebuilding firmware.
- Users can apply an optional address offset/app start before conversion.
- Users can pad/crop to a target image size.
- Users can add a selectable checksum at the final 4 bytes of the configured image.
- The primary BIN workflow is BIN without checksum to BIN with checksum; Intel HEX can be generated as an optional side copy.
- Users can convert BIN to HEX or HEX to BIN through `srec_cat`.

## Local Rules

- Keep the project layout aligned with sibling simple tools:
  - `src\`
  - `scripts\`
  - `docs\`
  - `demo_code\`
  - root `README.md`, `AGENTS.md`, `SELF_CHECK.md`, `HANDOFF.md`
- Keep the PC app standalone. Do not add MCU firmware or HID bridge behavior unless explicitly requested.
- Prefer SRecord command generation over reimplementing image conversion algorithms in the GUI.
- Make the smallest safe change and avoid unrelated refactors.

## Verification Expectations

For PC GUI changes, build:

```bat
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build_mfc.ps1 -Configuration Release -Platform x64
```

Expected output:

- `build\SimpleSRecordTool.exe`
