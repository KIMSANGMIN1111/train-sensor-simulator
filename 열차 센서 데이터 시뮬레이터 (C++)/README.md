# 🚆 열차 센서 데이터 시뮬레이터

> Portfolio Project #2 — C++ 객체지향 설계 실습

---

## 프로젝트 개요

열차 운행 중 발생하는 실시간 센서 데이터를 시뮬레이션하고,
이상 상태를 감지하여 이벤트 기반 알람을 발생시키는 시스템입니다.
열차제어 시스템 소프트웨어(TCMS)의 핵심 기능인 센서 데이터 수집·분석·경보를 C++로 구현했습니다.

---

## 구현 기능

| 기능 | 설명 |
|------|------|
| 센서 모델링 | 속도, 온도, 제동 압력, 진동 센서 클래스 |
| 상태 감지 | NORMAL / WARNING / CRITICAL / FAULT 4단계 |
| 이벤트 알람 | 콜백 기반 AlarmManager, 중복 알람 억제 |
| 시나리오 재현 | 출발→정속→과열→제동→진동이상→센서고장→복구 |
| 노이즈 모델 | Box-Muller 정규분포 노이즈로 실측값 재현 |

---

## 파일 구조

```
train_simulator/
├── sensor.h      — 센서 기본/파생 클래스 선언
├── sensor.cpp    — 센서 로직 구현
├── alarm.h       — 알람 이벤트 시스템 선언
├── alarm.cpp     — 알람 매니저 구현
├── main.cpp      — 시뮬레이터 & 시나리오
└── README.md
```

---

## 빌드 및 실행

```bash
# 컴파일
g++ -std=c++17 -o train_sim main.cpp sensor.cpp alarm.cpp -lm

# 실행
./train_sim
```

---

## 시나리오 흐름

```
Tick  0~ 9   역 출발 — 0 → 80 km/h 가속
Tick 10~19   정상 주행 80 km/h
Tick 20~24   모터 과열 시작 → WARNING/CRITICAL 알람
Tick 25~29   고속 구간 120 km/h
Tick 30~34   제동 구간 — 제동 압력 상승
Tick 35~41   차축 진동 이상 → WARNING 알람
Tick 40~44   속도 센서 고장 주입 → FAULT 알람
Tick 45~49   복구 및 정상화
```

---

## 적용 기술

- **C++17** — 스마트 포인터(`shared_ptr`), 람다, `std::function`
- **OOP** — 추상 클래스, 상속, 다형성(polymorphism)
- **이벤트 패턴** — 콜백 등록 방식 AlarmManager
- **수치 모델링** — Box-Muller 정규분포 노이즈
- **V-Model** 고려 — 각 클래스별 단위 테스트 가능한 구조 설계
