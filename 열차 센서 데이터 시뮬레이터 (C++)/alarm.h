#pragma once
// ============================================================
// alarm.h — 이벤트 기반 알람 시스템
// ============================================================

#include "sensor.h"
#include <string>
#include <vector>
#include <functional>

// ── 알람 레벨 ────────────────────────────────────────────────
enum class AlarmLevel {
    INFO,       // 정보
    WARNING,    // 경고
    CRITICAL,   // 위험
    FAULT       // 고장
};

inline std::string alarmLevelToString(AlarmLevel l) {
    switch (l) {
        case AlarmLevel::INFO:     return "[INFO    ]";
        case AlarmLevel::WARNING:  return "[WARNING ]";
        case AlarmLevel::CRITICAL: return "[CRITICAL]";
        case AlarmLevel::FAULT:    return "[FAULT   ]";
        default:                   return "[UNKNOWN ]";
    }
}

// ── 알람 이벤트 구조체 ───────────────────────────────────────
struct AlarmEvent {
    AlarmLevel  level;
    std::string sensorId;
    std::string message;
    double      value;
    std::string unit;
    time_t      timestamp;
};

// ── 알람 핸들러 타입 (콜백 함수) ────────────────────────────
using AlarmHandler = std::function<void(const AlarmEvent&)>;

// ── 알람 매니저 ──────────────────────────────────────────────
class AlarmManager {
public:
    AlarmManager();

    // 핸들러 등록 (여러 핸들러 등록 가능)
    void registerHandler(AlarmHandler handler);

    // 센서 리딩으로 알람 평가 및 발생
    void evaluate(const SensorReading& reading, const std::string& sensorName);

    // 알람 히스토리 조회
    const std::vector<AlarmEvent>& getHistory() const { return history_; }

    // 히스토리 요약 출력
    void printSummary() const;

    // 알람 카운트
    int countByLevel(AlarmLevel level) const;

private:
    std::vector<AlarmHandler>  handlers_;
    std::vector<AlarmEvent>    history_;

    // 중복 알람 억제 (동일 센서 연속 동일 상태는 무시)
    std::string lastAlarmKey_;

    void fireAlarm(const AlarmEvent& event);
    AlarmLevel stateToAlarmLevel(SensorState state) const;
};
