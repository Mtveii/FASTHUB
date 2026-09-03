#include "ayu_window_tracker.h"

#include <QHash>
#include <QString>

#include <tlhelp32.h>

namespace fasthub::ui::ayu {

namespace {

constexpr int kMaxTitle = 64;

struct EnumContext {
    WindowTracker::Query* query = nullptr;
    // Per-call cache: pid -> image name matches the query. Avoids running
    // OpenProcess + QueryFullProcessImageNameW for the same pid on every
    // poll tick (300 ms).
    QHash<DWORD, bool> imageNameMatch;
};

/// @brief Case-insensitive image-name check with per-call caching.
bool matchesImageName(DWORD pid, const QString& imageName,
                      QHash<DWORD, bool>& cache) {
    if (imageName.isEmpty()) {
        return true; // No filter requested
    }
    const auto it = cache.constFind(pid);
    if (it != cache.constEnd()) {
        return it.value();
    }

    bool match = false;
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (process) {
        wchar_t imagePath[MAX_PATH] = {};
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(process, 0, imagePath, &size)) {
            const wchar_t* fileName = imagePath;
            if (const wchar_t* slash = wcsrchr(imagePath, L'\\')) {
                fileName = slash + 1;
            }
            match = imageName.compare(
                        QString::fromWCharArray(fileName),
                        Qt::CaseInsensitive) == 0;
        }
        CloseHandle(process);
    }
    cache.insert(pid, match);
    return match;
}

BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam) {
    auto* ctx = reinterpret_cast<EnumContext*>(lParam);
    WindowTracker::Query& query = *ctx->query;

    if (!query.allowHidden && !IsWindowVisible(hwnd)) {
        return TRUE;
    }
    // Owned popups are never the main window — but when we search by exact
    // pid (pollForWindow) the main window itself can be owned by its own
    // hidden splash (FastHub_Telegram pid 21796: visible 0x3506D0 owner 0x180912).
    // Skip owner check for pid-targeted queries.
    if (query.targetPid == 0 && GetWindow(hwnd, GW_OWNER) != nullptr) {
        return TRUE;
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0) {
        return TRUE;
    }

    // Cheap path first: an exact pid match needs no image-name probing.
    if (query.targetPid != 0) {
        if (pid != query.targetPid) {
            return TRUE;
        }
    } else if (!matchesImageName(pid, query.imageName, ctx->imageNameMatch)) {
        return TRUE;
    }

    query.found = hwnd;
    return FALSE; // Stop enumeration
}

} // namespace

HWND WindowTracker::findWindow(const Query& queryIn) {
    Query query = queryIn; // Mutable copy: carries the result back.
    EnumContext ctx;
    ctx.query = &query;
    EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&ctx));
    return query.found;
}

bool WindowTracker::isWindowTitleReady(HWND hwnd) {
    wchar_t title[kMaxTitle] = {};
    const int len = GetWindowTextW(hwnd, title, kMaxTitle);
    return len > 0 && titleMatchesReadyState(title, static_cast<size_t>(len));
}

bool WindowTracker::titleMatchesReadyState(const wchar_t* title, size_t len) {
    Q_UNUSED(len);
    // The initializing window still carries the placeholder title:
    // AyuGram used 'AyuGramDesktop', FastHub_Telegram should be
    // 'FastHub_TelegramDesktop' but the actual window shows 'FastHubDesktop'
    // (see tasklist pid 21796). Keep all variants.
    const wchar_t* kInit1 = L"FastHub_TelegramDesktop";
    const wchar_t* kInit2 = L"FastHubDesktop";
    const wchar_t* kInitOld = L"AyuGramDesktop";
    return wcsncmp(title, kInit1, wcslen(kInit1)) != 0
        && wcsncmp(title, kInit2, wcslen(kInit2)) != 0
        && wcsncmp(title, kInitOld, wcslen(kInitOld)) != 0;
}

bool WindowTracker::isProcessRunning(const QString& imageName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    bool found = false;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (imageName.compare(QString::fromWCharArray(entry.szExeFile),
                                  Qt::CaseInsensitive) == 0) {
                found = true;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return found;
}

} // namespace fasthub::ui::ayu
