#include "performance_manager.h"

namespace fasthub::infrastructure {

PerformanceManager &PerformanceManager::instance() {
    static PerformanceManager s_instance;
    return s_instance;
}

void PerformanceManager::setMode(PerformanceMode mode) { m_mode = mode; }

PerformanceMode PerformanceManager::getMode() const { return m_mode; }

QString PerformanceManager::getModeName() const {
    switch (m_mode) {
    case PerformanceMode::Balanced:
        return "Balanced";
    case PerformanceMode::Performance:
        return "Performance";
    case PerformanceMode::BatterySaver:
        return "Battery Saver";
    }
    return "Unknown";
}

} // namespace fasthub::infrastructure
