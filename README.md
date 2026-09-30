# ATmega128 POV Display — 회전 LED 잔상 디스플레이

세로 한 줄로 배치한 **RGB LED 16개**를 모터로 회전시키고, 회전에 맞춰 열(column) 데이터를 빠르게 바꿔 **잔상(POV, Persistence of Vision)으로 글자와 그림을 공중에 그리는** 디스플레이입니다.
마이크로프로세서 수업 텀프로젝트로 제작했으며, LED 구동 PCB 설계 → 펌웨어 → 3D 프린팅 기구 설계까지 직접 구현했습니다.

| 항목 | 내용 |
|---|---|
| MCU | ATmega128 × 2 (LED 보드 / 모터 보드) |
| LED | RGB LED 16개 (16행 × R·G·B = 48채널) |
| LED 드라이버 | 74HC595 시프트 레지스터 6개 직렬 연결 (48비트) |
| 모터 | DC 모터 + L298N 모터 드라이버, Timer0 Fast PWM 속도 제어 |
| 입력 | 택트 스위치 → 외부 인터럽트 (LED 4개 + 모터 3개) |
| 기능 | 6색 전환 · 6단계 밝기 · 패턴 2종(글자 `ISA` / 하트) · 모터 속도 ±, 일시정지 |
| 개발 환경 | CodeVisionAVR (C), KiCad 9, 3D 모델링(STL) |

---

## 시스템 구조

<p align="center"><img src="docs/images/block_diagram.png" width="55%"></p>

```mermaid
flowchart LR
    subgraph PWR["전원"]
        AD[12V 어댑터]
    end
    subgraph MB["모터 보드 (ATmega128 #2)"]
        SW2["스위치 ×3<br/>INT0~2"] --> M128B[ATmega128]
        M128B -->|"OC0 (PB4) PWM"| L298[L298N]
        M128B -->|"PA0/PA1 방향"| L298
    end
    subgraph ROT["회전부 (자체 PCB, ATmega128 #1)"]
        SW1["스위치 ×4<br/>INT0~3"] --> M128A[ATmega128]
        M128A -->|"PA0 SER / PA1 SRCLK / PA2 RCLK"| SR["74HC595 ×6<br/>(48비트 체인)"]
        SR --> LED["RGB LED ×16"]
    end
    AD --> L298
    L298 -->|12V + PWM| MOT[DC 모터]
    L298 -->|5V| M128B
    MOT -. 회전 .-> ROT
```

### 동작 원리

1. **LED 한 열 출력** — 16비트 열 패턴을 색상에 따라 R/G/B 채널에 배치하고, B → G → R 순서로 48비트를 시프트한 뒤 RCLK로 한 번에 래치
   - 비트가 `0`이면 LED 켜짐(Active-Low), `0xFFFF`는 전체 소등
2. **밝기 (소프트웨어 PWM)** — 한 열을 `uptime` ms 동안 켜고 `7 - uptime` ms 동안 꺼서, 7 ms 주기 안에서 듀티비를 1/7 ~ 6/7로 조절
3. **패턴** — 회전하는 동안 열 데이터를 순서대로 바꿔 출력하면 잔상으로 2차원 이미지가 보임
   - `ISA` 글자: 24열 (글자당 8열), 하트: 12열
4. **모터 속도** — Timer0 Fast PWM의 `OCR0` 값(0~254)을 인터럽트로 ±10씩 바꿔 L298N ENA에 입력

### 제어 흐름

<p align="center"><img src="docs/images/flowchart.png" width="70%"></p>

### 핀 배치

**LED 보드 (ATmega128 #1)**

| 핀 | 방향 | 용도 |
|---|---|---|
| PA0 | 출력 | 74HC595 SER / DS (직렬 데이터) |
| PA1 | 출력 | 74HC595 SRCLK / SHCP (시프트 클럭) |
| PA2 | 출력 | 74HC595 RCLK / STCP (래치 클럭) |
| PD0 (INT0) | 입력, 풀업 | 색상 전환 (빨 → 초 → 파 → 노 → 마젠타 → 청록) |
| PD1 (INT1) | 입력, 풀업 | 밝기 변경 (1 ↔ 6 단계 왕복) |
| PD2 (INT2) | 입력, 풀업 | 패턴 1: `ISA` 글자 |
| PD3 (INT3) | 입력, 풀업 | 패턴 2: 하트 |

**모터 보드 (ATmega128 #2)**

| 핀 | 방향 | 용도 |
|---|---|---|
| PB4 (OC0) | 출력 | PWM → L298N ENA |
| PA0 / PA1 | 출력 | L298N IN1 / IN2 (회전 방향 고정) |
| PD0 (INT0) | 입력, 풀업 | 속도 +10 (최대 254) |
| PD1 (INT1) | 입력, 풀업 | 속도 −10 (최소 1) |
| PD2 (INT2) | 입력, 풀업 | 일시정지 / 재시작 (직전 속도 복원, 첫 시작 160) |

인터럽트는 모두 하강 에지 트리거(`EICRA`)입니다.

---

## 폴더 구조

```
atmega128-pov-display/
├─ firmware/                  CodeVisionAVR C 소스
│  ├─ led_board/main.c        74HC595 48비트 시프트 출력, 색상·밝기·패턴 제어
│  └─ motor_board/main.c      Timer0 Fast PWM 모터 속도 제어
├─ hardware/
│  └─ pcb/                    KiCad 9 프로젝트 (LED 보드)
│     ├─ MPMP.kicad_pro / .kicad_sch / .kicad_pcb
│     └─ gerber/              제작용 Gerber + 드릴 파일
├─ mechanical/                3D 프린팅 부품 (STL)
│  ├─ rotor_plate.stl         회전판 (PCB 장착)
│  ├─ rotor_cover_v2.stl      회전판 덮개
│  ├─ support_upper.stl       지지대 (위)
│  ├─ support_lower.stl       지지대 (아래)
│  ├─ base.stl                받침대
│  └─ base_support.stl        받침대 하부 받침
└─ docs/images/               블록 다이어그램, 플로우차트
```

---

## 하드웨어

### LED 보드 PCB

- **크기**: 90.3 × 87.2 mm, 2층, 두께 1.6 mm (KiCad 9)
- **구성**
  - ATmega128-16A (TQFP-64) + 크리스털, 22 pF 부하 커패시터, 리셋 스위치
  - 74HC595 × 6 (R·G·B 각 2개, 채널당 16비트) + 칩별 0.1 µF 바이패스 커패시터
  - 8연 저항 네트워크(R_Network08) × 2
  - LED 연결 패드 48개 (R/G/B × 16), 10핀 ISP 헤더 (PE0 / PE1 / PB1 / RESET)
  - 고정용 마운팅 홀 4개
- 48채널 LED 배선을 회전체에 싣기 위해 MCU와 시프트 레지스터를 한 장의 보드로 통합

### 기구부

- 회전판에 PCB와 LED를 고정하고, 모터 축에 연결해 회전
- 지지대(위/아래)와 받침대로 모터와 회전부를 고정
- 회전부 전원은 슬립링을 통해 공급

---

## 내 담당 범위

4인 팀 프로젝트였으며, 아래 항목을 **모두 직접** 설계·구현했습니다.

| 파트 | 내용 |
|---|---|
| 회로 / PCB | LED 보드 회로도 설계, PCB 아트웍, Gerber 출력 및 제작 |
| 펌웨어 | LED 보드(시프트 레지스터 구동, 색상·밝기·패턴 제어), 모터 보드(PWM 속도 제어) |
| 기구 | 회전판, 덮개, 지지대, 받침대 3D 모델링 및 출력 |
| 설계 문서 | 블록 다이어그램, 제어 플로우차트, 핀 배치 |

---

## 트러블슈팅

> 추후 추가 예정

---

## 실행 방법

### 1. 펌웨어 빌드 및 업로드
1. CodeVisionAVR에서 새 프로젝트 생성 → Chip: **ATmega128**, 클럭은 보드 크리스털에 맞게 설정
2. LED 보드는 `firmware/led_board/main.c`, 모터 보드는 `firmware/motor_board/main.c`를 프로젝트에 추가
3. Build 후 ISP 프로그래머로 각 보드에 업로드
   - `mega128.h`, `delay.h`는 CodeVisionAVR에 포함된 헤더입니다

### 2. 조작
| 보드 | 스위치 | 동작 |
|---|---|---|
| 모터 | INT2 | 모터 시작 / 일시정지 (전원 투입 시 정지 상태) |
| 모터 | INT0 / INT1 | 속도 증가 / 감소 |
| LED | INT2 / INT3 | `ISA` 글자 / 하트 패턴 |
| LED | INT0 | 색상 전환 (6색) |
| LED | INT1 | 밝기 변경 |

LED 보드는 전원 투입 시 R → G → B를 0.3초씩 켜서 LED 상태를 확인한 뒤 대기합니다.

### 3. PCB 제작
- `hardware/pcb/gerber/` 폴더 전체를 압축해 PCB 제작 업체에 업로드
- 설계 수정은 KiCad 9에서 `hardware/pcb/MPMP.kicad_pro` 열기

### 4. 기구 출력
- `mechanical/*.stl`을 슬라이서에서 열어 3D 프린터로 출력

---

## 기술 스택
`C (CodeVisionAVR)` `AVR ATmega128` · `74HC595` `L298N` · `KiCad 9` · `3D 모델링`

## 라이선스 / 출처
- 이 저장소의 코드, 회로 및 기구 설계는 모두 직접 작성했으며 외부 코드는 포함하지 않았습니다.
- `mega128.h`, `delay.h`: CodeVisionAVR 컴파일러 기본 헤더 (저장소에 미포함)
- PCB 회로도/풋프린트: KiCad 기본 라이브러리 심볼·풋프린트 사용 (CC-BY-SA 4.0, 설계물에 대한 예외 조항 적용)
