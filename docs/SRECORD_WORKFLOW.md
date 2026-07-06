# SRecord Workflow

The GUI generates `srec_cat` commands using the same high-level flow as the IAP/ISP post-build batches in the local BSP workspaces.

## BIN to HEX with Offset and CRC

Example:

```bat
srec_cat app.bin -binary -offset 0x4000 -fill 0xFF 0x0000 0x1FFFC -crop 0x0000 0x1FFFC -crc32-l-e 0x1FFFC -o app_srecord.hex -Intel
```

Meaning:

- Interpret `app.bin` as binary data starting at address `0`.
- Move it to absolute address `0x4000`.
- Fill empty image bytes with `0xFF`.
- Calculate CRC32 over `0x0000..0x1FFFB`.
- Store the CRC32 little-endian bytes at `0x1FFFC..0x1FFFF`.
- Write Intel HEX.

The GUI exposes the checksum algorithm as a dropdown. `-crc32-l-e` is the default. For 4-byte SRecord sum/checksum variants such as `-checksum-n-l-e`, the generated command appends `4` after the checksum address.

## HEX to BIN with CRC

Example:

```bat
srec_cat app.hex -Intel -fill 0xFF 0x0000 0x1FFFC -crop 0x0000 0x1FFFC -crc32-l-e 0x1FFFC -o app_srecord.bin -binary
```

The binary output is normally padded from base address `0x0000` when padding or checksum is enabled.

## Address Fields

- Base address: start of the full image region.
- Offset/App start: optional shift applied to input records before padding/checksum.
- Image size: total expected image size.
- CRC address: `base + image_size - 4`.
- CRC offset in app input: `CRC address - offset`.
- Reset Defaults restores base address `0x0000`, offset `0x0000`, fill byte `0xFF`, and checksum algorithm `-crc32-l-e`.

## M2A23 CAN PWM LIN AP Keil Example

Reference folder:

```text
D:\SourceCode\_Avery_M2A23\M2A23BSP_ISP_CAN_PWM_LIN\SampleCode\Template\AP\Keil
```

`checksum_config.cmd` values:

```bat
set APROM_BASE=0x0000
set APROM_SIZE=0x10000
set APP_START=0x0000
set APP_SIZE=0x10000
set CRC_ADDR=0x0FFFC
```

Equivalent GUI operation:

- Input: `obj\APROM_application.bin`
- Operation: `BIN -> BIN (checksum)`
- BIN suffix: `crc`
- Output BIN: `obj\APROM_application_crc.bin` or another `.bin` output path
- HEX copy: checked if `obj\APROM_application.hex` should also be generated
- HEX path: `obj\APROM_application.hex`
- SRecord: `srec_cat` or full path to `srec_cat.exe`
- Base: `0x0000`
- Offset/App start: unchecked, or checked with value `0x0000`
- Pad/Crop to image size: checked
- Size: `0x10000`
- Fill: `0xFF`
- Add checksum at last 4 bytes: checked
- Algorithm: `-crc32-l-e`
- Click `Convert`

The preview should show a checksum address of `0xFFFC..0x10000`. That matches `CRC_ADDR=0x0FFFC`.

Changing `BIN suffix` regenerates the default Output BIN path as `<input_stem>_<suffix>.bin`.

This matches the original batch intent:

- Step 2 main output: BIN without checksum -> BIN with checksum
- Step 4 side output: checksum-patched BIN -> Intel HEX
