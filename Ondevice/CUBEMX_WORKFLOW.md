# CubeMX 코드 생성과 수동 코드 유지

## 생성 후 개발 흐름

1. CubeMX에서 핀·주변장치·초기화 context·RIF 권한을 설정하고 저장한다.
2. Generate Code를 실행한다. **Keep User Code when re-generating**은 켜 둔다.
3. CubeIDE 루트와 하위 프로젝트를 Refresh(F5)한다.
4. **01 Build and Verify**를 실행한다. 활성화된 HAL 소스 링크를 자동 보완하고 부팅 필수
   설정을 검사한 뒤 빌드·패키지 검증을 수행한다. 일반 망치 버튼에는 이 스크립트 검사가 없다.
5. SUCCESS 이후 같은 bundle을 기록하고 보드에서 기능을 확인한다.

매번 수동으로 전체 부팅 파일을 비교할 필요를 줄이기 위한 구조다. 검사는 알려진 필수 설정의
손실을 찾으며 임의의 모든 CubeMX 변경이 안전하다고 보장하지 않는다. 클록·메모리 배치·
보안 정책·CubeMX/펌웨어 패키지 버전을 변경할 때는 해당 변경과 실제 부팅을 검토한다.

## 파일 소유권

| 파일 | 수정 원칙 |
| --- | --- |
| CubeMX의 main.c, MSP, IRQ 등 | 수동 추가는 USER CODE 영역에 둔다 |
| FSBL xspi.c, NS usart.c 등 | 핀·클록·주변장치 초기화 값은 `.ioc`를 기준으로 생성한다 |
| FSBL fsbl_app.c, nor_flash.c, fsbl_image.h | 프로젝트 전용 로더; CubeMX 생성 파일이 아니다 |
| Secure secure_boot.c/h | NS 진입·단계 진단을 유지하는 프로젝트 전용 코드 |
| Secure partition_stm32n657xx.h | 현재 SRAM/NSC 배치가 반영된 파일; 스크립트 검사와 컴파일 정적 검사로 보호 |

Secure main의 USER CODE 2에서 `Secure_BootEnterNonSecure()`를 호출한다. 이 함수가
Secure SysTick 정지와 NS VTOR/MSP 설정, 단계 3~6 진단, NS Reset Handler 호출을 맡고
복귀하지 않는다. 따라서 그 뒤에 남은 CubeMX 기본 NonSecure_Init 호출 경로는 실행하지 않는다.
RIF 초기화와 LED2 준비는 이 호출 전에 수행한다.

새 HAL 모듈은 `Sync-HalLinks.ps1`이 각 프로젝트의 `stm32n6xx_hal_conf.h`에서 활성화된
모듈을 읽고 공용 드라이버의 기본·확장 소스 링크를 `.project`에 추가한다. 기존 링크는 지우지
않는다. IDE의 기존 workspace가 링크를 반영하지 못하면 해당 프로젝트를 Close/Open하고
Refresh·Clean한다. 스크립트 빌드는 IDE의 캐시된 소스 목록을 사용하지 않는다.

## 자동 점검과 실패 처리

`Check-CubeMX-Boot.ps1`은 다음 필수 설정을 검사한다.

- USER CODE 내부 FSBL 헤더·이미지 로딩·READY 조건·Secure 이동 호출
- NOR용 VDDIO3 1.8V와 XSPI2 Port2/NCS1/PLL1 IC3 분주 설정
- `.ioc`와 생성 코드의 AHB 분주 일치
- USER CODE 내부 Secure 진입·RISAF 메모리 설정 유지
- SRAM2 NS, 주변장치 NS, 링커 veneer 기반 NSC SAU 설정

실패하면 빌드를 중단하고 새 manifest를 만들지 않는다. 잘못된 코드에 기본 설정을 자동으로
덮어씌우지는 않는다. `Boot guard` 메시지의 설정을 `.ioc` 또는 해당 USER CODE 영역에서 수정한다.
의도적으로 메모리 배치 등을 바꾸는 경우 검사 기준도 함께 변경하고 보드에서 다시 검증한다.

```powershell
pwsh -NoProfile -File .\Tools\Check-CubeMX-Boot.ps1
pwsh -NoProfile -File .\Tests\Test-CubeMX-Boot.ps1
```

## 2026-10-04 USART1 추가 후 검토 결과

- FSBL의 수동 `nor_flash.h`·`fsbl_app.h` include가 생성 영역에 있어 삭제됐다.
  USER CODE Includes로 이동해 복구했다.
- Secure 단계 진단이 생성 영역에 있어 일부 삭제됐다. USER CODE 호출과 별도 secure_boot.c로
  이동해 재생성에 영향을 받지 않도록 했다.
- 기존 XSPI 초기화는 같은 설정의 CubeMX 생성 코드로 대체됐다. VDDIO3 USER CODE,
  NOR 로더, SAU·RISAF, LED·Fault 진단은 유지됐다.
- USART1은 PE5/PE6, AF7, 115200 8N1, NS 초기화로 생성됐다. Secure USER CODE에서
  USART1 NS/nonprivileged 권한을 명시한다. GPIO NS 속성은 생성된 RIF 설정에 들어 있다.
- AHB 분주는 기존 수동 소스 DIV2(200MHz)에서 `.ioc`의 DIV4(100MHz)로 바뀌었다.
  현재 소스와 `.ioc`를 일치시켜 유지한다. XSPI IC3 50MHz는 변하지 않았다.
- FSBL 소스도 바뀌었으므로 새 FSBL을 검증한 뒤 최초 한 번 `-IncludeFsbl`로 기록해야
  SRAM 디버깅과 NOR 독립 부팅에서 같은 클록 구성을 사용한다. 그 이후 일반 앱 갱신은 02로 한다.

위 보완 후 사용자에게 실제 Generate Code를 다시 실행하도록 요청했고, 두 번째 생성에서도
USER CODE 훅·전용 Secure 진입 코드·UART HAL 링크가 유지되는 것을 확인했다.
부팅 필수 설정 검사, 손실 주입 테스트, 전체 빌드·이미지 검증이 통과했다.
변경 후 세 이미지를 기록·검증하고, 정상 L/L 전원 재인가 후 사용자 설정인 7500ms LED2
동작을 확인했다. 이어서 UART 로그 앱을 기록·검증하고 사용자가 1초 주기 로그 출력을 확인했다.
USART1/PE5/PE6의 NS 접근과 초기화·송신, ST-LINK VCP 경로가 보드에서 동작했다.
UART 송신 오류 카운터 값과 장시간 동작은 별도 검증 범위다. 이전에 기록한 NOR 이미지는
자동으로 바뀌지 않는다. 빌드와 ELF가 바뀌면 새 bundle을 기록한 뒤 NS 디버깅한다.

## RTOS UART logger

`AppLog_Init()` creates a 16-record queue and a `uartLog` task in the CubeMX
`RTOS_QUEUES` USER CODE block. The task has 2048 bytes of stack and BelowNormal
priority. These hooks survive regeneration; `app_log.c/h` are project-owned.

The default task remains Normal priority with a 10ms release interval and the
existing 7500ms LED interval. Once per second it copies a status snapshot into
the queue. Formatting and UART transmission run exclusively in `uartLog`.
`AppLog_Write(text)` copies up to 95 bytes, accepts task-context calls only,
and returns immediately (1 accepted, 0 rejected). Add CRLF when needed.
It does not retain the caller's buffer. Do not call it from an interrupt.

A full queue drops the newest message instead of blocking the producer.
`dropped` counts rejected records, `queue_peak` reports observed queued depth,
and `log_stack_words` reports the UART task's minimum remaining stack in
32-bit words. `stack_words` still refers to the default task. Heap usage now
includes the queue and log task. The log task uses bounded HAL polling transmit
(100ms timeout), so it still consumes CPU during transmission; DMA or interrupt
transmit is a later improvement. Other tasks must not transmit via USART1.

Validation: firmware build/package checks and CubeIDE build must pass before
programming. Hardware acceptance: HAL/OS ticks advance by 1000 per status line,
default task runs advance by about 100, both stacks retain margin, and
`tx_errors=0`, `dropped=0` remain stable. Hardware validation is pending for this
logger change.
