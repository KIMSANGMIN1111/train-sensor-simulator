// ============================================================
// alarm.cpp — 알람 매니저 구현
// ============================================================

#include "alarm.h"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <algorithm>

AlarmManager::AlarmManager() {}

void AlarmManager::registerHandler(AlarmHandler handler) {
    handlers_.push_back(handler);
}

void AlarmManager::evaluate(const SensorReading& reading, const std::string& sensorName) {
    // NORMAL 상태는 알람 발생 안 함
    if (reading.state == SensorState::NORMAL) {
        lastAlarmKey_ = "";
        return;
    }

    // 중복 억제: 동일 센서의 동일 상태가 연속되면 스킵
    std::string key = reading.sensorId + "_" + stateToString(reading.state);
    if (key == lastAlarmKey_) return;
    lastAlarmKey_ = key;

    // 알람 이벤트 생성
    AlarmEvent event;
    event.level     = stateToAlarmLevel(reading.state);
    event.sensorId  = reading.sensorId;
    event.value     = reading.value;
    event.unit      = reading.unit;
    event.timestamp = reading.timestamp;

    // 메시지 생성
    if (reading.state == SensorState::FAULT) {
        event.message = sensorName + " 센서 고장 감지";
    } else {
        event.message = sensorName + " 이상값 감지: "
                      + std::to_string((int)reading.value) + " " + reading.unit;
    }

    fireAlarm(event);
}

void AlarmManager::fireAlarm(const AlarmEvent& event) {
    history_.push_back(event);
    // 등록된 모든 핸들러 호출
    for (auto& handler : handlers_) {
        handler(event);
    }
}

AlarmLevel AlarmManager::stateToAlarmLevel(SensorState state) const {
    switch (state) {
        case SensorState::WARNING:  return AlarmLevel::WARNING;
        case SensorState::CRITICAL: return AlarmLevel::CRITICAL;
        case SensorState::FAULT:    return AlarmLevel::FAULT;
        default:                    return AlarmLevel::INFO;
    }
}

void AlarmManager::printSummary() const {
    std::cout << "\n========================================\n";
    std::cout << "  알람 발생 이력 요약\n";
    std::cout << "========================================\n";
    std::cout << "  총 알람 수 : " << history_.size() << "건\n";
    std::cout << "  WARNING    : " << countByLevel(AlarmLevel::WARNING)  << "건\n";
    std::cout << "  CRITICAL   : " << countByLevel(AlarmLevel::CRITICAL) << "건\n";
    std::cout << "  FAULT      : " << countByLevel(AlarmLevel::FAULT)    << "건\n";
    std::cout << "----------------------------------------\n";
    std::cout << "  최근 알람 5건:\n";
    int start = std::max(0, (int)history_.size() - 5);
    for (int i = start; i < (int)history_.size(); i++) {
        const auto& e = history_[i];
        char timeBuf[20];
        struct tm* tm_info = localtime(&e.timestamp);
        strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", tm_info);
        std::cout << "  " << timeBuf << " "
                  << alarmLevelToString(e.level) << " "
                  << e.message << "\n";
    }
    std::cout << "========================================\n";
}

int AlarmManager::countByLevel(AlarmLevel level) const {
    return (int)std::count_if(history_.begin(), history_.end(),
        [level](const AlarmEvent& e) { return e.level == level; });
}
