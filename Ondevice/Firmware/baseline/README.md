# 검증된 기초 이미지

2026-10-03 BOOT0=L/BOOT1=L 전원 재인가 후 LED2 점멸을 확인한 이미지 세트다.
FSBL은 실제 크기 기반 복사·체크섬 검증 버전이다. 주소·길이·벡터·SHA256은 manifest.json에 있다.

- FSBL-trusted.bin: 0x70000000
- Secure-trusted.bin: 0x70100000
- NonSecure-trusted.bin: 0x70180000

프로젝트의 일반 빌드 출력은 Build/bundle이며 이 baseline을 덮어쓰지 않는다.
복구는 Tools/Program-Firmware.ps1 -Bundle Firmware/baseline -IncludeFsbl로 수행한다.
개발 부트로 전원 재인가하고 디버깅을 종료한 뒤 실행한다.

original-fsbl-sector.bin은 업데이트 전 NOR 0x70000000의 64 KiB 백업이다.
FSBL-fixed-copy-trusted.bin은 최초 전체 독립 부팅을 검증한 고정 64 KiB 복사 버전이다.
둘은 기록 보존용이며 현재 앱 개발의 기본 배포 세트에는 포함하지 않는다.
