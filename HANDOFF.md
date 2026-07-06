# HANDOFF.md

## Current State

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
