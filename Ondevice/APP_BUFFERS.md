# PSRAM application buffers

This is a provisional storage layout, not a working camera/NPU pipeline.
Capacities live in AppliNonSecure/Core/Inc/app_buffers.h; change them after
selecting capture format/resolution and the model's generated memory contract.

| Reservation | Address (current build) | Bytes | Intended use |
|---|---|---:|---|
| camera0 | 0x90000000 | 153600 | Candidate 320x240 RGB565 frame |
| camera1 | 0x90025800 | 153600 | Second candidate frame |
| input | 0x9004B000 | 230400 | Candidate 320x240 RGB888 input |
| output | 0x90083400 | 65536 | Provisional inference output capacity |
| work | 0x90093400 | 4194304 | Provisional inference workspace capacity |

Reserved: 4,797,440 bytes. Unreserved: 28,756,992 bytes. These free bytes are not
an allocator or guaranteed model workspace. Use descriptors, not hardcoded
addresses; changing sizes moves later buffers. SORT_BY_NAME gives stable order.

The .external_ram section is 64-byte aligned, NOLOAD and separate from .data and
.bss. It is absent from binary file contents and startup clearing/copying.
C global zero-initializer semantics must not be relied on for these buffers:
contents are undefined until the application explicitly writes them.
Code, ordinary globals, C heap and RTOS heap/stacks remain in internal SRAM.
The linker rejects allocations beyond the 32MiB PSRAM window.

Startup sequence: Secure PSRAM init/map/permissions -> NS scheduler -> default
thread Secure context -> AppRamTest_Run (optional full test) -> AppBuffers_Init
-> normal 10ms loop. Init validates every buffer range, 64-byte alignment and
pairwise non-overlap before touching RAM, then writes/reads first and last words
of each reservation and clears those words. This test is startup-only and must
not run while camera/NPU/other tasks use the buffers. It does not clear the whole
buffer. Failures leave app_buffers_ready=0 and accessors return NULL; the basic
LED/log application continues.

Example (task context after startup):

```c
const AppBuffer *frame = AppBuffers_Get(APP_BUFFER_CAMERA_0);
if (frame != NULL) {
  /* Fill frame->data before reading it; capacity is frame->size bytes. */
}
```

AppBuffers_Get only provides storage. Camera frame ownership, queue handoff,
double-buffer scheduling and NPU memory formats are future integration work.
Do not concurrently reuse a frame that DMA/NPU/another task owns.
NS DCache remains disabled; enabling it makes this bring-up initializer reject
access until a coherent buffer policy is implemented. Alignment alone does not
establish DMA/NPU reachability or coherence. DMA/NPU RIF/CID permissions, MPU
attributes, cache maintenance and accelerator-specific alignment requirements
must be verified with the actual peripheral pipeline.

01 Build and Verify calls Sync-RamLayout.ps1 to restore the PSRAM MEMORY region
and NOLOAD section if CubeMX regenerates the selected linker script. After a
Generate, run 01 before CubeIDE build/debug. The checked-in linker already has
this layout, so another PC can build it without regeneration. Other linker
variants are not configured for these buffers.

Validation: scripted build/package checks and CubeIDE Debug link passed;
objdump reports .external_ram as ALLOC without LOAD/CONTENTS, 64-byte aligned;
nm confirms all five buffers fit in 0x90000000..0x91FFFFFF. The signed NS image
is 62,688 bytes, so buffer reservations do not inflate the NOR image.
Hardware validation passed on 2026-10-05: all five addresses and sizes match the table; range, alignment, overlap checks and CPU first/last-word probes succeeded. UART reported READY, reserved=4797440 and free=28756992. Camera/DMA/NPU and cache coherency are not tested; this result does not validate full buffer contents.

