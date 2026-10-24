# HANDOFF.md

## Adopted x64 size reduction 2026-10-25

Canonical 1.0.0.012 GUI is 680,448 bytes; unchanged required srec_cat makes core
delivery 2,121,230 bytes (was 4,213,774). See SELF_CHECK for exact SHA and raw
scope/metrics. Actual formal GUI conversion/output equality, controls/dialog,
Debug workflow, core/error fixtures and seven alternating performance pairs pass.
Actual clean retained all 3 delivery/config hashes and removed 28 compiler files;
nested fixtures/help/idempotence pass. Only x64 MFC registration was excluded;
Win32, features, static runtime/optimization/security unchanged. Visual/DPI/clean-PC
are not claimed. Shared progress records ordinary publication; pilot below is
historical. Do not delete required srec_cat or mix console fixtures into the GUI.

## Size pilot checkpoint 2026-10-24

IN PROGRESS, not an adopted release. Added standard-control MFC exclusion and a
separate console conversion regression target. Release baseline/candidate and
Debug compile pass; actual srec_cat roundtrip/six checksum/error fixtures pass.
Isolated GUI candidate is 680,448 bytes versus 2,772,992; required srec_cat stays.
Formal EXE/version inputs and personal INI are unchanged. SELF_CHECK lists exact
hashes and remaining desktop/performance/canonical-build/clean gates. Native
Computer Use pipe is unavailable; no candidate promotion/commit/push yet.

## Clean Safety Follow-up 2026-10-20

- Replaced named-directory purges with a standalone compiler-file allowlist; canonical and legacy entries share the same helper and support non-destructive help.
- PASS: shared verifier `-CleanBatch scripts/clean_mfc.bat -CleanActualProject -CheckNestedDelivery -CheckReparseRejection`: 17 fixture build sentinels plus source, help/no-deletion, object removal, repeat-clean and junction rejection before deletion.
- PASS: 3 actual protected build files unchanged (SHA-256).
- Application source/project resources are unchanged from verified baseline `a73b78aa6c5b8deea268c702f5e1cd6950c3767b`. The Release x64 build already passed earlier in this migration; it was not rebuilt or version-bumped just for this clean-only follow-up. This is not a new UI/hardware acceptance claim.
- NOT RUN: new functional/device/full visual/DPI/Debug acceptance; no hardware writes or user-instance closure.
- Final commit/publication evidence: workspace `#READ_ME_FIRST#/PROJECT_LAYOUT_MIGRATION.md`.

## Current State

### Layout audit 2026-10-20

- Selected as the workspace MFC directory-layout golden sample; application
  behavior and SRecord dependency are unchanged.
- Build: `scripts/build_mfc.ps1 -Configuration Release -Platform x64 -CloseRunningTool $false` PASS, 0 warnings/errors.
- Existing pre-build step legitimately generated version `1.0.0.011`, header,
  resource, and `build/SimpleSRecordTool.exe`.
- Isolated EXE startup/main-window/normal-close smoke: PASS, exit 0; real user INI
  and firmware inputs were not used. Conversion/hardware/DPI tests: NOT RUN.
- `scripts/clean_mfc.bat`: fixture/repeat-clean PASS; actual clean retained all
  3 protected build files unchanged by SHA-256 (including EXE/INI).
- Shared migration/publication status: `#READ_ME_FIRST#/PROJECT_LAYOUT_MIGRATION.md`.

`simple_Srecord_tool` now contains a standalone MFC project named `SimpleSRecordTool`.

Implemented:

- Visual Studio solution/project and sibling-style scripts.
- Single SRecord tab for BIN/HEX conversion.
- Default operation is now `BIN -> BIN (checksum)`.
- Editable `BIN suffix` defaults to `crc` and generates `<input_stem>_crc.bin`.
- Optional `HEX copy` output is available for the checksum-patched BIN workflow.
- Input browse/reload and output browse.
- `srec_cat` path field.
- Optional offset/app start.
- Optional pad/crop to target image size.
- Optional selectable checksum at final 4 bytes, defaulting to `-crc32-l-e`.
- Reset Defaults button for base `0x0000`, offset `0x0000`, fill `0xFF`, and checksum algorithm `-crc32-l-e`.
- Command preview and copy command.
- Process execution with output captured to the GUI log.
- Checksum byte dump after successful checksum-enabled conversion.
- INI persistence in `build\srecord_tool.ini` beside the executable.

## Important Files

- `src\core\srecord_command.cpp`
- `src\core\process_runner.cpp`
- `src\ui\srecord_tab.cpp`
- `src\main_frame.cpp`
- `scripts\build_mfc.ps1`

## Follow-Up Ideas

- Add named presets for common MCU image layouts.
- Add a dry-run validation button that only runs checksum dump or command preview.
- Add drag-and-drop for input files.
