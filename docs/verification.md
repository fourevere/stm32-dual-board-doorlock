# 빌드 확인 기록

시험일: 2026.09.12. 원본과 공개 사본 각각을 새 격리 폴더에 복사하여 빌드했습니다.

| 대상 | 원본 exit | 공개 사본 exit | 공개 경고/오류 | 공개 BIN |
| --- | --- | --- | --- | --- |
| BOARD_A | 0 | 0 | 0 / 0 | 28,208 bytes |
| BOARD_B | 0 | 0 | 0 / 0 | 27,612 bytes |

Arm GNU Toolchain 15.2.Rel1 / gcc 15.2.1, GNU Make 3.81, Cortex-M4 / STM32F411xE, GNU99 -O3 -Wall.

실행 명령은 `make all BOARD_DEF=-DBOARD_A`와 `make all BOARD_DEF=-DBOARD_B`이며, 하드웨어 플래싱·실행은 하지 않았습니다. 현재 컴파일 결과는 7월 당시 시험 결과가 아닙니다. ELF data / bss는 동적 최대 RAM 사용량과 다릅니다.

공개 값으로 변경한 main.c 외 나머지 소스 파일은 원본과 바이트 단위로 같습니다. 빌드 로그와 산출물의 SHA256은 현재 결과를 추적하기 위한 것입니다. 서로 다른 위치에서 -g 옵션을 포함해 재빌드하면 경로 정보 때문에 ELF는 달라질 수 있습니다.

상세 결과: [build-results.json](build-results.json)

## 보관한 공개 사본 빌드 로그

- [BOARD_A 빌드 로그](build-board-a.log)
- [BOARD_B 빌드 로그](build-board-b.log)

로그는 명령·컴파일·링크·objcopy 결과입니다. 장치 출력 로그가 아닙니다.
