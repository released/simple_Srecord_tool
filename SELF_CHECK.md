# SELF_CHECK.md

## Adopted x64 size reduction 2026-10-25

- Canonical Release 1.0.0.012: 680,448 bytes, SHA-256
  `8C437059EF4F2E0F9216174479CCA48BA3C97B996FE5802FC3C2852E61A93B8B`.
  Baseline GUI 2,772,992 bytes; unchanged required srec_cat 1,440,782 bytes.
  Core delivery binaries 4,213,774 -> 2,121,230 bytes; no hidden/new runtime.
  /O2 /GL /MT /GS, x64 and PE 0x8160 retained. Win32 definitions unchanged.
- PASS canonical build_mfc.ps1 -Configuration Release -Platform x64
  -CloseRunningTool $false (no running canonical instance); zero warnings/errors.
  Version/resource generation is normal. Original GUI/srec_cat remain preserved.
- PASS authorized isolated actual formal-EXE BIN -> HEX conversion, baseline/
  formal output byte equality, identical control-class shape, owned Browse file
  dialog open/cancel, normal exit. Debug candidate exercises conversion/dialog
  without assertion. Separate console roundtrip/six checksum/error tests pass.
- PASS scoped paired performance: two warmups, seven baseline calibration samples,
  seven alternating AB/BA pairs; no concurrent compiler. Before-candidate delta
  limits max(floor, 10% baseline median, 3*MAD); memory 5%; floors startup 25ms,
  workflow 2ms, CPU 16ms, memory 1MiB. Medians baseline -> formal: startup
  187.7740 -> 195.3647ms; GUI conversion 128.7977 -> 137.6756ms; process CPU
  187.500 -> 171.875ms; peak working set 18,190,336 -> 17,723,392 bytes.
  All within baseline-derived limits; do not describe each sample as faster.
- Raw limits/samples: shared batch ui-pair-e9f98fc444a448cfa1052fa28fb2d5fb.
- PASS actual clean_mfc.bat via preservation verifier: 28 compiler files removed,
  all 3 protected actual files unchanged by SHA; 17 nested fixture files/source,
  help/idempotence PASS. Prior unchanged junction coverage is reused.
- NOT RUN: visual/DPI, clean-PC and Win32 builds, broad unrelated format variants.
  No personal INI/real firmware/hardware/clipboard was used. Ordinary reviewed
  publication and exact remote SHA are tracked in shared EXE_SLIMMING_PROGRESS.md.

## Historical size pilot checkpoint 2026-10-24 — before adoption

- Original formal GUI and srec_cat preserved in the shared isolated batch;
  personal srecord_tool.ini was not used in tests. Formal output is unchanged.
- Rebuilt Release x64 baseline: 2,772,992 bytes; SHA-256
  `2A4C3B9BDBF5D838B1E55954948138AA586BD67002BC47BD5950A707D5BD93C3`.
- Standard-control MFC candidate: 680,448 bytes; SHA-256
  `2DD725F219744B72F039D50A6B6694B54DE1F9626108E9CA9A7A02AD398CFF0D`.
  Required srec_cat remains 1,440,782 bytes: core delivery binaries total
  4,213,774 -> 2,121,230 bytes, before unchanged settings/notices.
- PASS: separate Release baseline/candidate and candidate Debug x64 builds;
  /O2 /GL /MT /GS, architecture and PE 0x8160 retained; no reported compiler/linker
  warnings/errors (existing informational _WIN32_WINNT default messages remain).
  PreBuildEventUseInBuild=false prevents version churn in these paired trials.
- PASS: separate tests/SizeRegression.vcxproj console target, actual unchanged
  srec_cat, fresh fixtures: BIN/HEX roundtrip byte equality, all six checksum
  workflows with padding/dump, invalid algorithm/fill/address overflow rejection
  and original input preservation. No real firmware files or GUI were used.
- NOT RUN: actual formal/candidate GUI controls/dialogs/Debug startup, paired
  startup/UI/CPU/memory metrics, canonical build/actual clean/publication.
  Computer Use native pipe failed retry/reset. Alternative isolated script
  authorization requested; do not adopt or claim performance neutrality yet.
- Shared progress/evidence: #READ_ME_FIRST#/EXE_SLIMMING_PROGRESS.md. No native
  commit/push has been performed for this candidate.

## Clean Safety Follow-up 2026-10-20

- Replaced named-directory purges with a standalone compiler-file allowlist; canonical and legacy entries share the same helper and support non-destructive help.
- PASS: shared verifier `-CleanBatch scripts/clean_mfc.bat -CleanActualProject -CheckNestedDelivery -CheckReparseRejection`: 17 fixture build sentinels plus source, help/no-deletion, object removal, repeat-clean and junction rejection before deletion.
- PASS: 3 actual protected build files unchanged (SHA-256).
- Application source/project resources are unchanged from verified baseline `a73b78aa6c5b8deea268c702f5e1cd6950c3767b`. The Release x64 build already passed earlier in this migration; it was not rebuilt or version-bumped just for this clean-only follow-up. This is not a new UI/hardware acceptance claim.
- NOT RUN: new functional/device/full visual/DPI/Debug acceptance; no hardware writes or user-instance closure.
- Final commit/publication evidence: workspace `#READ_ME_FIRST#/PROJECT_LAYOUT_MIGRATION.md`.

## Layout Verification 2026-10-20

- PASS: Release x64 build, 0 warnings/errors (`scripts/build_mfc.ps1`, with
  `-CloseRunningTool $false`). Generated version: `1.0.0.011`.
- PASS: isolated EXE startup, main-window handle, normal close and exit 0.
- PASS: actual `scripts/clean_mfc.bat`; 3 protected build files SHA-256 unchanged.
- PASS: isolated clean sentinels and idempotence (9 build files plus source).
- NOT RUN: new SRecord conversions, hardware tests, exhaustive GUI/DPI review;
  no functional code was changed for this directory-layout reference.

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
