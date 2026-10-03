# STM32N6570-DK boot validation

Validated on 2026-10-03 (Asia/Seoul).

The final foundation bundle was rebuilt by Tools/Build-Firmware.ps1 without
CubeIDE generated makefiles, programmed and verified by
Tools/Program-Firmware.ps1 -IncludeFsbl, then independently boot-tested.
The user confirmed LED1 on and LED2 blinking after BOOT0=L/BOOT1=L power
reconnection. Firmware/baseline holds this final set and its SHA256 manifest.
Secure and Non-secure executable payloads matched the prior tested binaries
byte-for-byte. FSBL now links the identical shared XSPI HAL rather than a
source copy; function addresses therefore differ from historical maps.

## Confirmed result

With BOOT0=L and BOOT1=L, after power removal and reconnection without
starting a debugger, the user observed LED1 staying on and LED2 repeatedly
turning on and off. LED1 indicates FSBL NOR header read success. LED2 is
toggled by the non-secure main loop every 500 HAL ticks.

The validated flow is external NOR boot -> FSBL -> SRAM image loading ->
Secure startup -> Non-secure startup and main loop.

Earlier SRAM debugging also confirmed secure_boot_stage=6 and a changing
nonsecure_boot_counter.

## Image set currently programmed

| NOR address | Signed image relative to project root |
| --- | --- |
| 0x70000000 | Firmware/baseline/FSBL-trusted.bin |
| 0x70100000 | Firmware/baseline/Secure-trusted.bin |
| 0x70180000 | Firmware/baseline/NonSecure-trusted.bin |

CubeProgrammer 2.23.0 verified each download. The external loader used was
MX66UW1G45G_STM32N6570-DK.stldr. Do not confuse these images with the older
diagnostic-trusted or sau-fix-trusted image sets.

The original FSBL sector was backed up before replacement:
Firmware/baseline/original-fsbl-sector.bin (64 KiB from 0x70000000).
Its first 30496 bytes matched the original Ondevice_FSBL-trusted.bin.

## Changes required for success

- Enable Secure SAU configuration, using the ST isolation LRUN template:
  Secure Gateway veneers as NSC, 0x24100000-0x241FFFFF as non-secure SRAM,
  and 0x40000000-0x4FFFFFFF as the non-secure peripheral alias region.
- Keep the existing RISAF SRAM isolation configuration.
- Build Secure first, then link Non-secure against the newly generated
  Secure import library (Build/Debug/AppliSecure/secure_nsclib.o in scripts,
  AppliSecure/Debug/secure_nsclib.o in CubeIDE). Deploy both apps together.
  Reusing an old Non-secure image after Secure veneers move caused INVEP.
- Secure initializes LED2 on PG10 and marks this pin non-secure. The NS
  main loop toggles LED2. LED1 on PO1 remains the FSBL read indicator.

## Size-aware loader (SRAM and independent NOR boot validated)

The working source now reads the v2.3 image length at 0x6C. For the current
`-nk -align` image format, signed file length is 160 + 416 + image_length.
The length includes the 448 alignment bytes preceding the vector table.
Expected copies for the validated LED2 app set are 6272 and 3232 bytes.

Validated FSBL: Firmware/baseline/FSBL-trusted.bin.
It was programmed at 0x70000000 and verified by CubeProgrammer. After power
removal, BOOT0=L / BOOT1=L, and power reconnection without debugging, the
user confirmed LED2 blinking. This confirms the size-aware loader reaches
the non-secure main loop through independent NOR boot.
The prior fixed-copy working image remains available as
Firmware/baseline/FSBL-fixed-copy-trusted.bin for rollback.
CubeIDE SRAM debugging validated the new FSBL: status=7 (READY), Secure
copy size=6272, Non-secure copy size=3232, secure_boot_stage=6 and increasing
NS counter. With the debugger resumed, LED2 blinked and NS HAL Tick increased
from 3918 to 8606. LED2 remains static while the core is suspended.

After RISAF makes SRAM2 non-secure, inspect FSBL diagnostics through the
non-secure aliases: 0x241C002C (status), 0x241C0038 (Secure size), and
0x241C003C (NS size). Secure-alias debug reads returned zero in this session.
NS HAL Tick is at 0x24180030 for this app build.

The CLI `-s 0x34180400` operation reported success but a subsequent register
read showed PC still in Boot ROM. It did not establish FSBL execution.
CubeIDE debugging was the successful SRAM validation procedure.

Header/version/extension checks reject other formats (including encrypted or
authenticated app headers). Copies are bounded by 0x64000 bytes for Secure
and 0x80000 bytes for Non-secure, excluding each app's RAM/data region and
the FSBL execution area. Reset vectors must lie within the copied code.
D-cache is disabled before writes; copied bytes, vectors and the additive
payload checksum are checked. These checks do not authenticate applications.

Run `Tests\run_fsbl_image_tests.cmd` from a terminal with the installed MSVC
Community toolchain. Tests execute the same parser and production loader
against mock NOR and allocated SRAM addresses. Cases include current images,
malformed headers, bounds/overflow, invalid vectors, partial final transfers,
images above 64 KiB, NOR read failure, and payload corruption. All passed;
the STM32 FSBL target also built successfully.

## Diagnostic limitations and next work

- The size-aware FSBL is now programmed in NOR and has passed independent
  boot validation with the current app pair. Signed/authenticated and encrypted
  application formats remain unsupported by this loader.
- Non-secure D-cache remains disabled for this diagnostic stage.
- At this checkpoint, diagnostic symbols remain at 0x3406402C (Secure stage)
  and 0x2418002C (NS counter). Recheck the maps after future builds.
- HotPlug debug attachment to the running NS/normal-boot state failed in
  this session. Do not equate attachment failure with boot failure.
- For programming, set BOOT0=L, BOOT1=H and power-cycle. A reset button
  alone did not reliably restore debug access during this session.
- Generated Debug/Release/Build artifacts are excluded from the foundation
  commit. Tools/Build-Firmware.ps1 compiles all sources without generated
  makefiles. FSBL links the shared XSPI HAL through its .project file.
  No OTP changes or mass erase were performed.
