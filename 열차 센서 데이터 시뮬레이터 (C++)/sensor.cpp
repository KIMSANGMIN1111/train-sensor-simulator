// ============================================================
// sensor.cpp — 센서 클래스 구현
// ============================================================

#include "sensor.h"
#include <cmath>
#include <cstdlib>
#include <stdexcept>

// ── 공통 유틸 ────────────────────────────────────────────────

// Box-Muller 변환으로 정규분포 노이즈 생성
double Sensor::addNoise(double base, double stddev) const {
    double u1 = (std::rand() + 1.0) / (RAND_MAX + 1.0);
    double u2 = (std::rand() + 1.0) / (RAND_MAX + 1.0);
    double normal = std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2);
    return base + normal * stddev;
}

// ── Sensor (기본 클래스) ─────────────────────────────────────

Sensor::Sensor(const std::string& id, const std::string& name, const std::string& unit)
    : id_(id), name_(name), unit_(unit) {}

SensorReading Sensor::readSensor() {
    SensorReading reading;
    reading.sensorId  = id_;
    reading.timestamp = std::time(nullptr);

    if (isFaulty_) {
        // 고장 상태: NaN 반환
        reading.value = std::nan("");
        reading.state = SensorState::FAULT;
        reading.unit  = unit_;
        return reading;
    }

    double val     = generateValue();
    lastValue_     = val;
    reading.value  = val;
    reading.state  = evaluateState(val);
    reading.unit   = unit_;
    return reading;
}

void Sensor::injectFault(bool fault) {
    isFaulty_ = fault;
}

// ── SpeedSensor ──────────────────────────────────────────────

SpeedSensor::SpeedSensor(const std::string& id, double maxSpeed)
    : Sensor(id, "속도 센서", "km/h"), maxSpeed_(maxSpeed) {}

double SpeedSensor::generateValue() {
    // 목표 속도로 서서히 수렴하는 모델
    double diff    = targetSpeed_ - lastValue_;
    double newBase = lastValue_ + diff * 0.15;       // 부드러운 가속/감속
    double noisy   = addNoise(newBase, 0.8);         // ±0.8 km/h 노이즈
    return std::max(0.0, std::min(noisy, maxSpeed_ * 1.05)); // 최대 속도 초과 방지
}

SensorState SpeedSensor::evaluateState(double value) const {
    if (value > maxSpeed_ * 1.02) return SensorState::CRITICAL; // 2% 초과 → 위험
    if (value > maxSpeed_ * 0.95) return SensorState::WARNING;  // 5% 이내 → 경고
    return SensorState::NORMAL;
}

// ── TemperatureSensor ────────────────────────────────────────

TemperatureSensor::TemperatureSensor(const std::string& id, const std::string& location,
                                     double warnThreshold, double critThreshold)
    : Sensor(id, "온도 센서 [" + location + "]", "°C"),
      location_(location), warnThreshold_(warnThreshold), critThreshold_(critThreshold) {}

double TemperatureSensor::generateValue() {
    double base = overheating_
        ? lastValue_ + addNoise(3.5, 0.5)    // 과열 시: 급격한 온도 상승
        : addNoise(lastValue_, 0.3);          // 정상 시: 미세 변동

    // 자연 냉각 (과열이 없으면 25°C 평형 온도로 서서히 복귀)
    if (!overheating_) base += (25.0 - base) * 0.02;
    return std::max(15.0, base);
}

SensorState TemperatureSensor::evaluateState(double value) const {
    if (value >= critThreshold_) return SensorState::CRITICAL;
    if (value >= warnThreshold_) return SensorState::WARNING;
    return SensorState::NORMAL;
}

// ── BrakePressureSensor ──────────────────────────────────────

BrakePressureSensor::BrakePressureSensor(const std::string& id, double nominalPressure)
    : Sensor(id, "제동 압력 센서", "bar"), nominalPressure_(nominalPressure) {}

double BrakePressureSensor::generateValue() {
    double target = braking_
        ? nominalPressure_                  // 제동 중: 정격 압력
        : nominalPressure_ * 0.1;          // 비제동: 잔압만 유지
    double base = lastValue_ + (target - lastValue_) * 0.3;
    return std::max(0.0, addNoise(base, 0.1));
}

SensorState BrakePressureSensor::evaluateState(double value) const {
    if (value < nominalPressure_ * 0.5)  return SensorState::CRITICAL; // 50% 미만 → 위험
    if (value < nominalPressure_ * 0.75) return SensorState::WARNING;  // 75% 미만 → 경고
    return SensorState::NORMAL;
}

// ── VibrationSensor ──────────────────────────────────────────

VibrationSensor::VibrationSensor(const std::string& id,
                                 double warnThreshold, double critThreshold)
    : Sensor(id, "진동 센서", "mm/s"), warnThreshold_(warnThreshold), critThreshold_(critThreshold) {}

double VibrationSensor::generateValue() {
    if (anomaly_) {
        // 이상 진동: 정현파 + 랜덤 스파이크
        static int tick = 0;
        double sine = warnThreshold_ + 5.0 * std::sin(tick++ * 0.3);
        return std::abs(addNoise(sine, 1.5));
    }
    // 정상 진동: 속도에 비례하는 미세 진동 (베이스라인 2 mm/s)
    return std::abs(addNoise(2.0, 0.4));
}

SensorState VibrationSensor::evaluateState(double value) const {
    if (value >= critThreshold_) return SensorState::CRITICAL;
    if (value >= warnThreshold_) return SensorState::WARNING;
    return SensorState::NORMAL;
}
