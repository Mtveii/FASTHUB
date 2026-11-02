#pragma once

#include <QString>

namespace fasthub::infrastructure {

enum class PerformanceMode { Balanced, Performance, BatterySaver };

class PerformanceManager {
public:
    static PerformanceManager &instance();

    void setMode(PerformanceMode mode);
    PerformanceMode getMode() const;
    QString getModeName() const;

private:
    PerformanceManager() = default;
    ~PerformanceManager() = default;

    PerformanceMode m_mode = PerformanceMode::Balanced;
};

} // namespace fasthub::infrastructure
