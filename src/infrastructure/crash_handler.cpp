#include "crash_handler.h"

#include <windows.h>
#include <dbghelp.h>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>

namespace fasthub::infrastructure {

namespace {

QString g_logDir;

void writeCrashLog(const QString &text) {
    if (g_logDir.isEmpty()) {
        return;
    }
    QDir().mkpath(g_logDir);
    QFile file(g_logDir + "/crash.log");
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << " " << text
               << "\n";
    }
}

void writeMinidump(EXCEPTION_POINTERS *pointers) {
    // Best effort: a missing dbghelp.dll must not crash the handler itself.
    HMODULE dbghelp = LoadLibraryW(L"dbghelp.dll");
    if (!dbghelp) {
        return;
    }

    using MiniDumpWriteDumpFn =
        BOOL(WINAPI *)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION,
                       PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);
    auto dumpFn =
        reinterpret_cast<MiniDumpWriteDumpFn>(GetProcAddress(dbghelp, "MiniDumpWriteDump"));
    if (!dumpFn) {
        FreeLibrary(dbghelp);
        return;
    }

    HANDLE dumpFile = CreateFileW((g_logDir + "/crash.dmp").toStdWString().c_str(), GENERIC_WRITE,
                                  0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (dumpFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION info;
        info.ThreadId = GetCurrentThreadId();
        info.ExceptionPointers = pointers;
        info.ClientPointers = FALSE;
        dumpFn(GetCurrentProcess(), GetCurrentProcessId(), dumpFile, MiniDumpNormal, &info, nullptr,
               nullptr);
        CloseHandle(dumpFile);
    }
    FreeLibrary(dbghelp);
}

LONG WINAPI fastHubUnhandledExceptionFilter(EXCEPTION_POINTERS *pointers) {
    DWORD code = 0;
    void *address = nullptr;
    if (pointers && pointers->ExceptionRecord) {
        code = pointers->ExceptionRecord->ExceptionCode;
        address = pointers->ExceptionRecord->ExceptionAddress;
    }
    writeCrashLog(QString("Unhandled exception. code=0x%1 address=%2 thread=%3")
                      .arg(code, 8, 16, QLatin1Char('0'))
                      .arg(reinterpret_cast<quintptr>(address), 0, 16)
                      .arg(GetCurrentThreadId()));
    writeMinidump(pointers);
    return EXCEPTION_CONTINUE_SEARCH; // Let WER / default handling proceed.
}

} // namespace

void CrashHandler::install(const QString &logDir) {
    g_logDir = logDir;
    SetUnhandledExceptionFilter(fastHubUnhandledExceptionFilter);
}

} // namespace fasthub::infrastructure
