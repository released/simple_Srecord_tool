# AGENTS.md

## Compiler-File Clean Safety

- Both canonical/legacy clean entries use standalone `scripts/clean_intermediates.ps1`.
- Preflight all paths/reparse points before deleting allowlisted files only; never recursively purge obj/tests/verify or infer unknown content is disposable.
- Preserve EXE/INI/DLL/data/log/evidence/unknowns at every nesting level. Codex runs actual clean plus nested-delivery/help/idempotence/junction tests automatically.

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

- Workspace directory-layout reference: `#READ_ME_FIRST#/PROJECT_LAYOUT.md`.
- This project is the MFC directory-layout golden sample. Other tools must not copy its SRecord dependency or product-specific behavior just to match the folders.

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
