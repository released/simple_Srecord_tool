# Simple SRecord Tool

Standalone Windows MFC GUI for common SRecord firmware image operations.

## Scope

The tool wraps `srec_cat` so app-code images can be converted and prepared without editing post-build batch files each time.

Supported workflows:

- Load `.bin`, reload it after a firmware rebuild, optionally offset/pad/add checksum, then output checksum-patched BIN with an editable output suffix.
- Optionally generate an Intel HEX copy from the checksum-patched BIN.
- Load `.hex`/`.ihx`, reload it after a firmware rebuild, optionally offset/pad/add checksum, then output binary.
- Preview the exact `srec_cat` command before running it.
- Show the calculated checksum position and dump the generated checksum bytes after conversion.

## Default Image Semantics

The defaults follow common IAP/ISP post-build examples:

- SRecord executable: `srec_cat`
- Output BIN suffix: `crc`, producing `<input_stem>_crc.bin` for the primary BIN-to-BIN checksum flow
- Base address: `0x0000`
- Offset/App start: disabled by default, editable presets include `0x0200`, `0x0400`, `0x1000`, `0x3000`, `0x4000`, and `0x4800`
- Image size: `0x20000`
- Fill byte: `0xFF`
- Checksum: optional, default algorithm `-crc32-l-e`, at `base + image_size - 4`
- Reset Defaults button: restores base `0x0000`, offset `0x0000`, fill `0xFF`, and checksum algorithm `-crc32-l-e`

When checksum is enabled, the generated command uses this shape:

```bat
srec_cat input -binary -offset APP_START -fill 0xFF BASE CRC_ADDR -crop BASE CRC_ADDR -crc32-l-e CRC_ADDR -o output -Intel
```

Supported checksum dropdown options:

- `-crc32-l-e`
- `-crc32-b-e`
- `-checksum-n-l-e`
- `-checksum-n-b-e`
- `-checksum-p-l-e`
- `-checksum-p-b-e`

For `HEX -> BIN`, the input format is `-Intel` and output format is `-binary`.

## Plain Format Conversion Without Checksum

Use these settings when the goal is only to convert between BIN and Intel HEX without modifying the image content.

### BIN to HEX

GUI settings:

- Input: select the `.bin` file
- Operation: `BIN -> HEX`
- Output HEX: select the output `.hex` path
- Offset/App start: unchecked
- Pad/Crop to image size: unchecked
- Add checksum at last 4 bytes: unchecked
- Algorithm: ignored because checksum is disabled
- Click `Convert`

Generated command shape:

```bat
srec_cat input.bin -binary -o output.hex -Intel
```

If the HEX records must start at a specific address, enable only `Offset/App start` and set the value, for example `0x3000`:

```bat
srec_cat input.bin -binary -offset 0x3000 -o output.hex -Intel
```

### HEX to BIN

GUI settings:

- Input: select the `.hex` or `.ihx` file
- Operation: `HEX -> BIN`
- Output BIN: select the output `.bin` path
- Offset/App start: unchecked
- Pad/Crop to image size: unchecked
- Add checksum at last 4 bytes: unchecked
- Algorithm: ignored because checksum is disabled
- Click `Convert`

Generated command shape:

```bat
srec_cat input.hex -Intel -o output.bin -binary
```

If the output needs a fixed image size but still no checksum, enable only `Pad/Crop to image size` and set `Size` and `Fill`, for example `0x20000` and `0xFF`.

Quick rule:

- Pure format conversion: `Offset/App start` off, `Pad/Crop` off, checksum off.
- HEX output with a non-zero start address: enable only `Offset/App start`.
- Fixed-size output without checksum: enable only `Pad/Crop`.

## Usage Example: M2A23 APROM Checksum BIN

This example verifies the GUI output against the original batch output.

GUI settings:

- Input: `build\APROM_original.bin`
- Operation: `BIN -> BIN (checksum)`
- Output BIN: `build\APROM_original_application.bin`
- HEX copy: checked
- HEX path: `build\APROM_original_srecord.hex`
- Base: `0x0000`
- Offset/App start: unchecked, or checked with `0x0000`
- Pad/Crop to image size: checked
- Size: `0x10000`
- Fill: `0xFF`
- Add checksum at last 4 bytes: checked
- Algorithm: `-crc32-l-e`

Reference batch output:

```text
build\__APROM_application__.bin
```

Expected verification result:

```text
APROM_original_application.bin size : 65536
__APROM_application__.bin size      : 65536
SHA256                              : A4713A42E4F2730580506FBFD2646E69A8E6ACAFF022ED2C45AFD122526FA890
Binary byte compare                 : identical
```

Checksum bytes at the end of the 0x10000-byte image:

```text
0x0FFF0: FF FF FF FF FF FF FF FF FF FF FF FF 50 04 1D 18
```

`APROM_original_srecord.hex` can also be converted back to BIN with `srec_cat`; the decoded BIN should match both `APROM_original_application.bin` and `__APROM_application__.bin` byte-for-byte.

## Usage Example: M2A23 IAP UART APROM With Offset

Reference project-relative folder:

```text
M2A23BSP_IAP_UART_APROM\SampleCode\Template\AP\Keil
```

Relevant `checksum_config.cmd` values:

```bat
set APROM_BASE=0x0000
set APROM_SIZE=0x20000
set APP_START=0x3000
set APP_SIZE=0x1D000
set CRC_ADDR=0x1FFFC
```

This is an offset/app-start case. The linker scatter file places the app at `0x3000`, but `obj\APROM_application.bin` is still an app-only binary. Therefore the checksum is written into the BIN at the relative offset:

```text
CRC_OFFSET = CRC_ADDR - APP_START = 0x1FFFC - 0x3000 = 0x1CFFC
Output BIN size = CRC_OFFSET + 4 = 0x1D000
```

GUI settings:

- Input: `obj\APROM_application.bin`
- Operation: `BIN -> BIN (checksum)`
- Output BIN: `obj\APROM_application_crc.bin`
- HEX copy: checked
- HEX path: `obj\APROM_application.hex`
- Base: `0x0000`
- Offset/App start: checked
- Offset/App start value: `0x3000`
- Pad/Crop to image size: checked
- Size: `0x20000`
- Fill: `0xFF`
- Add checksum at last 4 bytes: checked
- Algorithm: `-crc32-l-e`

Expected preview values:

```text
Image end              : 0x20000
Checksum address       : 0x1FFFC..0x20000
Checksum offset in BIN : 0x1CFFC
Output BIN size        : 0x1D000
```

The generated primary BIN command should match the batch's Step 2 behavior, except the GUI writes to a separate output file instead of overwriting `obj\APROM_application.bin`:

```bat
srec_cat obj\APROM_application.bin -binary -fill 0xFF 0x0000 0x1D000 -crop 0x0000 0x1CFFC -crc32-l-e 0x1CFFC -o obj\APROM_application_crc.bin -binary
```

The optional HEX copy then applies the app start offset, matching the batch's Step 4 behavior:

```bat
srec_cat obj\APROM_application_crc.bin -binary -offset 0x3000 -o obj\APROM_application.hex -Intel
```

When comparing with the batch output, keep in mind that `generateChecksum.bat` writes the checksum back into `obj\APROM_application.bin`. Compare that patched batch BIN against the GUI's `obj\APROM_application_crc.bin`, or run each flow from a separate copied build folder.

## Additional Batch-Derived GUI Presets

The following presets were derived from BSP sample projects that contain both `generateChecksum.bat` and `checksum_config.cmd`.

Covered MCU/BSP families:

```text
M480
M2A23
M253
M031
NUC131
NUC1262
M0A21
```

Common GUI settings for these checksum examples:

- Operation: `BIN -> BIN (checksum)`
- BIN suffix: `crc`, or another suffix if the output file name should differ
- HEX copy: checked when an Intel HEX side output is needed
- Base: usually `0x0000`
- Offset/App start: checked when the table value is non-zero
- Pad/Crop to image size: checked
- Fill: `0xFF`
- Add checksum at last 4 bytes: checked
- Algorithm: `-crc32-l-e`

For GUI `Size`, use the absolute checksum position rule:

```text
GUI Size = CRC_ADDR - Base + 4
Checksum offset in BIN = CRC_ADDR - APP_START
Output BIN size = Checksum offset in BIN + 4
```

This matters because some batch configs use `APP_SIZE` or `APROM_SIZE` for other layout meanings. The GUI's `Size` controls where the final 4-byte checksum is placed.

| Reference project | Input BIN | Offset/App start | GUI Size | Checksum offset in BIN | Output BIN size |
| --- | --- | ---: | ---: | ---: | ---: |
| `M2A23BSP_CANFD_TX_RX` | `obj\APROM_application.bin` | `0x0000` | `0x10000` | `0xFFFC` | `0x10000` |
| `M2A23BSP_ISP_CAN_APROM` | `obj\APROM_application.bin` | `0x0000` | `0x10000` | `0xFFFC` | `0x10000` |
| `M2A23BSP_IAP_UART_APROM` | `obj\APROM_application.bin` | `0x3000` | `0x20000` | `0x1CFFC` | `0x1D000` |
| `M480BSP_IAP_HID_20_APROM` | `obj\APROM_application.bin` | `0x10000` | `0x80000` | `0x6FFFC` | `0x70000` |
| `M480BSP_IAP_LwIP_TCP_APROM` | `obj\APROM_application.bin` | `0x20000` | `0x80000` | `0x5FFFC` | `0x60000` |
| `M480BSP_IAP_LwIP_webserver_APROM` | `obj\APROM_application.bin` | `0x20000` | `0x80000` | `0x5FFFC` | `0x60000` |
| `M480BSP_IAP_XMODEM_APROM` Keil | `obj\APROM_application.bin` | `0x10000` | `0x30000` | `0x1FFFC` | `0x20000` |
| `M480BSP_IAP_XMODEM_APROM` GCC | `Release\AP.bin` | `0x10000` | `0x30000` | `0x1FFFC` | `0x20000` |
| `M480BSP_ISP_HID_20_APROM` | `obj\APROM_application.bin` | `0x0000` | `0x7B000` | `0x7AFFC` | `0x7B000` |
| `M480BSP_ISP_HID_APROM` | `obj\APROM_application.bin` | `0x0000` | `0x76000` | `0x75FFC` | `0x76000` |
| `M480BSP_ISP_UART_APROM` | `obj\APROM_application.bin` | `0x0000` | `0x1E000` | `0x1DFFC` | `0x1E000` |
| `M253BSP_IAP_XMODEM_APROM` Keil | `obj\APROM_application.bin` | `0x4800` | `0x20000` | `0x1B7FC` | `0x1B800` |
| `M253BSP_IAP_XMODEM_APROM` GCC | `Release\AP.bin` | `0x4800` | `0x20000` | `0x1B7FC` | `0x1B800` |
| `M031BSP_IAP_UART_APROM` | `obj\APROM_application.bin` | `0x4800` | `0x1F800` | `0x1AFFC` | `0x1B000` |
| `M031BSP_IAP_XMODEM_APROM` | `obj\APROM_application.bin` | `0x4800` | `0x1F800` | `0x1AFFC` | `0x1B000` |
| `M031BSP_IAP_UART_APROM_DualBackup` | `obj\APROM_application.bin` | `0x4000` | `0x10000` | `0xBFFC` | `0xC000` |
| `M031BSP_ISP_UART_APROM` | `obj\APROM_application.bin` | `0x0000` | `0x1E000` | `0x1DFFC` | `0x1E000` |
| `M031BSP_ISP_XMODEM_APROM` | `obj\APROM_application.bin` | `0x0000` | `0x1C000` | `0x1BFFC` | `0x1C000` |
| `M031BSP_FMC_IAP_IN_SRAM` | `obj\APROM_application.bin` | `0x0000` | `0xF000` | `0xEFFC` | `0xF000` |
| `NUC1262BSP_ISP_HID_APROM` | `obj\APROM_application.bin` | `0x0000` | `0x1D000` | `0x1CFFC` | `0x1D000` |
| `NUC131BSP_IAP_UART_APROM` | `obj\APROM_application.bin` | `0x0000` | `0xD000` | `0xCFFC` | `0xD000` |
| `NUC131BSP_ISP_UART_APROM` | `obj\APROM_application.bin` | `0x0000` | `0xE000` | `0xDFFC` | `0xE000` |
| `M0A21BSP_IAP_UART_APROM` | `obj\APROM_application.bin` | `0x0000` | `0x5800` | `0x57FC` | `0x5800` |
| `M0A21BSP_ISP_UART_APROM` | `obj\APROM_application.bin` | `0x0000` | `0x6000` | `0x5FFC` | `0x6000` |

Example command shape for the M480 LwIP APROM case:

```bat
srec_cat obj\APROM_application.bin -binary -fill 0xFF 0x0000 0x60000 -crop 0x0000 0x5FFFC -crc32-l-e 0x5FFFC -o obj\APROM_application_crc.bin -binary
srec_cat obj\APROM_application_crc.bin -binary -offset 0x20000 -o obj\APROM_application.hex -Intel
```

If a batch uses a separate CRC calculation end such as `CRC_CALC_END` that is different from the checksum address, treat it as a special case and verify the generated command before using the GUI output. The current GUI presets above assume the checksum is calculated up to the final 4-byte checksum field.

## Requirements

- Visual Studio 2022 with C++ and MFC components.
- SRecord installed so `srec_cat` is in `PATH`, or browse to `srec_cat.exe` in the GUI.

## Build

```bat
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build_mfc.ps1 -Configuration Release -Platform x64
```

Output:

```text
build\SimpleSRecordTool.exe
```

## Files

- `SimpleSRecordTool.sln` / `SimpleSRecordTool.vcxproj`: Visual Studio MFC project.
- `src\ui\srecord_tab.*`: main GUI controls for file paths, offset, padding, checksum, command preview, and execution.
- `src\core\srecord_command.*`: SRecord command generation and parameter validation.
- `src\core\process_runner.*`: hidden process execution with stdout/stderr capture.
- `scripts\build_mfc.ps1`: MSBuild wrapper matching sibling simple tools.
- `docs\SRECORD_WORKFLOW.md`: SRecord flow notes, including the M2A23 CAN PWM LIN AP Keil example.
