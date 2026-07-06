# SELF_CHECK.md

## Manual Check List

- [ ] Build Release x64 with `scripts\build_mfc.ps1`.
- [ ] Launch `build\SimpleSRecordTool.exe`.
- [ ] Browse to a `.bin` file and verify operation switches to `BIN -> BIN (checksum)`.
- [ ] Verify `BIN suffix` defaults to `crc` and regenerates Output BIN as `<input_stem>_crc.bin`.
- [ ] Verify optional HEX copy path is enabled for `BIN -> BIN (checksum)`.
- [ ] Browse to a `.hex` file and verify operation switches to `HEX -> BIN`.
- [ ] Toggle Offset/App start and verify offset edit/preset enable state.
- [ ] Toggle Pad/Crop and CRC32 and verify derived address preview changes.
- [ ] Change checksum algorithm dropdown and verify command preview changes.
- [ ] Click Reset Defaults and verify base `0x0000`, offset `0x0000`, fill `0xFF`, and `-crc32-l-e`.
- [ ] Run conversion with `srec_cat` available.
- [ ] Confirm output file is generated.
- [ ] Confirm checksum dump appears in the log when CRC32 is enabled.

## Latest Verification

2026-07-06:

- PASS: `powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build_mfc.ps1 -Configuration Release -Platform x64`
- PASS: Output generated at `build\SimpleSRecordTool.exe`
- PASS: Local `srec_cat` found at `C:\Program Files (x86)\Texas Instruments\Fusion Digital Power Designer\bin\srec_cat.exe`
- PASS: Manual BIN -> HEX command with offset `0x4000`, image size `0x20000`, and CRC32 LE at `0x1FFFC`
- PASS: Manual HEX -> BIN command with image size `0x20000`; output binary size was 131072 bytes
- PASS: Startup smoke test launched `build\SimpleSRecordTool.exe`, confirmed process stayed alive, then closed it
- PASS: Added checksum algorithm dropdown and Reset Defaults button, then rebuilt Release x64 successfully
- PASS: Manual SRecord command verified default `-crc32-l-e`
- PASS: Manual SRecord command verified dropdown option `-checksum-n-l-e` with 4-byte checksum length
- PASS: Startup smoke test passed after checksum dropdown changes
- PASS: Generated new app icon as white `S` on green background
- PASS: Release x64 build accepted the regenerated `src\res\p_tool.ico`
- PASS: Startup smoke test passed after icon update
- PASS: Added workflow notes for the M2A23 CAN PWM LIN AP Keil checksum example
- PASS: Reworked SRecord tab layout into separate Operation, Output, Address, Image, and Checksum rows
- PASS: Screenshot check confirmed long labels no longer overlap or truncate in the settings area
- PASS: Manual M2A23-style BIN -> HEX command verified with image size `0x10000` and checksum at `0x0FFFC`
- PASS: Changed default primary workflow to `BIN -> BIN (checksum)`
- PASS: Added optional `HEX copy` side output for the checksum-patched BIN
- PASS: Manual M2A23-style BIN -> BIN command verified with output size `0x10000` and checksum at relative `0x0FFFC`
- PASS: Manual HEX copy command verified from checksum-patched BIN
- PASS: Screenshot check confirmed the new primary BIN / optional HEX copy rows render cleanly
- PASS: Added editable `BIN suffix` defaulting to `crc` for the checksum-patched BIN output name
- PASS: Release x64 build passed after `BIN suffix` and command-button spacing changes
- PASS: Screenshot check at 1200px width confirmed `Convert`, `Copy Command`, and `Open Folder` render with clear spacing
