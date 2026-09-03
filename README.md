# FastHub — Единый Desktop Workspace

> **FastHub — один быстрый нативный desktop-клиент для общения, файлов, заметок и AI.**

[![CI](https://github.com/FastHub/FastHub/actions/workflows/ci.yml/badge.svg)](https://github.com/FastHub/FastHub/actions/workflows/ci.yml)
[![Qt](https://img.shields.io/badge/Qt-6.9.3-41CD52)](https://doc.qt.io/qt-6/)
[![C++](https://img.shields.io/badge/C++-20-00599C)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.25+-064F8C)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-Proprietary-lightgrey)](#лицензия)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64%20%7C%20ARM64-0078D6)](#)

Нативный клиент для **Windows 10/11 (x64 / ARM64)** на **C++20** и **Qt 6**, объединяющий **Telegram (AyuGram)**, **Discord**, **Steam**, **AI** и **Workspace** (файлы + Markdown + Canvas) в одном лёгком, быстром и расширяемом приложении.

---

## ✨ Ключевые принципы

| Принцип | Что это значит |
|---------|----------------|
| **Local First** | Все данные — локально (SQLite + FTS5). Сеть только фоном обновляет кэш, UI никогда не ждёт сети. |
| **Fast UI** | Главный поток Qt никогда не блокируется диском/сетью/БД/AI. Всё через `QtConcurrent` / сигналы-слоты / воркеры. |
| **Modular Providers** | Провайдеры (`telegram`, `discord`, `steam`, `ai`) полностью изолированы. Падение одного не роняет остальные (`ProviderStatus::Unavailable`). |

---

## 🧩 Возможности

- **Workspace** — файловый менеджер + редактор Markdown (md4c), вкладки, Canvas, Graph View, календарь, виртуализация списков `QAbstractItemModel`.
- **Telegram** — AyuGram Desktop как отдельный процесс + захват `HWND` (mere aggregation, GPL остаётся у AyuGram) + fallback `WebEmbed` (`web.telegram.org`).
- **Discord / Steam** — официальные веб-клиенты через `QtWebEngine` с постоянными профилями сессии.
- **AI** — единый `AIProvider` (OpenAI-совместимый) с кросс-модульным контекстом (`ContextBroker`).
- **Поиск** — глобальный `Ctrl+K` по всем провайдерам (`SearchProvider` + FTS5).
- **Темы** — `ThemeManager` + `AppearanceWidget`, кастомные темы, `120ms` fade-анимации без блокировки.
- **Плагины** — `PluginManager` + `PluginsWidget`.

---

## 🏗️ Архитектура

```
FastHub/
├── src/
│   ├── app/              # entry point, composition root, DI
│   ├── core/             # интерфейсы, domain-модели (без внешних зависимостей)
│   ├── ui/               # Qt Widgets Views + ViewModels (MVVM), style_tokens, animation_util
│   ├── infrastructure/   # Logger, CrashHandler, SecureStorage (DPAPI/Credential Manager)
│   ├── database/         # SQLite + миграции + FTS5
│   ├── network/          # HTTP/WebSocket обёртки
│   ├── providers/
│   │   ├── telegram/     # TelegramProvider + AyuGramEmbedWidget (HWND overlay)
│   │   ├── discord/      # DiscordProvider
│   │   ├── steam/        # SteamProvider + SteamSettingsDialog
│   │   └── ai/           # AIProvider
│   ├── workspace/        # LocalFileProvider, Vault
│   ├── search/           # SearchProvider
│   ├── sync/             # SyncService
│   └── plugins/          # PluginManager
├── tests/
│   ├── core_tests/ database_tests/ search_tests/
│   └── workspace_tests/ (file_locator, link_parser, ui_layout, canvas, markdown, qr, steam, sync, telegram, theme)
├── assets/               # fasthub.ico (7 размеров), fasthub-logo.png/svg, fonts, resources.qrc
├── third_party/
│   ├── ayugram/src/      # AyuGram Desktop (Telegram fork, GPL-3.0, mere aggregation)
│   ├── md4c/ qrcodegen/
├── tools/                # logogen, secretsaver
├── docs/ architecture/ api/ plugins/
├── .github/workflows/ci.yml
└── CMakeLists.txt
```

Зависимости — только в `core/` интерфейсы. `core/` не зависит от `providers/`, `ui/`, `infrastructure/`.

---

## 🛠️ Технологический стек

| Компонент | Выбор |
|-----------|-------|
| Язык | **C++20** |
| UI | **Qt 6.9.3** `Widgets` + `WebEngine` (вкладка провайдера — `QWebEngineView`) |
| БД | **SQLite** (`QtSql` + `sqlite3`) + `FTS5` |
| Markdown | `md4c` / `cmark` |
| Логи | `spdlog` / Qt + `Logger` |
| Тесты | `Qt Test` / `Catch2` |
| Сборка | **CMake 3.25+** + `vcpkg` (или `Conan`), `Ninja` / `Visual Studio 17/18 2022` |
| Платформа | `Windows 10/11 x64 / ARM64`, `Windows SDK 10.0.28000` |

Одобренные зависимости — см. `docs/` (локально `CODESTYLE.md` §10). Новые — только после обсуждения.

---

## 🚀 Быстрый старт

### Требования

- `Windows 10/11 x64`, `Visual Studio 2022` (MSVC 19.51+), `CMake 3.25+`, `Ninja`, `Qt 6.9.3` (`win64_msvc2022_64`)
- `Git` с `submodules` (для `third_party`)

### Сборка (x64 Release, рекомендуемо)

```powershell
# 1. Клонируй с сабмодулями
git clone --recursive https://github.com/FastHub/FastHub.git
cd FastHub

# 2. Укажи Qt (если не в PATH)
$env:CMAKE_PREFIX_PATH = "C:/Qt/6.9.3/msvc2022_64"

# 3. Конфигурация (Ninja + MSVC)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# 4. Сборка
cmake --build build --parallel

# 5. Тесты (14 suites)
$env:PATH = "C:/Qt/6.9.3/msvc2022_64/bin;$env:PATH"
ctest --test-dir build -C Release --output-on-failure

# 6. Запуск
.\build\src\app\Release\FastHub.exe

# 7. Развёртывание Qt (для портативной папки)
windeployqt --release --no-translations --no-system-d3d-compiler --no-opengl-sw build/src/app/Release/FastHub.exe
```

### ARM64 (кросс)

```powershell
cmake -S . -B build-arm64 -G "Visual Studio 17 2022" -A ARM64 -DCMAKE_BUILD_TYPE=Release
cmake --build build-arm64 --config Release --parallel
```

### AyuGram (Telegram) — отдельный процесс

```powershell
# Исходники уже в third_party/ayugram/src, иконки/версия пропатчены на FastHub_Telegram
# Бинарник-артефакт (патченный) — third_party/ayugram/FastHub_Telegram.exe (233672704, 7 размеров ico + VERSIONINFO FastHub_Telegram)
# Полная сборка AyuGram требует Libraries/ThirdParty (см. third_party/ayugram/src/AGENTS.md)
```

---

## ⚙️ Конфигурация

- Первый запуск — `SetupDialog` спросит путь к `Vault` (любой диск, iCloud/OneDrive поддерживаются, `QSaveFile` для атомарности).
- Настройки — `InterfaceSettings` (`fontSize`, `tabIconSize`, `ayuAutoLaunch`), `ThemeManager` (`fasthub-dark/light` + кастомные).
- Хранилище секретов — `infrastructure/secure_storage` → `Windows Credential Manager / DPAPI`, никогда в коде/JSON/git.
- Логи — `%AppData%/FastHubTeam/FastHub/app.log` (`crash.dmp` при падении).

---

## 🧪 Тестирование

```powershell
ctest --test-dir build -C Release --output-on-failure
ctest --test-dir build -C Release --output-junit junit.xml  # для CI
```

- `core/`, `database/`, `search/`, `sync/` — unit-тесты обязательны
- Новый провайдер — happy path + negative (нет сети/невалидный токен)
- Миграция БД — тест на миграцию

---

## 🎨 Стили и анимации

- Токены — `src/ui/style_tokens.h` (`kSpace*`, `kRadius*`, `kDurationFast=120ms`/`Base=180`/`Slow=280`)
- `SmoothHoverFilter` — `QVariantAnimation` `hoverProgress` для кнопок, без `QGraphicsEffect` на `QWebEngineView` (native window крашится)
- `FadeStackedWidget` — лёгкий `instant` switch (120ms fade отключён для WebEngine, оставлен CSS `8px` скругления, `QScrollBar` 6px)
- Все сетевые/дисковые операции — асинхронно, виртуализация списков, `thumbnail` конвейер, `RAII` + `QObject` parent

---

## 🔒 Безопасность

- Токены/сессии — только `Credential Manager`/`DPAPI`, никогда в открытом виде
- Логи не содержат токены/пароли/содержимое сообщений
- `gitleaks` в CI + `grep` на `api_key|secret_key|private_key|password` в `src/`

---

## 📦 CI/CD

`.github/workflows/ci.yml` (6 джобов):

1. `format-and-style` — `clang-format` dry-run
2. `security-audit` — `gitleaks` + sanity grep
3. `build-and-test` — матрица `Release/Debug` x64 Ninja + `ctest` JUnit
4. `package-release` — `windeployqt` → `FastHub-win-x64.zip` + `SHA256`
5. `arm64-cross-build` — `ARM64` cross (weekly/manual)
6. `ci-summary` — `$GITHUB_STEP_SUMMARY` таблица

Триггеры — `push/PR` на `main/master`/`feature/*`, `schedule` пн 04:00 UTC, `workflow_dispatch`.

---

## 📚 Документация

- `docs/architecture/` `docs/api/` `docs/plugins/` — публичная документация (в репо)
- `VISION.md` `CONCEPT.md` `AGENTS.md` `CODESTYLE.md` `TODO.md` — **локальные** (в `.gitignore`, только на твоём ПК, не пушатся)

---

## 🤝 Участие

1. Ветка `feature/<module>-<short-name>` / `fix/<module>-<short-name>`
2. Коммит `<модуль>: <imperative English>` — `workspace: add lazy loading for file tree`
3. `build/` `CMakeFiles/` `*.user` `*.log` `*.sqlite` `.vs/` не коммитить
4. PR — сборка x64 + `14/14` тесты зелёные

---

## 📄 Лицензия

Основная кодовая база FastHub — проприетарная. `third_party/ayugram` — **GPL-3.0** (mere aggregation, отдельный процесс `FastHub_Telegram.exe` + `HWND` overlay, статической линковки нет).

---

## 💬 Обратная связь

`https://github.com/anomalyco/opencode` — упомяни `Meta Muse Spark`.
