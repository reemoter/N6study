# External PSRAM bring-up

STM32N6570-DK: APS256XX, 256Mbit = 32MiB, XSPI1 Port 1, 16-bit DDR.
Data: PP0..PP15; CS: PO0; DQS0/1: PO2/PO3; clock: PO4 (AF9).
VDDIO2 is explicitly configured for 1.8V. NOR remains XSPI2 Port 2.

Secure_BootEnterNonSecure calls ExtRam_SecureInit before suspending Secure
HAL tick and entering NS. Initial clock is HCLK/4 for mode-register setup;
mapped transfers then use HCLK (currently 100MHz), fixed read/write latency 7.
The driver checks each register write and map operation. Debug status 10..13
identifies the initialization stage (10 controller contract, 12 registers/ID, 13 mapping); 1 means mapped RAM and permissions ready.
ID is read before switching to x16 mode; invalid all-zero/all-one IDs reject init.

Only XSPI1 is reset. XSPIM configuration preserves the NOR port configuration;
there is no shared XSPIM reset. The controller and GPIO remain Secure-owned.
RISAF11 region 1 allows NS access to offsets 0..0x01FFFFFF. SAU region 3 exposes
0x90000000..0x91FFFFFF as NS data. The FreeRTOS default task allocates its Secure
context before querying SECURE_ExtRamStatus/Id gateways.

AppRamTest_Run executes once at startup before the normal 10ms LED/status loop.
It overwrites all 32MiB twice with address-dependent complementary patterns,
then verifies every 32-bit word after each full write pass. It yields every
32KiB so the log task can send progress. This is destructive bring-up code:
remove/disable it before using PSRAM for frame buffers, heap, or model buffers.
It refuses to claim physical RAM validation when NS DCache is enabled.
It is not a bandwidth benchmark or a complete RAM qualification test.

UART acceptance: init=1 and a plausible ID, pass=1, pass=2, final RAM PASS,
then ordinary periodic NS logs. LED2 resumes its normal 7500ms interval after
startup testing. Startup can take several seconds. Failures do not intentionally
stop the normal app: initialization/test failures are reported, but bus/security
faults may stop execution and require debugger inspection.
Debug variables: psram_init_status/id/clock_hz (Secure), ram_test_status
(0 idle, 1 running, 2 passed, 3 failed, 4 cache enabled), ram_test_fail_address,
ram_test_expected, ram_test_actual, ram_test_bytes (NS).

The integration resides in project-owned ext_ram_secure.c/h and app_ram_test.c/h,
with FreeRTOS startup hooks inside USER CODE blocks. HAL_XSPI_MODULE_ENABLED is selected by generated Secure HAL configuration. SAU region 3 is reserved and checked
at compile time; do not allocate it in CubeMX. The .ioc describes PSRAM pins and basic controller configuration; runtime mapping and access policy remain project-owned.
CubeMX regeneration must retain the header USER block and NSC declarations.
Sync-HalLinks restores the Secure XSPI HAL source link when building via 01.

Vendor aps256xx.c/h are unmodified copies from STM32Cube_FW_N6_V1.4.1,
Drivers/BSP/Components/aps256xx. License is aps256xx.LICENSE.txt beside the source.
aps256xx_conf.h is the official template with the STM32N6 HAL include selected.
References: installed STM32N6570-DK BSP stm32n6570_discovery_xspi.c/h,
and ST RIF overview https://wiki.st.com/stm32mcu/wiki/Security:Resource_Isolation_Framework_(RIF)_overview_for_STM32N6

Hardware validation passed on STM32N6570-DK (2026-10-05): init=1, ID=0x0d10, both complete 32MiB write/read passes verified from NonSecure, elapsed 18432ms. This establishes CPU access with NS DCache disabled; DMA/NPU access and cached buffer coherency remain unverified. Previous working queued-logger bundle is saved
in Build/backups/pre-psram-bundle (local ignored backup).


## CubeMX integration (2026-10-05)

The saved .ioc now describes XSPI1 in AppliSecure: Hexa Port1, PO0/NCS1,
PP0..PP15, PO2/PO3 DQS, PO4 clock; AF9, Pull-up, Very High speed.
Generated Secure main calls MX_XSPI1_Init once before SystemIsolation_Config.
Generated xspi.c owns hxspi1, HCLK selection, GPIO and controller configuration.
ExtRam_SecureInit now reuses hxspi1 without calling HAL_XSPI_Init or GPIO init.
The Secure MSP USER block selects VDDIO2=1.8V before pin initialization;
XSPI1 MSP USER block resets only XSPI1. Mode-register commands, memory mapping,
RISAF11 and runtime SAU region 3 remain project-owned.
The manually forced Secure HAL XSPI enable is superseded by generated config.

Check-CubeMX-Boot validates PSRAM size/type/prescaler/refresh/boundary, PO0/NCS1,
PO1 GPIO, generated initialization, HCLK and preserved voltage hook; it rejects
basic initialization duplicated in the PSRAM driver or a shared XSPIM reset.
The CubeMX warning mentioning PO1 while NCS1 is selected did not produce an
NCS2 assignment: generated nCSOverride is NCS1 and the RAM GPIOO mask excludes
PO1. The UI warning cause is still not established.

Both scripted and CubeIDE builds passed after migration. Hardware revalidation passed on 2026-10-05: generated XSPI1 initialization, init=1, ID=0x0d10, both full 32MiB write/read patterns verified from NS, elapsed_ms=18432. The queued logger also started successfully. DMA/NPU access and cached buffer coherency remain unverified.


