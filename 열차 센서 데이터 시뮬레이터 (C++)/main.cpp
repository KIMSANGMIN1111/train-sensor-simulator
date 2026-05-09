// ============================================================
// main.cpp — 열차 센서 데이터 시뮬레이터 메인
//
// [실행 방법]
//   g++ -std=c++17 -o train_sim main.cpp sensor.cpp alarm.cpp
//   ./train_sim
//
// [프로젝트 구조]
//   sensor.h / sensor.cpp   — 센서 기본·파생 클래스
//   alarm.h  / alarm.cpp    — 이벤트 기반 알람 시스템
//   main.cpp                — 시뮬레이터 & 시나리오
// ============================================================

#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <sstream>
#include <cstring>

#include "sensor.h"
#include "alarm.h"

// ── 출력 색상 코드 (ANSI) ────────────────────────────────────
#define COL_RESET   "\033[0m"
#define COL_GREEN   "\033[32m"
#define COL_YELLOW  "\033[33m"
#define COL_RED     "\033[31m"
#define COL_CYAN    "\033[36m"
#define COL_BOLD    "\033[1m"

// ── 상태 → 색상 ──────────────────────────────────────────────
std::string stateColor(SensorState s) {
    switch (s) {
        case SensorState::NORMAL:   return COL_GREEN;
        case SensorState::WARNING:  return COL_YELLOW;
        case SensorState::CRITICAL: return COL_RED;
        case SensorState::FAULT:    return COL_RED;
        default: return COL_RESET;
    }
}

// ── 센서 리딩 1줄 출력 ───────────────────────────────────────
void printReading(const SensorReading& r, const std::string& sensorName) {
    char timeBuf[20];
    struct tm* tm_info = localtime(&r.timestamp);
    strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", tm_info);

    // 이름 패딩
    std::string namePad = sensorName;
    while ((int)namePad.size() < 24) namePad += ' ';

    std::cout << COL_CYAN << timeBuf << COL_RESET << "  "
              << namePad << "  ";

    if (r.state == SensorState::FAULT) {
        std::cout << COL_RED << "  FAULT  " << COL_RESET << "\n";
        return;
    }

    // 값 출력
    std::cout << std::fixed << std::setprecision(1)
              << std::setw(7) << r.value << " " << r.unit << "  "
              << stateColor(r.state)
              << "[" << stateToString(r.state) << "]"
              << COL_RESET << "\n";
}

// ── 알람 콘솔 핸들러 ─────────────────────────────────────────
void consoleAlarmHandler(const AlarmEvent& event) {
    char timeBuf[20];
    struct tm* tm_info = localtime(&event.timestamp);
    strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", tm_info);

    std::string col = (event.level == AlarmLevel::CRITICAL || event.level == AlarmLevel::FAULT)
                      ? COL_RED : COL_YELLOW;

    std::cout << "\n" << col << COL_BOLD
              << "  *** ALARM " << alarmLevelToString(event.level)
              << " [" << timeBuf << "] "
              << event.message << " ***"
              << COL_RESET << "\n\n";
}

// ── TrainSimulator 클래스 ────────────────────────────────────
class TrainSimulator {
public:
    TrainSimulator() {
        // 센서 등록
        speedSensor_   = std::make_shared<SpeedSensor>    ("SPD-001", 150.0);
        motorTemp_     = std::make_shared<TemperatureSensor>("TMP-001", "모터",   80.0, 110.0);
        bearingTemp_   = std::make_shared<TemperatureSensor>("TMP-002", "베어링", 70.0, 95.0);
        brakePressure_ = std::make_shared<BrakePressureSensor>("BRK-001", 8.0);
        vibration_     = std::make_shared<VibrationSensor>("VIB-001", 10.0, 20.0);

        sensors_ = {speedSensor_, motorTemp_, bearingTemp_, brakePressure_, vibration_};
        names_   = {"속도 센서",   "모터 온도 센서",
                    "베어링 온도 센서", "제동 압력 센서", "진동 센서"};

        // 알람 핸들러 등록
        alarm_.registerHandler(consoleAlarmHandler);
    }

    // 시뮬레이션 메인 루프
    void run(int totalTicks) {
        printHeader();
        std::srand(42); // 재현 가능한 난수 시드

        for (int tick = 0; tick < totalTicks; tick++) {
            applyScenario(tick);
            printTickHeader(tick);

            for (int i = 0; i < (int)sensors_.size(); i++) {
                SensorReading r = sensors_[i]->readSensor();
                printReading(r, names_[i]);
                alarm_.evaluate(r, names_[i]);
            }

            // 100ms 간격 (시뮬레이션)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        alarm_.printSummary();
        std::cout << COL_BOLD << "\n  시뮬레이션 완료.\n" << COL_RESET;
    }

private:
    // 센서 포인터
    std::shared_ptr<SpeedSensor>         speedSensor_;
    std::shared_ptr<TemperatureSensor>   motorTemp_;
    std::shared_ptr<TemperatureSensor>   bearingTemp_;
    std::shared_ptr<BrakePressureSensor> brakePressure_;
    std::shared_ptr<VibrationSensor>     vibration_;

    std::vector<std::shared_ptr<Sensor>> sensors_;
    std::vector<std::string>             names_;
    AlarmManager                         alarm_;

    // ── 시나리오 설정 ────────────────────────────────────────
    //  Tick  0~ 9  : 역 출발 (속도 0 → 80 km/h)
    //  Tick 10~19  : 정상 주행 80 km/h
    //  Tick 20~24  : 모터 과열 시작
    //  Tick 25~29  : 고속 구간 (120 km/h)
    //  Tick 30~34  : 제동 구간 (감속 + 제동 압력)
    //  Tick 35~39  : 진동 이상
    //  Tick 40~44  : 속도 센서 고장
    //  Tick 45~49  : 복구 및 정상화
    void applyScenario(int tick) {
        // 속도
        if      (tick <  10) speedSensor_->setTargetSpeed(tick * 9.0);      // 가속
        else if (tick <  25) speedSensor_->setTargetSpeed(80.0);             // 정속
        else if (tick <  30) speedSensor_->setTargetSpeed(120.0);            // 고속
        else if (tick <  35) speedSensor_->setTargetSpeed(30.0);             // 감속
        else if (tick <  45) speedSensor_->setTargetSpeed(80.0);             // 재가속
        else                 speedSensor_->setTargetSpeed(60.0);             // 정상화

        // 제동
        brakePressure_->applyBrake(tick >= 30 && tick < 35);

        // 모터 과열
        motorTemp_->simulateOverheat(tick >= 20 && tick < 35);

        // 진동 이상
        vibration_->simulateAnomalousVibration(tick >= 35 && tick < 42);

        // 센서 고장 주입
        speedSensor_->injectFault(tick >= 40 && tick < 45);
    }

    void printHeader() {
        std::cout << COL_BOLD << COL_CYAN
                  << "\n====================================================\n"
                  << "  열차 센서 데이터 시뮬레이터 v1.0\n"
                  << "  Train Sensor Data Simulator — Portfolio Project #2\n"
                  << "====================================================\n"
                  << COL_RESET;
        std::cout << "  센서 수: " << sensors_.size() << "개\n";
        std::cout << "  샘플 주기: 100ms\n\n";
    }

    void printTickHeader(int tick) {
        std::cout << COL_BOLD
                  << "── Tick " << std::setw(3) << tick
                  << " ─────────────────────────────────────\n"
                  << COL_RESET;
    }
};

// ── main ─────────────────────────────────────────────────────
int main() {
    TrainSimulator sim;
    sim.run(50);   // 50 tick 시뮬레이션
    return 0;
}
