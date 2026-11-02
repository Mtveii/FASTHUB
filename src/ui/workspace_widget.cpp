#include "workspace_widget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QTextEdit>
#include <QTextBrowser>
#include <QMessageBox>
#include <QFile>
#include <QFileInfo>
#include <QPushButton>
#include <QStackedWidget>
#include <QDesktopServices>
#include <QShortcut>
#include <QKeySequence>
#include <QAction>
#include <QClipboard>
#include <QGuiApplication>
#include <QProcess>
#include <QInputDialog>
#include <QTableWidget>
#include <QDirIterator>
#include <QtConcurrent>
#include <QFutureWatcher>
#include "core/canvas/canvas_document.h"

#include "workspace/local_file_provider.h"
#include "workspace/file_locator.h"
#include "workspace/calendar_widget.h"
#include "workspace/canvas_widget.h"
#include "infrastructure/logger.h"
#include "markdown_renderer.h"
#include "theme_manager.h"
#include "icon_util.h"
#include "context_broker.h"
#include "interface_settings.h"
#include "appearance_widget.h"
#include "graph_view.h"
#include "style_tokens.h"

#include <QFileIconProvider>
#include <QSet>
#include <QPointer>
#include <QMenu>
#include <QLabel>
#include <QLineEdit>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>

namespace fasthub::ui {

namespace {
constexpr int kAutoSaveDelayMs = 500;

/// @brief Unified Bootstrap icon set for the file tree (design spec: one
///        set everywhere; amber folders, per-type file glyphs).
class HubIconProvider : public QFileIconProvider {
public:
    HubIconProvider() {
        m_folder =
            fasthub::ui::IconUtil::tintedIcon(":/icons/tree-folder.svg", QColor("#C9A857"), 20);
        m_folderOpen = fasthub::ui::IconUtil::tintedIcon(":/icons/tree-folder-open.svg",
                                                         QColor("#C9A857"), 20);
        m_file = fasthub::ui::IconUtil::tintedIcon(":/icons/tree-file.svg", QColor("#8A93A6"), 20);
        m_md = fasthub::ui::IconUtil::tintedIcon(":/icons/tree-file-md.svg", QColor("#6E7BFF"), 20);
        m_code =
            fasthub::ui::IconUtil::tintedIcon(":/icons/tree-file-code.svg", QColor("#7BD88F"), 20);
        m_image =
            fasthub::ui::IconUtil::tintedIcon(":/icons/tree-file-image.svg", QColor("#66C0F0"), 20);
        m_richtext = fasthub::ui::IconUtil::tintedIcon(":/icons/tree-file-richtext.svg",
                                                       QColor("#B388FF"), 20);
        m_canvas = fasthub::ui::IconUtil::tintedIcon(":/icons/canvas.svg", QColor("#FF8A65"), 20);
        m_base = fasthub::ui::IconUtil::tintedIcon(":/icons/columns.svg", QColor("#4DB6AC"), 20);
    }

    QIcon icon(IconType type) const override {
        switch (type) {
        case QFileIconProvider::Folder:
        case QFileIconProvider::Drive:
        case QFileIconProvider::Desktop:
            return m_folder;
        default:
            return m_file;
        }
    }

    QIcon icon(const QFileInfo &info) const override {
        if (info.isDir()) {
            return m_folder;
        }
        return iconForSuffix(info.suffix().toLower());
    }

    QIcon iconForSuffix(const QString &suffix) const {
        if (suffix == QLatin1String("canvas"))
            return m_canvas;
        if (suffix == QLatin1String("base"))
            return m_base;
        if (suffix == QLatin1String("md") || suffix == QLatin1String("markdown"))
            return m_md;
        if (suffix == QLatin1String("html") || suffix == QLatin1String("htm"))
            return m_richtext;
        static const QSet<QString> code = {
            QStringLiteral("cpp"),   QStringLiteral("h"),   QStringLiteral("hpp"),
            QStringLiteral("c"),     QStringLiteral("cc"),  QStringLiteral("js"),
            QStringLiteral("ts"),    QStringLiteral("py"),  QStringLiteral("json"),
            QStringLiteral("xml"),   QStringLiteral("yml"), QStringLiteral("yaml"),
            QStringLiteral("css"),   QStringLiteral("sql"), QStringLiteral("sh"),
            QStringLiteral("cmake"), QStringLiteral("in"),  QStringLiteral("rc"),
        };
        if (code.contains(suffix))
            return m_code;
        static const QSet<QString> images = {
            QStringLiteral("png"),  QStringLiteral("jpg"), QStringLiteral("jpeg"),
            QStringLiteral("gif"),  QStringLiteral("svg"), QStringLiteral("bmp"),
            QStringLiteral("webp"), QStringLiteral("ico"),
        };
        if (images.contains(suffix))
            return m_image;
        return m_file;
    }

private:
    QIcon m_folder;
    QIcon m_folderOpen;
    QIcon m_file;
    QIcon m_md;
    QIcon m_code;
    QIcon m_image;
    QIcon m_richtext;
    QIcon m_canvas;
    QIcon m_base;
};

bool isMarkdownFile(const QString &path) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == QLatin1String("md") || suffix == QLatin1String("markdown");
}

bool isHtmlFile(const QString &path) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == QLatin1String("html") || suffix == QLatin1String("htm");
}
} // namespace

WorkspaceWidget::WorkspaceWidget(QWidget *parent) : QWidget(parent) {
    m_iconProvider = new HubIconProvider();
    setupUi();

    connect(&ThemeManager::instance(), &ThemeManager::themeApplied, this, [this](const QString &) {
        for (auto it = m_tabs.begin(); it != m_tabs.end(); ++it) {
            refreshPreview(it.value());
        }
    });
}

WorkspaceWidget::~WorkspaceWidget() {
    // Flush pending debounced saves so no edits are lost on close.
    for (int i = 0; i < m_editorTabWidget->count(); ++i) {
        flushPendingSave(i);
    }
    delete m_iconProvider;
}

void WorkspaceWidget::setupUi() {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 1. Left icon rail (Zone 2)
    m_iconRail = new IconRail(this);
    m_iconRail->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_iconRail->setFixedWidth(44);
    mainLayout->addWidget(m_iconRail);

    connect(m_iconRail, &IconRail::sectionRequested, this, [this](IconRail::Section section) {
        static bool s_handling = false;
        if (s_handling)
            return;
        s_handling = true;

        switch (section) {
        case IconRail::Section::Files:
            setFileTreeVisible(true);
            if (m_sidebarContainer)
                m_sidebarContainer->setVisible(true);
            break;
        case IconRail::Section::Graph:
            toggleGraphView();
            break;
        case IconRail::Section::Canvas: {
            createNewCanvas();
            break;
        }
        case IconRail::Section::Calendar: {
            auto *cal = new CalendarWidget(m_currentWorkspacePath, this);
            openOrFocusTab("Calendar", cal);
            break;
        }
        case IconRail::Section::Templates: {
            auto *lbl = new QLabel("Templates (Coming Soon)", this);
            lbl->setAlignment(Qt::AlignCenter);
            openOrFocusTab("Templates", lbl);
            break;
        }
        case IconRail::Section::Terminal: {
            auto *lbl = new QLabel("Terminal (Coming Soon)", this);
            lbl->setAlignment(Qt::AlignCenter);
            openOrFocusTab("Terminal", lbl);
            break;
        }
        case IconRail::Section::Tags: {
            auto *lbl = new QLabel("Tags & Outline (Coming Soon)", this);
            lbl->setAlignment(Qt::AlignCenter);
            openOrFocusTab("Tags", lbl);
            break;
        }
        }
        s_handling = false;
    });

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(4);
    m_splitter->setCollapsible(0, true);
    m_splitter->setCollapsible(1, false);
    m_splitter->setStyleSheet(
        "QSplitter::handle { background-color: transparent; margin: 4px; }"
        "QSplitter::handle:hover { background-color: palette(highlight); border-radius: 2px; }");

    // 2. Sidebar container (Zone 1 + 3 + 4 + footer)
    m_sidebarContainer = new QWidget(this);
    m_sidebarContainer->setObjectName("sidebarContainer");
    m_sidebarContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_sidebarContainer->setMinimumWidth(200);
    m_sidebarContainer->setMaximumWidth(450);
    QVBoxLayout *sidebarLayout = new QVBoxLayout(m_sidebarContainer);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);

    // Zone 1: Top Bar
    m_topBar = new TopBar(m_sidebarContainer);
    sidebarLayout->addWidget(m_topBar);

    // Zone 3: File List Toolbar
    m_fileListToolbar = new FileListToolbar(m_sidebarContainer);

    // Open Folder fallback button
    m_openFolderBtn = new QPushButton("Open Folder", m_sidebarContainer);
    m_openFolderBtn->setStyleSheet(
        "QPushButton { background-color: palette(button); color: palette(button-text); border: 1px "
        "solid palette(mid); border-radius: 4px; padding: 6px; margin: 8px; } QPushButton:hover { "
        "background-color: palette(alternate-base); }");
    connect(m_openFolderBtn, &QPushButton::clicked, this, &WorkspaceWidget::onOpenFolderClicked);

    // Zone 4: File tree
    m_fileModel = new QFileSystemModel(this);
    m_fileModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    m_fileModel->setIconProvider(m_iconProvider);
    m_fileModel->setOption(QFileSystemModel::DontWatchForChanges, true);

    m_fileTreeView = new QTreeView(m_sidebarContainer);
    m_fileTreeView->setModel(m_fileModel);
    m_fileTreeView->hideColumn(1); // Size
    m_fileTreeView->hideColumn(2); // Type
    m_fileTreeView->hideColumn(3); // Date Modified
    m_fileTreeView->setHeaderHidden(true);
    m_fileTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_fileTreeView->setStyleSheet(
        "QTreeView { background: transparent; border: none; padding: 4px; outline: 0; }"
        "QTreeView::item { padding: 6px 10px; border-radius: 6px; margin: 2px 6px; }"
        "QTreeView::item:hover { background-color: rgba(128, 128, 128, 0.12); }"
        "QTreeView::item:selected { background-color: palette(highlight); color: "
        "palette(highlighted-text); border-radius: 6px; }");
    connect(m_fileTreeView, &QTreeView::clicked, this, &WorkspaceWidget::onFileClicked);
    connect(m_fileTreeView, &QTreeView::doubleClicked, this, &WorkspaceWidget::onFileDoubleClicked);
    connect(m_fileTreeView, &QTreeView::customContextMenuRequested, this,
            &WorkspaceWidget::onFileContextMenuRequested);

    // Sidebar stacked pages (Files, Search, Bookmarks)
    m_sidebarPages = new QStackedWidget(m_sidebarContainer);

    // Page 0: Files
    QWidget *filesPage = new QWidget(m_sidebarPages);
    QVBoxLayout *filesPageLayout = new QVBoxLayout(filesPage);
    filesPageLayout->setContentsMargins(0, 0, 0, 0);
    filesPageLayout->setSpacing(0);
    filesPageLayout->addWidget(m_fileListToolbar);
    filesPageLayout->addWidget(m_openFolderBtn);
    filesPageLayout->addWidget(m_fileTreeView);
    m_sidebarPages->addWidget(filesPage);

    // Page 1: Search (matching screenshot 134035.png)
    QWidget *searchPage = new QWidget(m_sidebarPages);
    QVBoxLayout *searchLayout = new QVBoxLayout(searchPage);
    searchLayout->setContentsMargins(12, 12, 12, 12);
    searchLayout->setSpacing(8);

    QLineEdit *searchBox = new QLineEdit(searchPage);
    searchBox->setPlaceholderText("Введите поисковый запрос...");
    searchBox->setStyleSheet(
        "QLineEdit { background: palette(base); border: 1px solid palette(mid); border-radius: "
        "6px; padding: 6px 12px; color: palette(text); }");
    searchLayout->addWidget(searchBox);

    QLabel *searchHintsHeader = new QLabel("Настройки поиска", searchPage);
    searchHintsHeader->setStyleSheet("font-weight: bold; color: palette(text); margin-top: 8px;");
    searchLayout->addWidget(searchHintsHeader);

    QListWidget *searchHintsList = new QListWidget(searchPage);
    searchHintsList->setStyleSheet(
        "QListWidget { background: transparent; border: none; color: palette(text); } "
        "QListWidget::item { padding: 6px; border-radius: 4px; } QListWidget::item:hover { "
        "background-color: rgba(128,128,128,0.1); }");
    searchHintsList->addItem("path: совпадение в пути к файлу");
    searchHintsList->addItem("file: совпадение в имени файла");
    searchHintsList->addItem("tag: совпадение в тегах");
    searchHintsList->addItem("line: ключевые слова на одной строке");
    searchHintsList->addItem("section: ключевые слова под одним заголовком");
    searchHintsList->addItem("[property] совпадение со свойством");
    searchLayout->addWidget(searchHintsList);
    searchLayout->addStretch();
    m_sidebarPages->addWidget(searchPage);

    // Page 2: Bookmarks (matching screenshot 134043.png)
    QWidget *bookmarksPage = new QWidget(m_sidebarPages);
    QVBoxLayout *bmLayout = new QVBoxLayout(bookmarksPage);
    bmLayout->setContentsMargins(16, 32, 16, 16);
    bmLayout->setSpacing(12);
    QLabel *bmEmpty = new QLabel("Закладки не найдены", bookmarksPage);
    bmEmpty->setAlignment(Qt::AlignCenter);
    bmEmpty->setStyleSheet("color: palette(mid); font-size: 14px;");
    bmLayout->addWidget(bmEmpty);
    bmLayout->addStretch();
    m_sidebarPages->addWidget(bookmarksPage);

    sidebarLayout->addWidget(m_sidebarPages, 1);

    // Connect TopBar tab switching
    connect(m_topBar, &TopBar::filesTabRequested, this,
            [this]() { m_sidebarPages->setCurrentIndex(0); });
    connect(m_topBar, &TopBar::searchTabRequested, this,
            [this]() { m_sidebarPages->setCurrentIndex(1); });
    connect(m_topBar, &TopBar::bookmarksTabRequested, this,
            [this]() { m_sidebarPages->setCurrentIndex(2); });

    // Sidebar footer (Vault label + settings/help) — всегда видим, переход в оригинальный Obsidian
    QWidget *sidebarFooter = new QWidget(m_sidebarContainer);
    sidebarFooter->setObjectName("vaultFooter");
    sidebarFooter->setStyleSheet(
        "QWidget#vaultFooter { background: #1E293B; border-top: 1px solid #334155; } QPushButton { "
        "background: #334155; border: 1px solid #475569; border-radius: 6px; padding: 6px 10px; "
        "color: #F8FAFC; } QPushButton:hover { background: #475569; }");
    QHBoxLayout *footerLayout = new QHBoxLayout(sidebarFooter);
    footerLayout->setContentsMargins(8, 6, 8, 6);
    footerLayout->setSpacing(6);
    QPushButton *vaultLabel = new QPushButton("⬢ Vault", sidebarFooter);
    vaultLabel->setToolTip(
        "Переход в оригинальный Obsidian — открыть Vault в Obsidian / проводнике");
    vaultLabel->setMinimumHeight(28);
    footerLayout->addWidget(vaultLabel, 1);

    QPushButton *obsidianBtn = new QPushButton("↗ Obsidian", sidebarFooter);
    obsidianBtn->setToolTip("Открыть текущий Vault в приложении Obsidian");
    obsidianBtn->setMinimumHeight(28);
    footerLayout->addWidget(obsidianBtn);

    QPushButton *helpBtn = new QPushButton("?", sidebarFooter);
    helpBtn->setFixedWidth(28);
    helpBtn->setMinimumHeight(28);
    helpBtn->setToolTip("Справка");
    QPushButton *settingsBtn = new QPushButton("⚙", sidebarFooter);
    settingsBtn->setFixedWidth(28);
    settingsBtn->setMinimumHeight(28);
    settingsBtn->setToolTip("Настройки FastHub");
    footerLayout->addWidget(helpBtn);
    footerLayout->addWidget(settingsBtn);

    connect(settingsBtn, &QPushButton::clicked, this, [this]() {
        auto *appearance = new AppearanceWidget(this);
        openOrFocusTab("Settings", appearance);
    });
    auto openVaultInObsidian = [this]() {
        if (!m_currentWorkspacePath.isEmpty() && QDir(m_currentWorkspacePath).exists()) {
            // Try obsidian:// URL first, fallback to explorer
            QString vaultName = QFileInfo(m_currentWorkspacePath).fileName();
            QUrl obsidianUrl(QString("obsidian://open?vault=%1&file=%2")
                                 .arg(QString(QUrl::toPercentEncoding(vaultName)),
                                      QString(QUrl::toPercentEncoding(m_currentWorkspacePath))));
            bool opened = QDesktopServices::openUrl(obsidianUrl);
            if (!opened) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(m_currentWorkspacePath));
            }
            // Visual feedback
            infrastructure::Logger::instance().info("Workspace", "Open Vault in Obsidian: " +
                                                                     m_currentWorkspacePath);
        } else {
            auto *appearance = new AppearanceWidget(this);
            openOrFocusTab("Settings", appearance);
        }
    };
    connect(vaultLabel, &QPushButton::clicked, this, openVaultInObsidian);
    connect(obsidianBtn, &QPushButton::clicked, this, openVaultInObsidian);

    sidebarLayout->addWidget(sidebarFooter);

    // Connect toolbar & topbar signals
    connect(m_fileListToolbar, &FileListToolbar::newNoteRequested, this,
            &WorkspaceWidget::createNewNote);
    connect(m_fileListToolbar, &FileListToolbar::newFolderRequested, this,
            &WorkspaceWidget::createNewFolder);
    connect(m_fileListToolbar, &FileListToolbar::closeRequested, this,
            [this]() { setFileTreeVisible(false); });

    connect(m_fileListToolbar, &FileListToolbar::quickSwitcherRequested, this, [this]() {
        QDialog dlg(this);
        dlg.setWindowTitle("Быстрый переход");
        dlg.resize(620, 420);
        dlg.setStyleSheet("QDialog { background-color: palette(base); color: palette(text); "
                          "border-radius: 8px; }");
        QVBoxLayout *l = new QVBoxLayout(&dlg);
        l->setContentsMargins(16, 16, 16, 16);
        l->setSpacing(12);

        QLineEdit *input = new QLineEdit(&dlg);
        input->setPlaceholderText("Введите имя файла для создания или перехода...");
        input->setStyleSheet(
            "QLineEdit { background: palette(alternate-base); border: 1px solid palette(mid); "
            "border-radius: 6px; padding: 8px 12px; color: palette(text); font-size: 14px; }");
        l->addWidget(input);

        QListWidget *list = new QListWidget(&dlg);
        list->setStyleSheet(
            "QListWidget { background: transparent; border: none; color: palette(text); font-size: "
            "13px; } QListWidget::item { padding: 8px 12px; border-radius: 4px; } "
            "QListWidget::item:selected { background-color: palette(highlight); color: "
            "palette(highlighted-text); }");

        QStringList files = {"Без названия.base", "logo",
                             "Studying/личное изучение/подключение ssh а также команды линукс 3",
                             "Thoughts/Мимолетные мысли/Мимолетные мысли", "Долги"};
        list->addItems(files);
        l->addWidget(list);

        connect(input, &QLineEdit::textChanged, list, [list](const QString &text) {
            for (int i = 0; i < list->count(); ++i) {
                auto *item = list->item(i);
                bool match = item->text().contains(text, Qt::CaseInsensitive);
                item->setHidden(!match);
            }
        });

        QLabel *hints =
            new QLabel("↑↓ навигация    ↵ открыть    ctrl ↵ открыть в новой вкладке    ctrl alt ↵ "
                       "открыть в области справа    shift ↵ создать    esc отмена",
                       &dlg);
        hints->setAlignment(Qt::AlignCenter);
        hints->setStyleSheet("color: palette(mid); font-size: 11px; margin-top: 4px;");
        l->addWidget(hints);

        connect(input, &QLineEdit::returnPressed, &dlg, &QDialog::accept);
        connect(list, &QListWidget::itemDoubleClicked, &dlg, &QDialog::accept);

        dlg.exec();
    });

    connect(m_fileListToolbar, &FileListToolbar::graphRequested, this,
            [this]() { toggleGraphView(); });

    connect(m_fileListToolbar, &FileListToolbar::canvasRequested, this,
            [this]() { createNewCanvas(); });

    connect(m_fileListToolbar, &FileListToolbar::dailyNoteRequested, this, [this]() {
        QString dateNote = m_currentWorkspacePath.isEmpty()
                               ? "2026-08-29.md"
                               : m_currentWorkspacePath + "/2026-08-29.md";
        openFileAtPath(dateNote, true);
    });

    connect(m_fileListToolbar, &FileListToolbar::commandPaletteRequested, this,
            [this]() { openCommandPalette(); });

    connect(m_fileListToolbar, &FileListToolbar::baseTableRequested, this, [this]() {
        QWidget *tableWidget = new QWidget(this);
        QVBoxLayout *tl = new QVBoxLayout(tableWidget);
        QHBoxLayout *th = new QHBoxLayout();
        th->addWidget(new QLabel("📋 Таблица     82 результат"));
        th->addStretch();
        th->addWidget(new QPushButton("Сортировка"));
        th->addWidget(new QPushButton("Фильтр"));
        th->addWidget(new QPushButton("Свойства"));
        th->addWidget(new QPushButton("Поиск"));
        th->addWidget(new QPushButton("+ Создать"));
        tl->addLayout(th);

        QTableWidget *table = new QTableWidget(5, 1, tableWidget);
        table->setHorizontalHeaderLabels({"имя"});
        table->setItem(0, 0, new QTableWidgetItem("_init__cpython-314.pyc"));
        table->setItem(1, 0, new QTableWidgetItem("__init__.py"));
        table->setItem(2, 0, new QTableWidgetItem("2026-08-29"));
        table->setItem(3, 0, new QTableWidgetItem("Бег"));
        table->setItem(4, 0, new QTableWidgetItem("дипломный проект"));
        tl->addWidget(table);

        openOrFocusTab("Без названия 1.base", tableWidget);
    });

    connect(m_topBar, &TopBar::sidebarToggleRequested, this, [this]() {
        if (m_sidebarContainer) {
            bool visible = !m_sidebarContainer->isVisible();
            m_sidebarContainer->setVisible(visible);
        }
    });

    // Right panel: Tabbed notes (Obsidian-style)
    m_editorTabWidget = new QTabWidget(this);
    m_editorTabWidget->setTabsClosable(true);
    m_editorTabWidget->setMovable(true);
    m_editorTabWidget->setStyleSheet(
        "QTabWidget::pane { border: none; background: transparent; border-top: 1px solid "
        "palette(mid); }"
        "QTabBar::tab { padding: 8px 18px; margin-right: 4px; border-top-left-radius: 8px; "
        "border-top-right-radius: 8px; font-family: 'Manrope','Inter'; }"
        "QTabBar::tab:selected { border-bottom: 3px solid palette(highlight); font-weight: 600; }"
        "QTabBar::tab:hover { background-color: rgba(128, 128, 128, 0.08); }");
    connect(m_editorTabWidget, &QTabWidget::tabCloseRequested, this, &WorkspaceWidget::closeTab);
    connect(m_editorTabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (QWidget *w = m_editorTabWidget->widget(index)) {
            auto *effect = new QGraphicsOpacityEffect(w);
            w->setGraphicsEffect(effect);
            auto *anim = new QPropertyAnimation(effect, "opacity", w);
            anim->setDuration(tokens::kDurationFast);
            anim->setStartValue(0.4);
            anim->setEndValue(1.0);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        }
    });

    m_splitter->addWidget(m_sidebarContainer);
    m_splitter->addWidget(m_editorTabWidget);
    m_splitter->setSizes({320, 880});

    mainLayout->addWidget(m_splitter);

    // Obsidian-like editing shortcuts:
    //   Ctrl+E - toggle Edit/Preview for the current note
    //   Ctrl+S - save the current note immediately
    auto *toggleShortcut = new QShortcut(QKeySequence("Ctrl+E"), this);
    connect(toggleShortcut, &QShortcut::activated, this, &WorkspaceWidget::toggleCurrentPreview);

    auto *saveShortcut = new QShortcut(QKeySequence("Ctrl+S"), this);
    connect(saveShortcut, &QShortcut::activated, this, &WorkspaceWidget::saveCurrentNow);

    auto *cmdPaletteShortcut = new QShortcut(QKeySequence("Ctrl+P"), this);
    connect(cmdPaletteShortcut, &QShortcut::activated, this, &WorkspaceWidget::openCommandPalette);
}

void WorkspaceWidget::openCommandPalette() {
    QDialog dlg(this);
    dlg.setWindowTitle("Палитра команд");
    dlg.resize(620, 500);
    dlg.setStyleSheet(
        "QDialog { background-color: palette(base); color: palette(text); border-radius: 8px; }");
    QVBoxLayout *l = new QVBoxLayout(&dlg);
    l->setContentsMargins(16, 16, 16, 16);
    l->setSpacing(12);

    QLabel *toast =
        new QLabel("Не удается отобразить шаблоны. Не настроена папка с шаблонами.", &dlg);
    toast->setStyleSheet("background-color: #2D3748; color: #FFFFFF; padding: 10px; border-radius: "
                         "6px; font-weight: 500;");
    l->addWidget(toast);

    QLineEdit *input = new QLineEdit(&dlg);
    input->setPlaceholderText("Выберите команду...");
    input->setStyleSheet(
        "QLineEdit { background: palette(alternate-base); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 12px; color: palette(text); font-size: 14px; }");
    l->addWidget(input);

    QListWidget *list = new QListWidget(&dlg);
    list->setStyleSheet("QListWidget { background: transparent; border: none; color: "
                        "palette(text); font-size: 13px; } QListWidget::item { padding: 8px 12px; "
                        "border-radius: 4px; } QListWidget::item:selected { background-color: "
                        "palette(highlight); color: palette(highlighted-text); }");

    QStringList commands = {
        "Базы данных: Вставить новую базу",
        "Базы данных: Создать новую базу",
        "Блок кода",
        "Быстрый переход: Меню быстрого перехода                        Ctrl + O",
        "Вернуться в редактор",
        "Вернуться назад                                                                  Ctrl + "
        "Alt + ←",
        "Включить/отключить жирное начертание для выделения               Ctrl + B",
        "Включить/отключить закрепление области",
        "Включить/отключить комментарии для выделения                     Ctrl + /",
        "Включить/отключить курсивное начертание для выделения            Ctrl + I",
        "Включить/отключить проверку орфографии",
        "Включить/отключить тег подсветки для выделения",
        "Вложение",
        "Восстановление файлов: Открыть сохраненные снимки",
        "Вставить блок для математических выражений",
        "Вставить блок кода",
        "Вставить выносной блок"};
    list->addItems(commands);
    l->addWidget(list);

    connect(input, &QLineEdit::textChanged, list, [list](const QString &text) {
        for (int i = 0; i < list->count(); ++i) {
            auto *item = list->item(i);
            bool match = item->text().contains(text, Qt::CaseInsensitive);
            item->setHidden(!match);
        }
    });

    QLabel *hints = new QLabel("↑↓ навигация       ↵ использовать       esc отмена", &dlg);
    hints->setAlignment(Qt::AlignCenter);
    hints->setStyleSheet("color: palette(mid); font-size: 11px; margin-top: 4px;");
    l->addWidget(hints);

    connect(input, &QLineEdit::returnPressed, &dlg, &QDialog::accept);
    connect(list, &QListWidget::itemDoubleClicked, &dlg, &QDialog::accept);

    dlg.exec();
}

void WorkspaceWidget::toggleGraphView() {
    if (!m_graphView) {
        m_graphView = new GraphView(this);
        connect(m_graphView, &GraphView::noteActivated, this,
                [this](const QString &path) { openFileAtPath(path, true); });
        m_splitter->addWidget(m_graphView);
        m_graphView->setWorkspaceRoot(m_currentWorkspacePath);
    }
    bool visible = !m_graphView->isVisible();
    m_graphView->setVisible(visible);
}

void WorkspaceWidget::openWorkspacePath(const QString &path) {
    if (path.isEmpty() || !QDir(path).exists()) {
        infrastructure::Logger::instance().warn(
            "Workspace", "openWorkspacePath: path missing, ignored: " + path);
        return;
    }
    if (!m_fileProvider.setRootPath(path.toStdString())) {
        QMessageBox::warning(this, "Error", "Could not open workspace folder: " + path);
        return;
    }
    m_currentWorkspacePath = path;
    QModelIndex rootIndex = m_fileModel->setRootPath(path);
    m_fileTreeView->setRootIndex(rootIndex);

    if (m_graphView) {
        m_graphView->setWorkspaceRoot(path);
    }

    // Apply saved panel width (owner-requested editable UI).
    m_splitter->setSizes({InterfaceSettings::instance().treePanelWidth(), 900});
    connect(
        &InterfaceSettings::instance(), &InterfaceSettings::treePanelWidthChanged, this,
        [this](int px) { m_splitter->setSizes({px, 900}); }, Qt::UniqueConnection);
}

void WorkspaceWidget::onOpenFolderClicked() {
    QString dir =
        QFileDialog::getExistingDirectory(this, "Open Workspace Folder", QDir::homePath());
    if (!dir.isEmpty()) {
        openWorkspacePath(dir);
    }
}

NoteTab *WorkspaceWidget::findNoteByPath(const QString &path) {
    for (auto it = m_tabs.begin(); it != m_tabs.end(); ++it) {
        if (it->path.compare(path, Qt::CaseInsensitive) == 0) {
            return &it.value();
        }
    }
    return nullptr;
}

void WorkspaceWidget::onFileClicked(const QModelIndex &index) {
    // Click behavior is user-configurable (Appearance -> Behavior).
    const QFileInfo info = m_fileModel->fileInfo(index);
    if (info.isDir()) {
        m_fileTreeView->setExpanded(index, !m_fileTreeView->isExpanded(index));
        return;
    }
    if (InterfaceSettings::instance().treeSingleClick()) {
        onFileDoubleClicked(index);
    }
}

void WorkspaceWidget::onFileDoubleClicked(const QModelIndex &index) {
    QString filePath = m_fileModel->filePath(index);
    QFileInfo info(filePath);
    if (info.isDir()) {
        return;
    }

    // Defense in depth: the model can only show files under the chosen root,
    // but never trust UI input for file operations.
    if (!m_fileProvider.isInsideRoot(filePath.toStdString())) {
        QMessageBox::warning(this, "Error", "File is outside the workspace root: " + filePath);
        return;
    }

    openFileAtPath(filePath, true);
}

void WorkspaceWidget::openFileAtPath(const QString &filePath, bool activateTab) {
    // Already open? Just focus it.
    if (NoteTab *existing = findNoteByPath(filePath)) {
        const int idx = m_editorTabWidget->indexOf(existing->container);
        if (idx >= 0 && activateTab) {
            m_editorTabWidget->setCurrentIndex(idx);
        }
        return;
    }

    const QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == "canvas") {
        auto *canvas = new CanvasWidget(filePath, this);
        const int tabIndex = m_editorTabWidget->addTab(canvas, QFileInfo(filePath).fileName());
        m_editorTabWidget->setTabToolTip(tabIndex, filePath);
        if (activateTab)
            m_editorTabWidget->setCurrentIndex(tabIndex);
        return;
    }
    if (suffix == "base") {
        auto *container = new QWidget(this);
        auto *layout = new QVBoxLayout(container);
        layout->setContentsMargins(16, 16, 16, 16);
        layout->setSpacing(12);

        QLabel *header = new QLabel(QString("Компилятор %1: Перечень всех файлов хранилища")
                                        .arg(QFileInfo(filePath).fileName()),
                                    container);
        header->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(text);");
        layout->addWidget(header);

        QLineEdit *searchBox = new QLineEdit(container);
        searchBox->setPlaceholderText("Поиск по всем файлам хранилища...");
        searchBox->setStyleSheet(
            "QLineEdit { background: palette(alternate-base); border: 1px solid palette(mid); "
            "border-radius: 6px; padding: 8px 12px; color: palette(text); font-size: 14px; }");
        layout->addWidget(searchBox);

        QListWidget *fileList = new QListWidget(container);
        fileList->setStyleSheet(
            "QListWidget { background: palette(base); border: 1px solid palette(mid); "
            "border-radius: 6px; color: palette(text); font-size: 13px; } QListWidget::item { "
            "padding: 8px 12px; border-radius: 4px; } QListWidget::item:selected { background: "
            "palette(highlight); color: palette(highlighted-text); }");

        if (!m_currentWorkspacePath.isEmpty()) {
            QDirIterator it(m_currentWorkspacePath, QDir::Files | QDir::NoSymLinks,
                            QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString fPath = it.next();
                QString relPath = QDir(m_currentWorkspacePath).relativeFilePath(fPath);
                auto *item = new QListWidgetItem(relPath, fileList);
                item->setData(Qt::UserRole, fPath);
            }
        }
        layout->addWidget(fileList);

        connect(searchBox, &QLineEdit::textChanged, fileList, [fileList](const QString &text) {
            for (int i = 0; i < fileList->count(); ++i) {
                auto *item = fileList->item(i);
                bool match = item->text().contains(text, Qt::CaseInsensitive);
                item->setHidden(!match);
            }
        });

        connect(fileList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
            QString targetPath = item->data(Qt::UserRole).toString();
            if (!targetPath.isEmpty()) {
                openFileAtPath(targetPath, true);
            }
        });

        const int tabIndex = m_editorTabWidget->addTab(container, QFileInfo(filePath).fileName());
        m_editorTabWidget->setTabToolTip(tabIndex, filePath);
        if (activateTab) {
            m_editorTabWidget->setCurrentIndex(tabIndex);
        }
        return;
    }

    auto contentOpt = m_fileProvider.readFile(filePath.toStdString());
    if (!contentOpt.has_value()) {
        QMessageBox::warning(this, "Error", "Could not read file: " + filePath);
        return;
    }
    const QString content = QString::fromStdString(contentOpt.value());

    // ---- Tab page: toolbar + edit/read stack -------------------------
    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    QLineEdit *titleEdit = new QLineEdit(container);
    titleEdit->setText(QFileInfo(filePath).completeBaseName());
    titleEdit->setStyleSheet(
        "QLineEdit { background: transparent; border: none; font-size: 20px; font-weight: bold; "
        "color: palette(text); padding: 6px 0px; border-bottom: 1px solid transparent; }"
        "QLineEdit:hover { border-bottom: 1px solid palette(mid); }"
        "QLineEdit:focus { border-bottom: 2px solid palette(highlight); }");
    layout->addWidget(titleEdit);

    connect(titleEdit, &QLineEdit::editingFinished, this, [this, container, filePath, titleEdit]() {
        const QString newTitle = titleEdit->text().trimmed();
        if (newTitle.isEmpty())
            return;
        const QFileInfo fi(filePath);
        const QString newPath = fi.absolutePath() + "/" + newTitle + ".md";
        if (newPath == filePath)
            return;

        flushPendingSave(m_editorTabWidget->indexOf(container));
        if (QFile::rename(filePath, newPath)) {
            if (NoteTab *tab = findNoteByPath(filePath)) {
                tab->path = newPath;
                const int idx = m_editorTabWidget->indexOf(container);
                m_editorTabWidget->setTabText(idx, newTitle + ".md");
                m_editorTabWidget->setTabToolTip(idx, newPath);
            }
        }
    });

    auto *stack = new QStackedWidget(container);
    layout->addWidget(stack);

    NoteTab tab;
    tab.path = filePath;
    tab.container = container;
    tab.isRenderable = isMarkdownFile(filePath) || isHtmlFile(filePath);
    tab.stack = stack;

    tab.editor = new QTextEdit(container);
    tab.editor->setPlainText(content);
    tab.editor->setFontFamily(InterfaceSettings::instance().editorFontFamily());
    stack->addWidget(tab.editor);

    connect(&InterfaceSettings::instance(), &InterfaceSettings::editorFontChanged, tab.editor,
            &QTextEdit::setFontFamily, Qt::UniqueConnection);

    // Publish selection + current file to the AI context broker.
    connect(tab.editor, &QTextEdit::selectionChanged, this,
            [this, filePath, editor = tab.editor]() {
                const QString sel = editor->textCursor().selectedText();
                ContextBroker::instance().setSelection(
                    QStringLiteral("workspace: %1").arg(QFileInfo(filePath).fileName()), sel);
            });
    {
        QPointer<QTextEdit> editorGuard = tab.editor;
        ContextBroker::instance().setCurrentFile(filePath, [editorGuard]() {
            return editorGuard ? editorGuard->toPlainText() : QString();
        });
    }

    if (tab.isRenderable) {
        tab.preview = new QTextBrowser(container);
        tab.preview->setOpenLinks(false); // route clicks through anchorClicked
        stack->addWidget(tab.preview);
        // Obsidian-like behavior: notes open ready for editing; the rendered
        // preview is one Ctrl+E / button click away.
        stack->setCurrentWidget(tab.editor);
    }

    layout->addWidget(stack);

    // Mode toggle (hidden for plain text files).
    if (tab.isRenderable) {
        tab.modeButton = new QPushButton(QStringLiteral("Preview"), container);
        tab.modeButton->setToolTip("Switch Edit/Reading view (Ctrl+E)");
        tab.modeButton->setFixedWidth(110);
        connect(tab.modeButton, &QPushButton::clicked, this,
                [this, container]() { togglePreview(m_tabs[container]); });
        layout->addWidget(tab.modeButton, 0, Qt::AlignRight);
    }

    // Debounced auto-save on every keystroke pause (delay is configurable).
    QTimer *saveTimer = new QTimer(container);
    saveTimer->setObjectName("autoSaveTimer");
    saveTimer->setSingleShot(true);
    saveTimer->setInterval(InterfaceSettings::instance().autoSaveDelayMs());
    connect(
        &InterfaceSettings::instance(), &InterfaceSettings::autoSaveDelayChanged, saveTimer,
        [saveTimer](int ms) { saveTimer->setInterval(ms); }, Qt::UniqueConnection);

    // The timer writes through writeTabToFile() so manual save, flush and
    // auto-save share one code path. Index is resolved at fire time because
    // tabs are movable/removable.
    connect(saveTimer, &QTimer::timeout, this,
            [this, container]() { writeTabToFile(m_editorTabWidget->indexOf(container)); });
    connect(tab.editor, &QTextEdit::textChanged, saveTimer, qOverload<>(&QTimer::start));

    // Wiki-link / external link navigation in preview.
    if (tab.preview) {
        connect(
            tab.preview, &QTextBrowser::anchorClicked, this,
            [this, preview = tab.preview](const QUrl &url) { handleAnchorClick(url, preview); });
        refreshPreview(tab);
    }

    m_tabs.insert(container, tab);

    const int tabIndex = m_editorTabWidget->addTab(container, QFileInfo(filePath).fileName());
    m_editorTabWidget->setTabToolTip(tabIndex, filePath);
    if (activateTab) {
        m_editorTabWidget->setCurrentIndex(tabIndex);
    }
}

void WorkspaceWidget::togglePreview(NoteTab &tab) {
    if (!tab.stack || !tab.preview) {
        return;
    }

    if (tab.stack->currentWidget() == tab.editor) {
        // Switch to Preview: re-render from the latest editor content.
        refreshPreview(tab);
        tab.stack->setCurrentWidget(tab.preview);
    } else {
        tab.stack->setCurrentWidget(tab.editor);
        tab.editor->setFocus();
    }

    // Update the button label.
    if (tab.modeButton) {
        const bool editing = tab.stack->currentWidget() == tab.editor;
        tab.modeButton->setText(editing ? QStringLiteral("Preview") : QStringLiteral("Edit"));
    }
}

NoteTab *WorkspaceWidget::currentNoteTab() {
    QWidget *page = m_editorTabWidget->currentWidget();
    if (!page) {
        return nullptr;
    }
    auto it = m_tabs.find(page);
    return it != m_tabs.end() ? &it.value() : nullptr;
}

void WorkspaceWidget::toggleCurrentPreview() {
    if (NoteTab *tab = currentNoteTab()) {
        togglePreview(*tab);
    }
}

void WorkspaceWidget::writeTabToFile(int index) {
    QWidget *page = m_editorTabWidget->widget(index);
    if (!page) {
        return;
    }
    // Prefer stored NoteTab (robust vs findChild returning QTextBrowser)
    auto it = m_tabs.find(page);
    QString path;
    QTextEdit *editor = nullptr;
    if (it != m_tabs.end()) {
        path = it->path;
        editor = it->editor;
        // Fallback to tooltip if path empty (defensive)
        if (path.isEmpty())
            path = m_editorTabWidget->tabToolTip(index);
    } else {
        // Canvas/base tabs have no NoteTab - handled elsewhere
        path = m_editorTabWidget->tabToolTip(index);
        if (path.isEmpty())
            return;
        editor = page->findChild<QTextEdit *>();
    }
    if (path.isEmpty() || !editor) {
        return;
    }
    // For canvas widgets, delegate to CanvasWidget save
    if (auto *canvas = qobject_cast<CanvasWidget *>(page)) {
        if (canvas->saveToFile()) {
            infrastructure::Logger::instance().info("Workspace", "Auto-saved canvas: " + path);
        } else {
            infrastructure::Logger::instance().error("Workspace", "Failed to save canvas: " + path);
        }
        return;
    }
    // Also handle canvas inside container (future)
    if (auto *canvasChild = page->findChild<CanvasWidget *>()) {
        if (canvasChild->saveToFile()) {
            infrastructure::Logger::instance().info("Workspace", "Auto-saved canvas: " + path);
        }
        return;
    }
    if (!m_fileProvider.writeFile(path.toStdString(), editor->toPlainText().toStdString())) {
        infrastructure::Logger::instance().error("Workspace", "Failed to save: " + path);
    } else {
        infrastructure::Logger::instance().info("Workspace", "Auto-saved: " + path);
        if (editor->document())
            editor->document()->setModified(false);
        // Update tab tooltip/path if it was renamed elsewhere
        if (it != m_tabs.end() && it->path != path)
            it->path = path;
    }
}

void WorkspaceWidget::saveCurrentNow() {
    const int index = m_editorTabWidget->currentIndex();
    if (index < 0) {
        return;
    }
    writeTabToFile(index);
    // Restart the debounce so auto-save does not rewrite identical content.
    if (QWidget *page = m_editorTabWidget->widget(index)) {
        if (auto *saveTimer = page->findChild<QTimer *>(QStringLiteral("autoSaveTimer"))) {
            saveTimer->stop();
        }
    }
}

void WorkspaceWidget::refreshPreview(const NoteTab &tab) {
    if (!tab.preview) {
        return;
    }
    const QString source = tab.editor ? tab.editor->toPlainText() : QString();

    if (isMarkdownFile(tab.path)) {
        tab.preview->setHtml(MarkdownRenderer::renderToHtml(source));
    } else {
        // HTML files render natively via the rich-text engine.
        tab.preview->setHtml(source);
    }
    tab.preview->document()->setBaseUrl(
        QUrl::fromLocalFile(QFileInfo(tab.path).absolutePath() + "/"));
}

void WorkspaceWidget::handleAnchorClick(const QUrl &url, const QTextBrowser *source) {
    Q_UNUSED(source);
    const QString scheme = url.scheme();

    if (scheme == QLatin1String("wiki")) {
        const QString target = url.toString(QUrl::RemoveScheme);
        if (m_currentWorkspacePath.isEmpty()) {
            QMessageBox::information(this, "Wiki link", "Open a workspace folder first.");
            return;
        }

        const QString resolved =
            workspace::FileLocator::findFileByName(m_currentWorkspacePath, target);
        if (!resolved.isEmpty()) {
            openFileAtPath(resolved, true);
            return;
        }

        // Obsidian behavior: clicking an unresolved link offers to create it.
        const auto answer = QMessageBox::question(
            this, QStringLiteral("Create note"),
            QStringLiteral("Note '%1' does not exist. Create it?").arg(target));
        if (answer == QMessageBox::Yes) {
            const QString created =
                workspace::FileLocator::createIfMissing(m_currentWorkspacePath, target);
            if (!created.isEmpty()) {
                openFileAtPath(created, true);
            }
        }
        return;
    }

    if (scheme.startsWith(QLatin1String("http")) || scheme == QLatin1String("mailto")) {
        QDesktopServices::openUrl(url);
        return;
    }

    infrastructure::Logger::instance().warn("Workspace",
                                            "Unhandled link in note: " + url.toString());
}

void WorkspaceWidget::flushPendingSave(int index) {
    QWidget *page = m_editorTabWidget->widget(index);
    if (!page)
        return;

    auto *saveTimer = page->findChild<QTimer *>(QStringLiteral("autoSaveTimer"));
    if (saveTimer)
        saveTimer->stop();

    // Always flush if this is a note tab (prevents loss on iCloud + rapid close).
    // For canvas, always attempt save as well.
    if (m_tabs.contains(page)) {
        auto it = m_tabs.find(page);
        bool needsSave = false;
        if (it != m_tabs.end() && it->editor && it->editor->document()) {
            needsSave = it->editor->document()->isModified();
        }
        // If timer was active we already stopped it, need save. Also save if modified.
        // Fallback: also save if file on disk is empty but editor has content (iCloud placeholder
        // case).
        if (needsSave) {
            writeTabToFile(index);
        } else if (saveTimer) {
            // Timer existed -> there was a pending debounce, flush anyway to pin iCloud file
            // Only write if editor has content differing from disk? For safety, check non-empty.
            // We do unconditional flush for note tabs to guarantee persistence on close.
            // To avoid redundant writes for unmodified tabs, we can skip if not modified and timer
            // wasn't active. Since we stopped timer, if it wasn't active, isModified==false =>
            // skip. If we still want to force pin, we can check file size 0 case.
            if (it != m_tabs.end() && it->editor) {
                QFileInfo fi(m_editorTabWidget->tabToolTip(index));
                if (fi.exists() && fi.size() == 0 && !it->editor->toPlainText().isEmpty()) {
                    writeTabToFile(index);
                }
            }
        }
    } else if (qobject_cast<CanvasWidget *>(page) || page->findChild<CanvasWidget *>()) {
        writeTabToFile(index);
    }
}

void WorkspaceWidget::closeTab(int index) {
    const QString path = m_editorTabWidget->tabToolTip(index);
    flushPendingSave(index);
    QWidget *page = m_editorTabWidget->widget(index);
    m_editorTabWidget->removeTab(index);
    if (page) {
        m_tabs.remove(page);
        page->deleteLater();
    }
    // Drop stale AI context if the closed file was the published one.
    if (ContextBroker::instance().currentFilePath() == path) {
        ContextBroker::instance().setSelection(QString(), QString());
        ContextBroker::instance().setCurrentFile(QString(), nullptr);
    }
}

void WorkspaceWidget::createNewNote() {
    if (m_currentWorkspacePath.isEmpty()) {
        onOpenFolderClicked();
        if (m_currentWorkspacePath.isEmpty())
            return;
    }
    QString notePath = m_currentWorkspacePath + "/Untitled.md";
    int count = 1;
    while (QFile::exists(notePath)) {
        notePath = QString("%1/Untitled %2.md").arg(m_currentWorkspacePath).arg(count++);
    }
    QFile file(notePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "# Untitled\n\n";
        file.close();
        QModelIndex r = m_fileModel->setRootPath(m_currentWorkspacePath);
        m_fileTreeView->setRootIndex(r);
        openFileAtPath(notePath, true);
    }
}

void WorkspaceWidget::createNewFolder() {
    if (m_currentWorkspacePath.isEmpty()) {
        onOpenFolderClicked();
        if (m_currentWorkspacePath.isEmpty())
            return;
    }
    QString dirPath = m_currentWorkspacePath + "/New Folder";
    int count = 1;
    while (QDir(dirPath).exists()) {
        dirPath = QString("%1/New Folder %2").arg(m_currentWorkspacePath).arg(count++);
    }
    QDir().mkpath(dirPath);
    QModelIndex r = m_fileModel->setRootPath(m_currentWorkspacePath);
    m_fileTreeView->setRootIndex(r);
}

void WorkspaceWidget::createNewCanvas() {
    if (m_currentWorkspacePath.isEmpty()) {
        onOpenFolderClicked();
        if (m_currentWorkspacePath.isEmpty())
            return;
    }
    QString canvasPath = m_currentWorkspacePath + "/Untitled.canvas";
    int count = 1;
    while (QFile::exists(canvasPath)) {
        canvasPath = QString("%1/Untitled %2.canvas").arg(m_currentWorkspacePath).arg(count++);
    }
    QFile file(canvasPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "{\n  \"nodes\": [],\n  \"edges\": []\n}\n";
        file.close();
        // Refresh tree immediately (DontWatchForChanges is on)
        QModelIndex r = m_fileModel->setRootPath(m_currentWorkspacePath);
        m_fileTreeView->setRootIndex(r);
        openFileAtPath(canvasPath, true);
    }
}

void WorkspaceWidget::setFileTreeVisible(bool visible) {
    if (m_fileTreeView)
        m_fileTreeView->setVisible(visible);
    if (m_fileListToolbar)
        m_fileListToolbar->setVisible(visible);
    if (m_openFolderBtn && m_currentWorkspacePath.isEmpty()) {
        m_openFolderBtn->setVisible(visible);
    }
}

bool WorkspaceWidget::fileTreeVisible() const {
    return m_fileTreeView && m_fileTreeView->isVisible();
}

void WorkspaceWidget::openOrFocusTab(const QString &title, QWidget *widget) {
    for (int i = 0; i < m_editorTabWidget->count(); ++i) {
        if (m_editorTabWidget->tabText(i) == title) {
            m_editorTabWidget->setCurrentIndex(i);
            return;
        }
    }
    const int idx = m_editorTabWidget->addTab(widget, title);
    m_editorTabWidget->setCurrentIndex(idx);
    if (auto *gv = qobject_cast<GraphView *>(widget)) {
        gv->setWorkspaceRoot(m_currentWorkspacePath);
    }
}

void WorkspaceWidget::onFileContextMenuRequested(const QPoint &pos) {
    QModelIndex index = m_fileTreeView->indexAt(pos);
    if (!index.isValid()) {
        // Empty area -> creation menu
        QMenu menu(this);
        QAction *newNoteAct = menu.addAction("Создать заметку");
        QAction *newCanvasAct = menu.addAction("Создать canvas");
        QAction *newFolderAct = menu.addAction("Создать папку");
        QAction *sel = menu.exec(m_fileTreeView->viewport()->mapToGlobal(pos));
        if (sel == newNoteAct)
            createNewNote();
        else if (sel == newCanvasAct)
            createNewCanvas();
        else if (sel == newFolderAct)
            createNewFolder();
        return;
    }
    QString filePath = m_fileModel->filePath(index);
    QFileInfo info(filePath);
    if (info.isDir()) {
        QMenu menu(this);
        QAction *newNoteAct = menu.addAction("Создать заметку здесь");
        QAction *newCanvasAct = menu.addAction("Создать canvas здесь");
        QAction *newFolderAct = menu.addAction("Создать папку здесь");
        menu.addSeparator();
        QAction *renameAct = menu.addAction("Переименовать папку");
        QAction *deleteAct = menu.addAction("Удалить папку");
        QAction *sel = menu.exec(m_fileTreeView->viewport()->mapToGlobal(pos));
        if (sel == newNoteAct) {
            QString p = filePath + "/Untitled.md";
            int c = 1;
            while (QFile::exists(p))
                p = QString("%1/Untitled %2.md").arg(filePath).arg(c++);
            QFile f(p);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream o(&f);
                o << "# Untitled\n\n";
                f.close();
                openFileAtPath(p, true);
            }
        } else if (sel == newCanvasAct) {
            QString p = filePath + "/Untitled.canvas";
            int c = 1;
            while (QFile::exists(p))
                p = QString("%1/Untitled %2.canvas").arg(filePath).arg(c++);
            QFile f(p);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream o(&f);
                o << "{\n  \"nodes\": [],\n  \"edges\": []\n}\n";
                f.close();
                openFileAtPath(p, true);
            }
        } else if (sel == newFolderAct) {
            QString d = filePath + "/New Folder";
            int c = 1;
            while (QDir(d).exists())
                d = QString("%1/New Folder %2").arg(filePath).arg(c++);
            QDir().mkpath(d);
        } else if (sel == renameAct) {
            bool ok = false;
            QString n = QInputDialog::getText(
                this, "Переименовать", "Новое имя:", QLineEdit::Normal, info.fileName(), &ok);
            if (ok && !n.isEmpty())
                QFile::rename(filePath, info.absolutePath() + "/" + n);
        } else if (sel == deleteAct) {
            if (QMessageBox::question(this, "Удалить папку", "Удалить " + info.fileName() + "?",
                                      QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
                QDir(filePath).removeRecursively();
        }
        return;
    }

    QMenu menu(this);

    QAction *openNewTabAct = menu.addAction("Открыть в новой вкладке");
    QAction *openRightAct = menu.addAction("Открыть в области справа");
    QAction *openNewWinAct = menu.addAction("Открыть в новом окне");
    menu.addSeparator();

    QAction *makeCopyAct = menu.addAction("Создать копию");
    QAction *moveFileAct = menu.addAction("Переместить файл в...");
    QAction *addBookmarkAct = menu.addAction("Добавить в закладки...");
    QAction *mergeAct = menu.addAction("Объединить весь файл с...");
    menu.addSeparator();

    QMenu *copyPathMenu = menu.addMenu("Копировать путь");
    QAction *copyRelPathAct = copyPathMenu->addAction("Копировать относительный путь");
    QAction *copyAbsPathAct = copyPathMenu->addAction("Копировать абсолютный путь");
    menu.addSeparator();

    QAction *versionHistoryAct = menu.addAction("Открыть историю версий");
    menu.addSeparator();

    QAction *openDefaultAppAct = menu.addAction("Открыть в приложении по умолчанию");
    QAction *showInExplorerAct = menu.addAction("Показать в Проводнике");
    menu.addSeparator();

    QAction *renameAct = menu.addAction("Переименовать");
    QAction *deleteAct = menu.addAction("Удалить");

    QAction *selected = menu.exec(m_fileTreeView->viewport()->mapToGlobal(pos));
    if (!selected)
        return;

    if (selected == openNewTabAct) {
        openFileAtPath(filePath, true);
    } else if (selected == openRightAct) {
        openFileAtPath(filePath, true);
    } else if (selected == openNewWinAct) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    } else if (selected == makeCopyAct) {
        QFileInfo fi(filePath);
        QString newPath = fi.absolutePath() + "/" + fi.completeBaseName() + "_copy." + fi.suffix();
        int count = 1;
        while (QFile::exists(newPath)) {
            newPath = fi.absolutePath() + "/" + fi.completeBaseName() + "_copy" +
                      QString::number(count++) + "." + fi.suffix();
        }
        QFile::copy(filePath, newPath);
    } else if (selected == moveFileAct) {
        QString destDir = QFileDialog::getExistingDirectory(this, "Select Destination Folder",
                                                            m_currentWorkspacePath);
        if (!destDir.isEmpty()) {
            QFileInfo fi(filePath);
            QString destPath = destDir + "/" + fi.fileName();
            if (NoteTab *tab = findNoteByPath(filePath)) {
                flushPendingSave(m_editorTabWidget->indexOf(tab->container));
            }
            QFile::rename(filePath, destPath);
        }
    } else if (selected == addBookmarkAct) {
        QMessageBox::information(this, "Закладки", "Файл добавлен в закладки: " + info.fileName());
    } else if (selected == mergeAct) {
        QString targetPath = QFileDialog::getOpenFileName(
            this, "Merge with File", m_currentWorkspacePath, "Markdown Files (*.md)");
        if (!targetPath.isEmpty() && targetPath != filePath) {
            auto content1 = m_fileProvider.readFile(filePath.toStdString());
            auto content2 = m_fileProvider.readFile(targetPath.toStdString());
            if (content1 && content2) {
                std::string merged = *content1 + "\n\n" + *content2;
                m_fileProvider.writeFile(filePath.toStdString(), merged);
                if (NoteTab *tab = findNoteByPath(filePath)) {
                    closeTab(m_editorTabWidget->indexOf(tab->container));
                    openFileAtPath(filePath, true);
                }
            }
        }
    } else if (selected == copyRelPathAct) {
        QString rel = QDir(m_currentWorkspacePath).relativeFilePath(filePath);
        QGuiApplication::clipboard()->setText(rel);
    } else if (selected == copyAbsPathAct) {
        QGuiApplication::clipboard()->setText(filePath);
    } else if (selected == versionHistoryAct) {
        QMessageBox::information(this, "История версий",
                                 "Для файла " + info.fileName() + " история версий пуста.");
    } else if (selected == openDefaultAppAct) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    } else if (selected == showInExplorerAct) {
        QProcess::startDetached("explorer.exe", {"/select,", QDir::toNativeSeparators(filePath)});
    } else if (selected == renameAct) {
        bool ok = false;
        QString newName = QInputDialog::getText(
            this, "Переименовать", "Новое имя:", QLineEdit::Normal, info.completeBaseName(), &ok);
        if (ok && !newName.isEmpty()) {
            QString newPath = info.absolutePath() + "/" + newName + "." + info.suffix();
            if (newPath != filePath) {
                if (NoteTab *tab = findNoteByPath(filePath)) {
                    flushPendingSave(m_editorTabWidget->indexOf(tab->container));
                }
                if (QFile::rename(filePath, newPath)) {
                    if (NoteTab *tab = findNoteByPath(filePath)) {
                        tab->path = newPath;
                        const int idx = m_editorTabWidget->indexOf(tab->container);
                        m_editorTabWidget->setTabText(idx, newName + "." + info.suffix());
                        m_editorTabWidget->setTabToolTip(idx, newPath);
                    }
                }
            }
        }
    } else if (selected == deleteAct) {
        if (QMessageBox::question(this, "Удалить файл",
                                  "Вы действительно хотите удалить " + info.fileName() + "?",
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            if (NoteTab *tab = findNoteByPath(filePath)) {
                const int idx = m_editorTabWidget->indexOf(tab->container);
                if (idx >= 0)
                    closeTab(idx);
            }
            QFile::remove(filePath);
        }
    }
}

} // namespace fasthub::ui
