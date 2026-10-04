# 펌웨어 검증

`run_fsbl_image_tests.cmd`는 검증된 Firmware/baseline의 실제 이미지를 읽는다.
Visual Studio 2022 Community의 C compiler를 사용하며, CubeIDE/하드웨어가 없어도 실행된다.
MSVC 경로가 다르면 batch의 vcvars64 위치를 해당 설치에 맞춘다.

| 테스트 | 검증 범위 |
| --- | --- |
| fsbl_image_test.c | 현재 파일 길이·헤더·체크섬·벡터, 상한/초과/overflow, 틀린 header, 스택 정렬과 Reset 범위 |
| fsbl_loader_test.c | production fsbl_app.c를 사용, exact copy와 canary, 마지막 부분 전송, 64 KiB 초과, 읽기 실패, payload 손상, 쓰기 전 두 앱 preflight |
| Tools/Test-Firmware.ps1 | 배포 bundle의 SHA256·메타데이터·범위, payload 손상/잘못된 Flash 슬롯 거부, PlanOnly |
| Test-CubeMX-Boot.ps1 | 복사한 소스에서 FSBL include 삭제, NOR 전압 변경, Secure 진입 삭제, NS SRAM SAU 비활성화를 주입하여 부팅 검사 거부 확인 |

CubeMX 재생성 관련 검사는 `pwsh -File .\Tests\Test-CubeMX-Boot.ps1`로 실행한다.
01 빌드는 `Tools/Check-CubeMX-Boot.ps1`을 자동 실행하며, 실패 시 패키지 생성을 중단한다.

HAL stub은 테스트 프로세스 안에서 cache/레지스터 동작을 모의한다. CPU/캐시/보안 하드웨어의
실제 동작은 대체하지 않는다. 이 부분은 SRAM 디버깅과 독립 부팅으로 별도 검증한다.

기능을 추가하면 해당 동작의 테스트도 추가한다. GPIO/DMA/IRQ/SAU/RISAF 변경은 실제 보드에서
검증한다. 현재 테스트들은 Flash erase/write나 OTP 작업을 하지 않는다.
