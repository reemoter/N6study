# 새 Windows PC에서 개발 환경 설치

이 문서는 저장소를 받은 다른 PC에서 STM32N6570-DK 펌웨어를 빌드하고, 외장 NOR에
기록하고, NonSecure 앱을 디버깅하기 위한 절차다. 검증 기준일은 2026-10-04다.
기존 PC에서 빌드·NOR 기록·독립 부팅·NS 하드웨어 중단점·Resume 후 LED 점멸을 확인했다.
새 PC에서의 최종 동작은 아래 절차로 다시 확인한다.

## 1. 필요한 도구 설치

| 도구 | 프로젝트에서 검증한 환경 | 설치 안내 |
| --- | --- | --- |
| STM32CubeIDE, Eclipse 기반 | 2.2.0 | [ST 공식 다운로드](https://www.st.com/en/development-tools/stm32cubeide.html) |
| STM32 GNU 컴파일러 | CubeIDE 포함 14.3.rel1, GCC 14.3.1 | CubeIDE의 `plugins/...gnu-tools-for-stm32.../tools/bin` |
| STM32CubeProgrammer | 2.23.0 | [ST 공식 다운로드](https://www.st.com/en/development-tools/stm32cubeprog.html) |
| PowerShell | 7.x, 기존 PC 7.6.5 | [Microsoft 설치 안내](https://learn.microsoft.com/en-us/powershell/scripting/install/install-powershell-on-windows) |
| Git | 저장소 clone/pull에 사용 | Git 클라이언트 또는 기존 Git GUI 사용 |

같은 버전으로 시작하면 재현하기 쉽다. 다른 버전은 빌드 성공과 보드 동작을 별도로 확인한다.
이 저장소의 `.launch`는 Eclipse 기반 CubeIDE용이며 VS Code용 설정은 아니다.
Windows PowerShell 5.1의 `powershell.exe`와 PowerShell 7의 `pwsh.exe`는 다르다.
PowerShell 7은 일반 설치 경로 `C:\Program Files\PowerShell\7` 또는 PATH에 있어야 한다.
Codex 설치는 필요 없다.

CubeIDE/CubeProgrammer 설치 시 ST-LINK 드라이버도 설치한다. 보드는 데이터 통신 가능한
USB 케이블로 ST-LINK USB 포트에 연결한다. 처음에는 ST-LINK 한 개만 연결한다.

CubeProgrammer 설치 폴더에 아래 파일이 있는지 확인한다.

```text
bin/STM32_Programmer_CLI.exe
bin/STM32_SigningTool_CLI.exe
bin/ExternalLoader/MX66UW1G45G_STM32N6570-DK.stldr
```

Visual Studio/MSVC는 펌웨어 빌드·다운로드에 필요하지 않다. `Tests/run_fsbl_image_tests.cmd`의
호스트 C 테스트를 실행하려는 경우에만 해당 스크립트가 사용하는 MSVC 환경이 필요하다.

## 2. 저장소 받고 프로젝트 루트 확인

푸시된 저장소를 clone한다. 기존 PC의 절대 경로와 같을 필요는 없다.
이 문서에서 **프로젝트 루트**는 다음 파일과 폴더가 있는 디렉터리다.

```text
.project                    # 프로젝트 이름: Ondevice
Ondevice.ioc
README.md
Tools/
Drivers/
FSBL/
AppliSecure/
AppliNonSecure/
Firmware/baseline/
```

Git 저장소 최상위와 프로젝트 루트가 다를 수 있다. 예를 들어 저장소 안에 `Ondevice` 폴더가
있다면 그 안으로 들어간다. 터미널에서 `Test-Path .\Tools\Build-Firmware.ps1` 결과가
`True`여야 아래 명령을 실행할 위치가 맞다.

`Debug`, `Release`, `Build` 산출물은 Git에서 제외되어 있다. 새 PC에서는 먼저 빌드해야
다운로드 bundle과 디버거 ELF가 생긴다. 기존 PC의 Debug 폴더를 복사할 필요는 없다.

## 3. 도구 경로 설정

현재 스크립트의 기본 탐색 위치는 다음과 같다.

| 항목 | 기본 위치 |
| --- | --- |
| 컴파일러 | `C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins` 아래 GNU tools의 `tools/bin` |
| Programmer | `%ProgramFiles%\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin` |

설치 버전이나 경로가 다르면 Windows의 **사용자 환경변수**에 아래 두 변수를 등록한다.
값은 폴더 경로이며, 따옴표를 값에 포함하지 않는다.

```text
ST_GCC_BIN        = arm-none-eabi-gcc.exe가 있는 tools\bin 폴더
ST_PROGRAMMER_BIN = STM32_Programmer_CLI.exe가 있는 bin 폴더
```

PowerShell로 영구 등록할 수도 있다. 아래 예시 경로는 실제 설치 경로로 교체한다.

```powershell
[Environment]::SetEnvironmentVariable('ST_GCC_BIN', 'D:\ST\STM32CubeIDE\plugins\실제_GNU_도구_폴더\tools\bin', 'User')
[Environment]::SetEnvironmentVariable('ST_PROGRAMMER_BIN', 'D:\ST\STM32CubeProgrammer\bin', 'User')
```

환경변수를 변경한 뒤 CubeIDE와 터미널을 모두 종료하고 다시 연다. 이미 실행 중인 CubeIDE에는
변경된 사용자 환경변수가 적용되지 않는다. PowerShell에서 `pwsh -NoProfile -Command
'$PSVersionTable.PSVersion'`이 실행되는지도 확인한다.

## 4. CubeIDE에 Import

1. 새 CubeIDE workspace를 연다.
2. **File → Import → General → Existing Projects into Workspace**를 선택한다.
3. 프로젝트 루트를 선택한다. 필요하면 nested project 검색을 켜거나 하위 폴더도 각각 Import한다.
4. 최소 다음 네 프로젝트를 가져온다: `Ondevice`, `Ondevice_FSBL`,
   `Ondevice_AppliSecure`, `Ondevice_AppliNonSecure`.
5. **Copy projects into workspace는 끈다.** 저장소의 폴더 관계를 그대로 유지한다.
6. 루트와 세 하위 프로젝트를 Refresh(F5)한다.

루트 프로젝트 이름 **`Ondevice`는 유지**한다. External Tools의 `${workspace_loc:/Ondevice}`가
이 이름을 사용한다. 공용 HAL은 하위 프로젝트에서 상대 링크로 참조하므로 `Drivers`를
다른 곳으로 옮기거나 하위 프로젝트만 따로 복사하지 않는다.

IDE 자체 빌드도 사용할 경우 **Secure → NonSecure** 순서로 빌드한다. HAL 링크가 누락되면
Close/Open Project, Refresh, Project Clean을 순서대로 수행한다. 계속 누락되면 workspace에서
프로젝트만 제거하고 다시 Import한다. 이때 디스크의 프로젝트 파일 삭제는 선택하지 않는다.

이 설치 절차를 위해 CubeMX에서 Generate Code를 실행할 필요는 없다. XSPI·보안 관련 수동
보완 코드가 있으므로 코드 재생성은 별도 검토 후 수행한다.

## 5. IDE 빌드·다운로드 메뉴 등록 및 첫 빌드

**Run → External Tools → External Tools Configurations → Program**에 다음 설정이 보여야 한다.

| 설정 | 실행 내용 |
| --- | --- |
| `Ondevice 01 Build and Verify` | 세 프로젝트 빌드, 이미지 생성, 생성 bundle 검증 |
| `Ondevice 02 Program Apps` | Secure·NS 앱 기록 및 검증; FSBL 유지 |

목록에 없으면 루트 프로젝트를 Refresh하고 `Tools`의 해당 `.launch` 파일을 열거나
External Tools로 실행한다. Common 탭에서 External Tools 즐겨찾기를 켜면 툴바 메뉴로 실행한다.

설정이 이전 값으로 남아 있으면 다음 값을 확인한다.

```text
Location:          ${env_var:SystemRoot}\System32\cmd.exe
Working Directory: ${workspace_loc:/Ondevice}
01 Arguments:      /d /c Tools\Run-CubeIDE-Tool.cmd BuildAndTest
02 Arguments:      /d /c Tools\Run-CubeIDE-Tool.cmd ProgramApps
Console Encoding:  UTF-8
```

소스를 저장하고 **01**을 실행한다. Console에 다음 문구가 나오면 성공이다.

```text
SUCCESS: build and package tests passed. NOR has not been changed.
```

`Plan only: no target connection or writes.`는 테스트에 포함된 다운로드 계획 검사의 정상
출력이다. 이 단계에서는 보드에 기록하지 않는다. 로그는 `Build/logs`에 남는다.
일반 망치(Build) 버튼은 이미지 생성·패키지 검증·NOR 기록을 대신하지 않는다.

## 6. 보드 기록: 기존 보드와 새 보드 구분

다운로드 전 디버깅을 Terminate하고 CubeProgrammer 연결도 종료한다.
전원 OFF → **BOOT0=L / BOOT1=H** → 전원 ON으로 개발 부트에 진입한다.

### 이 프로젝트의 FSBL이 이미 기록된 보드

**02 Program Apps**를 실행한다. `Downloads verified`가 나오면 두 앱 기록과 검증이 완료됐다.
01과 02를 동시에 실행하지 않는다.

### 새 보드 또는 현재 FSBL과 다른 부트 로더가 들어 있는 보드

02 메뉴는 FSBL을 기록하지 않는다. 01 성공 후 프로젝트 루트의 터미널에서 최초 한 번 실행한다.

```powershell
pwsh -NoProfile -File .\Tools\Program-Firmware.ps1 -IncludeFsbl
```

이 명령은 기존 NOR 부팅 영역 64 KiB를 `Build/backups`에 백업하고, Secure·NS·FSBL을
기록·검증한다. 백업 실패 시 FSBL 교체를 진행하지 않는다. 백업은 해당 PC에만 남으므로 보관한다.
여러 ST-LINK가 연결되어 있으면 한 개만 남기거나 `-SerialNumber`로 지정한다.

| 이미지 | 외장 NOR 기록 주소 |
| --- | --- |
| FSBL | `0x70000000` |
| Secure | `0x70100000` |
| NonSecure | `0x70180000` |

다운로드는 ST 제공 External Loader가 NOR를 기록한다. 부팅 시에는 FSBL이 앱을 SRAM으로
복사한다. 펌웨어는 SRAM에서 실행되며 외장 NOR에서 직접 실행하는 XIP 구성은 아니다.

## 7. 독립 부팅 확인

다운로드 완료 후 전원 OFF → **BOOT0=L / BOOT1=L** → 전원 ON으로 바꾼다.
디버거를 실행하지 않은 상태에서 LED1 유지와 LED2 점멸을 확인한다.
LED2 주기는 현재 빌드한 `AppliNonSecure/Core/Src/main.c`의 heartbeat 조건에 따른다.
예를 들어 `5000U`이면 켜짐·꺼짐이 각각 약 5초이고 전체 주기는 약 10초다.

LED2 점멸은 Boot ROM → FSBL → Secure → NonSecure main까지 실행된 결과다.
LED1 점등만으로 NS 진입 성공을 판단하지 않는다.

## 8. NonSecure 앱 디버깅

실제 기능 개발은 **AppliNonSecure**에서 한다. `Ondevice_FSBL NS Debug`는 보안·클록·GPIO
초기화를 유지하기 위해 FSBL부터 시작하는 실행 설정이다.

1. 반드시 새 PC의 01에서 생성한 bundle을 먼저 보드에 기록한다.
2. 디버깅 종료 → 전원 OFF → **L/H** → 전원 ON.
3. FSBL 프로젝트 Refresh 후 **Run → Debug Configurations → STM32 C/C++ Application →
   Ondevice_FSBL NS Debug**를 선택한다.
4. FSBL은 `../Build/Debug/FSBL/FSBL.elf`를 SRAM에 다운로드한다.
   NS는 `../Build/Debug/AppliNonSecure/NonSecure.elf`의 심볼만 읽고 다운로드하지 않는다.
5. `FSBL_JumpToSecure`에서 정지하면 앱 복사 단계까지 도달했다. NS main 또는 반복문에
   하드웨어 중단점을 설정하고 Resume하여 NS 정지를 확인한다.
6. 중단점을 해제하고 Resume하면 LED2 점멸이 재개되는지 확인한다.

중단점 주소는 재빌드에 따라 바뀐다. README의 과거 숫자를 그대로 재사용하지 않는다.
프로젝트 루트 PowerShell에서 현재 ELF의 main 주소를 조회할 수 있다.

```powershell
. .\Tools\Firmware.Common.ps1
$armBin = Find-ArmToolchain ''
& (Join-Path $armBin 'arm-none-eabi-nm.exe') .\Build\Debug\AppliNonSecure\NonSecure.elf |
    Select-String ' main$'
```

Debugger Console에 `hbreak *0x조회한주소` 형태로 입력한 뒤 **Enter**를 누른다.
`Hardware assisted breakpoint` 응답과 `info breakpoints` 목록을 확인하고 Resume한다.
main 시작점은 한 번만 지나므로 이미 루프 실행 중이면 반복문 위치에 중단점을 설정한다.
중단점 설정 시 코어가 실행 중이라면 먼저 Suspend한다.

01을 다시 실행하면 ELF가 갱신된다. NOR에 이전 이미지가 남아 있으면 심볼과 실행 코드가
맞지 않으므로 02로 새 bundle을 기록한 뒤 디버깅한다. 기존 IDE의 `Debug/*.elf`와 혼용하지 않는다.

기존 `.launch`에 이전 PC의 ST-LINK 로그 경로가 남아 있을 수 있다. 로그를 활성화하려면
Debugger 설정에서 새 PC의 쓰기 가능한 경로로 변경한다. 보드 일련번호 고정 옵션은 꺼 두거나
새 보드에 맞게 변경한다. 기존 PC의 정상 L/L 부팅 후 Attach는 Core ID 읽기에 실패했으므로,
검증된 경로는 **개발 부트 L/H에서 FSBL부터 디버깅하는 방식**이다.

## 9. 자주 발생하는 문제

| 증상 | 확인·대응 |
| --- | --- |
| `pwsh`를 찾지 못함 | PowerShell 7 설치, 새 터미널/IDE 실행; 필요 시 전체 경로 사용 |
| GNU tools 탐색 실패 | `ST_GCC_BIN` 값이 GCC 실행 파일이 있는 bin인지 확인 |
| Programmer/Signing Tool/loader 없음 | CubeProgrammer bin과 설치 파일 확인, `ST_PROGRAMMER_BIN` 지정 |
| 메뉴의 경로 변수 오류 | 루트 `Ondevice` Import, 프로젝트 이름과 Working Directory 확인 |
| 파일/디렉터리 이름 구문 오류 | 위 Arguments 값으로 수정하고 Apply; 중첩 따옴표로 된 이전 설정 제거 |
| Console 글자 깨짐 | Common의 Encoding을 UTF-8로 설정; `Build/logs` 확인 |
| `Build/bundle/manifest.json` 없음 | 먼저 01 실행; Build 산출물은 Git에 포함되지 않음 |
| HAL 함수 undefined reference | HAL 링크 확인, Close/Open·Refresh·Clean 또는 재Import |
| `No device found on target` / Core ID 오류 | 다른 연결 종료, L/H로 완전 전원 재인가, ST-LINK 포트·케이블·드라이버 확인 |
| NS main에 진입하지만 LED 없음 | NS 단독 Debug 대신 FSBL NS Debug 사용; Suspend 상태인지 확인 |
| 중단점이 걸리지 않음 | 명령 Enter/응답 확인, 현재 ELF 주소와 NOR 이미지 일치 확인 |

## 10. 설치 완료 확인

- 01이 SUCCESS로 종료되고 `Build/bundle/manifest.json`이 생성된다.
- 새 보드는 FSBL 포함 최초 기록, 기존 보드는 02 기록이 검증 완료된다.
- L/L 전원 재인가만으로 LED2가 점멸한다.
- L/H + FSBL NS Debug에서 NS 중단점으로 정지한다.
- 중단점 해제 후 Resume하면 LED2가 다시 점멸한다.

이후에는 NS 수정·저장 → 01 → L/H 전원 재인가 → 02 → 디버깅 또는 L/L 독립 실행을 반복한다.
부팅 구조와 메모리 배치는 [README.md](README.md), 과거 검증 기록은
[BOOT_VALIDATION.md](BOOT_VALIDATION.md)를 참고한다.
