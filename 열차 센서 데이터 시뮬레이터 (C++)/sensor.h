#pragma once
// ============================================================
// sensor.h — 센서 기본 클래스 및 파생 클래스 선언
// Train Sensor Data Simulator — Portfolio Project #2
// ============================================================

#include <string>
#include <ctime>

// ── 센서 상태 열거형 ─────────────────────────────────────────
enum class SensorState {
    NORMAL,     // 정상
    WARNING,    // 경고
    CRITICAL,   // 위험
    FAULT       // 센서 고장
};

// 상태 → 문자열 변환 헬퍼
inline std::string stateToString(SensorState s) {
    switch (s) {
        case SensorState::NORMAL:   return "NORMAL";
        case SensorState::WARNING:  return "WARNING";
        case SensorState::CRITICAL: return "CRITICAL";
        case SensorState::FAULT:    return "FAULT";
        default:                    return "UNKNOWN";
    }
}

// ── 센서 데이터 구조체 ───────────────────────────────────────
struct SensorReading {
    std::string sensorId;    // 센서 고유 ID
    double      value;       // 측정값
    std::string unit;        // 단위 (km/h, °C, bar 등)
    SensorState state;       // 상태
    time_t      timestamp;   // 측정 시각
};

// ── 기본 센서 추상 클래스 ────────────────────────────────────
class Sensor {
public:
    Sensor(const std::string& id, const std::string& name, const std::string& unit);
    virtual ~Sensor() = default;

    // 순수 가상 함수: 파생 클래스에서 반드시 구현
    virtual double      generateValue()                  = 0;
    virtual SensorState evaluateState(double value) const = 0;
    virtual std::string getSensorType()            const = 0;

    // 공통 동작
    SensorReading readSensor();
    void          injectFault(bool fault);

    // getter
    std::string getId()         const { return id_; }
    std::string getName()       const { return name_; }
    std::string getUnit()       const { return unit_; }
    bool        isFaulty()      const { return isFaulty_; }
    double      getLastValue()  const { return lastValue_; }

protected:
    std::string id_;
    std::string name_;
    std::string unit_;
    double      lastValue_  = 0.0;
    bool        isFaulty_   = false;

    // 간단한 가우시안 노이즈 추가 (현실감)
    double addNoise(double base, double stddev) const;
};

// ── 속도 센서 ────────────────────────────────────────────────
class SpeedSensor : public Sensor {
public:
    SpeedSensor(const std::string& id, double maxSpeed);

    double      generateValue()                   override;
    SensorState evaluateState(double value) const  override;
    std::string getSensorType()             const  override { return "SpeedSensor"; }

    void setTargetSpeed(double speed) { targetSpeed_ = speed; }

private:
    double maxSpeed_;
    double targetSpeed_ = 0.0;
};

// ── 온도 센서 (모터/베어링) ──────────────────────────────────
class TemperatureSensor : public Sensor {
public:
    TemperatureSensor(const std::string& id, const std::string& location,
                      double warnThreshold, double critThreshold);

    double      generateValue()                   override;
    SensorState evaluateState(double value) const  override;
    std::string getSensorType()             const  override { return "TemperatureSensor"; }

    void simulateOverheat(bool overheat) { overheating_ = overheat; }

private:
    std::string location_;
    double      warnThreshold_;
    double      critThreshold_;
    bool        overheating_ = false;
};

// ── 제동 압력 센서 ───────────────────────────────────────────
class BrakePressureSensor : public Sensor {
public:
    BrakePressureSensor(const std::string& id, double nominalPressure);

    double      generateValue()                   override;
    SensorState evaluateState(double value) const  override;
    std::string getSensorType()             const  override { return "BrakePressureSensor"; }

    void applyBrake(bool braking) { braking_ = braking; }

private:
    double nominalPressure_;
    bool   braking_ = false;
};

// ── 진동 센서 (차축) ─────────────────────────────────────────
class VibrationSensor : public Sensor {
public:
    VibrationSensor(const std::string& id, double warnThreshold, double critThreshold);

    double      generateValue()                   override;
    SensorState evaluateState(double value) const  override;
    std::string getSensorType()             const  override { return "VibrationSensor"; }

    void simulateAnomalousVibration(bool anomaly) { anomaly_ = anomaly; }

private:
    double warnThreshold_;
    double critThreshold_;
    bool   anomaly_ = false;
};
