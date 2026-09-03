#pragma once

#include <QString>

namespace fasthub::infrastructure {

/// @brief Installs a process-wide unhandled exception handler.
///
/// On an unhandled SEH exception the handler writes:
/// - crash.log: timestamp, exception code and faulting address
/// - crash.dmp: minidump (best effort, requires dbghelp.dll)
///
/// After logging, the default OS handling is resumed so the process still
/// terminates through the normal WER path.
class CrashHandler {
public:
    /// @brief Installs the handler. Safe to call multiple times.
    /// @param logDir Directory where crash.log / crash.dmp are written.
    static void install(const QString& logDir);
};

} // namespace fasthub::infrastructure
