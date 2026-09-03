#pragma once

#include <QString>

class QProcess;

namespace fasthub::ui::ayu {

/// @brief Locates, spawns and cleans up the AyuGram process.
///
/// Owns all process-lifecycle decisions; the widget only coordinates
/// signals. Kept separate from window tracking so a future embedded-chat
/// module (Discord / Steam native clients) can reuse it with another exe.
class ProcessLauncher {
public:
    /// @brief Upper bound for zombie-kill retries (stops infinite loops).
    static constexpr int kMaxZombieKills = 3;

    explicit ProcessLauncher(QString imageName);

    /// @brief Locates the executable relative to the FastHub binary.
    /// No machine-specific absolute paths.
    /// @return Empty string when not found (caller shows an error overlay).
    QString locateExecutable() const;

    /// @brief True when any process with our image name is running.
    bool isProcessRunning() const;

    /// @brief True when a process is running but owns NO windows at all
    /// (orphan from a force-killed session that blocks single-instance
    /// relaunch). Healthy-but-hidden instances are NOT orphans.
    bool hasWindowlessOrphan() const;

    /// @brief Force-terminates every instance of the image name (async,
    /// detached taskkill - never blocks the UI thread).
    void terminateAllInstances();

    int zombieKillAttempts() const { return m_zombieKills; }
    void resetZombieKills() { m_zombieKills = 0; }
    void noteZombieKill() { ++m_zombieKills; }

private:
    QString m_imageName;
    int m_zombieKills = 0;
};

} // namespace fasthub::ui::ayu
