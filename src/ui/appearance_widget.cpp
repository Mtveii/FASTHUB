#include "appearance_widget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QInputDialog>
#include <QColorDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QGroupBox>
#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QSplitter>
#include <QListWidgetItem>

#include <algorithm>

#include "theme_manager.h"
#include "interface_settings.h"
#include "icon_util.h"

namespace fasthub::ui {

namespace {
constexpr int kCardW = 108;
constexpr int kCardH = 128;
constexpr int kPreviewH = 60;

QPixmap makePreview(const QColor &bg, const QColor &accent, bool isDark) {
    QPixmap pm(kCardW - 12, kPreviewH);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    QLinearGradient grad(0, 0, pm.width(), pm.height());
    grad.setColorAt(0, bg);
    grad.setColorAt(1, accent);
    p.setBrush(grad);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(0, 0, pm.width(), pm.height(), 8, 8);

    QColor panel = isDark ? QColor(255, 255, 255, 28) : QColor(0, 0, 0, 24);
    p.setBrush(panel);
    p.drawRoundedRect(8, 10, pm.width() - 16, 8, 3, 3);
    p.setBrush(accent);
    p.drawEllipse(QPoint(14, 34), 6, 6);
    p.setBrush(isDark ? QColor(255, 255, 255, 60) : QColor(0, 0, 0, 40));
    p.drawRoundedRect(26, 30, pm.width() - 40, 8, 3, 3);
    p.drawRoundedRect(8, 44, pm.width() - 16, 6, 3, 3);
    return pm;
}

QPixmap makeBigPreview(const ThemeInfo &t) {
    const QColor bg(t.colors.value("bg"));
    const QColor panel(t.colors.value("panel"));
    const QColor accent(t.colors.value("accent"));
    const QColor text(t.colors.value("text"));
    const QColor muted(t.colors.value("muted"));
    const QColor border(t.colors.value("border"));

    QPixmap pm(260, 150);
    pm.fill(bg);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    p.fillRect(0, 0, 260, 22, panel);
    p.setBrush(accent);
    p.drawEllipse(8, 7, 8, 8);
    p.setPen(QPen(text));
    p.drawText(QRect(22, 0, 120, 22), Qt::AlignVCenter, t.name);

    p.fillRect(0, 22, 46, 128, panel);
    for (int i = 0; i < 4; ++i) {
        p.setBrush(i == 0 ? accent : border);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(8, 32 + i * 28, 30, 18, 4, 4);
    }

    p.setPen(Qt::NoPen);
    for (int i = 0; i < 5; ++i) {
        p.setBrush(i == 0 ? QColor(accent.red(), accent.green(), accent.blue(), 40) : panel);
        p.drawRoundedRect(54, 30 + i * 24, 120, 16, 3, 3);
        p.setBrush(muted);
        p.drawRoundedRect(62, 35 + i * 24, 70, 6, 2, 2);
    }

    p.setBrush(panel);
    p.drawRoundedRect(184, 30, 68, 112, 4, 4);
    p.fillRect(54, 24, 260 - 54, 2, accent);
    return pm;
}

} // namespace

AppearanceWidget::AppearanceWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
    refreshList();

    connect(&ThemeManager::instance(), &ThemeManager::themeApplied, this,
            [this](const QString &) { markCurrentCard(); });
}

void AppearanceWidget::setupUi() {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(1);

    // Left navigation menu — Obsidian-style, скругления 8px, 120ms hover
    m_navMenu = new QListWidget(this);
    m_navMenu->setFixedWidth(240);
    m_navMenu->setStyleSheet(
        "QListWidget { background: palette(base); border: none; border-right: 1px solid "
        "palette(mid); padding: 10px 8px; }"
        "QListWidget::item { padding: 9px 14px; border-radius: 8px; margin: 3px 4px; color: "
        "palette(text); }"
        "QListWidget::item:hover { background-color: rgba(128, 128, 128, 0.10); }"
        "QListWidget::item:selected { background-color: palette(highlight); color: "
        "palette(highlighted-text); }"
        "QScrollBar:vertical { width: 6px; background: transparent; }"
        "QScrollBar::handle:vertical { background: rgba(128,128,128,0.25); border-radius: 3px; "
        "min-height: 24px; }"
        "QScrollBar::handle:vertical:hover { background: rgba(128,128,128,0.4); }");

    auto addItem = [this](const QString &text, bool isHeader = false) {
        auto *item = new QListWidgetItem(text, m_navMenu);
        if (isHeader) {
            item->setFlags(Qt::NoItemFlags);
            item->setForeground(palette().color(QPalette::Disabled, QPalette::Text));
            QFont f = item->font();
            f.setBold(true);
            f.setPointSize(f.pointSize() - 1);
            item->setFont(f);
        }
    };

    addItem("Настройки", true);
    addItem("Общие");
    addItem("Оформление");
    addItem("Интерфейс");
    addItem("Редактор");
    addItem("Файлы и ссылки");
    addItem("Сочетания клавиш");
    addItem("Связка ключей");
    addItem("Встроенные плагины");
    addItem("Сторонние плагины");

    addItem("Встроенные плагины", true);
    addItem("Быстрый переход");
    addItem("Восстановление файлов");
    addItem("Ежедневные заметки");
    addItem("Компоновщик заметок");
    addItem("Обратные ссылки");
    addItem("Палитра команд");
    addItem("Предпросмотр страницы");
    addItem("Синхронизация");
    addItem("Холст");
    addItem("Шаблоны");

    splitter->addWidget(m_navMenu);

    // Right stacked — Fade 120ms OutCubic, не блокирует
    m_settingsStack = new FadeStackedWidget(this);

    // 1. Общие (General) - matches 134759.png
    QWidget *generalPage = new QWidget(this);
    QVBoxLayout *genLayout = new QVBoxLayout(generalPage);
    genLayout->setContentsMargins(24, 24, 24, 24);
    genLayout->setSpacing(16);
    QLabel *genTitle = new QLabel("Версия 1.13.7", generalPage);
    genTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    genLayout->addWidget(genTitle);
    genLayout->addWidget(new QLabel("Версия установщика: 1.13.7\nСписок изменений", generalPage));

    QGroupBox *updatesBox = new QGroupBox("Автоматические обновления", generalPage);
    QVBoxLayout *upLayout = new QVBoxLayout(updatesBox);
    upLayout->addWidget(
        new QCheckBox("Проверять и скачивать доступные обновления автоматически.", updatesBox));
    genLayout->addWidget(updatesBox);

    QGroupBox *langBox = new QGroupBox("Язык", generalPage);
    QVBoxLayout *lLayout = new QVBoxLayout(langBox);
    QHBoxLayout *langRow = new QHBoxLayout();
    langRow->addWidget(new QLabel("Изменяет язык интерфейса."));
    QComboBox *langCombo = new QComboBox();
    langCombo->addItems({"Русский", "English"});
    langRow->addWidget(langCombo);
    lLayout->addLayout(langRow);
    genLayout->addWidget(langBox);
    genLayout->addStretch();
    m_settingsStack->addWidget(generalPage);

    // 2. Оформление (Appearance)
    QWidget *appearancePage = new QWidget(this);
    QVBoxLayout *appLayout = new QVBoxLayout(appearancePage);
    appLayout->setContentsMargins(24, 24, 24, 24);
    appLayout->setSpacing(16);

    auto *appHeader = new QLabel("Appearance", appearancePage);
    appHeader->setProperty("heading", true);
    appLayout->addWidget(appHeader);

    auto *previewRow = new QHBoxLayout();
    for (const QString &id : {QStringLiteral("fasthub-dark"), QStringLiteral("fasthub-light")}) {
        for (const auto &t : ThemeManager::instance().themes()) {
            if (t.id != id)
                continue;
            auto *card = new QLabel(this);
            card->setPixmap(makeBigPreview(t));
            card->setFixedSize(260, 150);
            previewRow->addWidget(card);
        }
    }
    previewRow->addStretch();
    appLayout->addLayout(previewRow);

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText("Search themes...");
    m_search->setFixedWidth(240);
    connect(m_search, &QLineEdit::textChanged, this, &AppearanceWidget::onFilterChanged);
    appLayout->addWidget(m_search);

    m_grid = new QListWidget(this);
    m_grid->setViewMode(QListWidget::IconMode);
    m_grid->setResizeMode(QListWidget::Adjust);
    m_grid->setMovement(QListWidget::Static);
    m_grid->setGridSize(QSize(kCardW + 12, kCardH + 12));
    m_grid->setSelectionMode(QAbstractItemView::SingleSelection);
    m_grid->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_grid, &QListWidget::itemClicked, this, &AppearanceWidget::onCardClicked);
    connect(m_grid, &QListWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        auto *item = m_grid->itemAt(pos);
        if (!item)
            return;
        const QString id = item->data(Qt::UserRole).toString();
        if (!id.startsWith(QStringLiteral("custom-")))
            return;

        QMenu menu(this);
        QAction *del = menu.addAction("Delete custom theme");
        if (menu.exec(m_grid->mapToGlobal(pos)) == del) {
            ThemeManager::instance().removeCustomTheme(id);
            refreshList();
        }
    });
    appLayout->addWidget(m_grid);
    m_settingsStack->addWidget(appearancePage);

    // 3. Интерфейс (Interface) - matches 134811.png
    QWidget *interfacePage = new QWidget(this);
    QVBoxLayout *intLayout = new QVBoxLayout(interfacePage);
    intLayout->setContentsMargins(24, 24, 24, 24);
    intLayout->setSpacing(16);
    intLayout->addWidget(
        new QCheckBox("Отображать строку заголовка для каждой вкладки.", interfacePage));
    intLayout->addWidget(new QCheckBox(
        "Отображать вертикальную панель инструментов в боковой части окна.", interfacePage));
    intLayout->addStretch();
    m_settingsStack->addWidget(interfacePage);

    // 4. Редактор (Editor) - matches 134819.png
    QWidget *editorPage = new QWidget(this);
    QVBoxLayout *edLayout = new QVBoxLayout(editorPage);
    edLayout->setContentsMargins(24, 24, 24, 24);
    edLayout->setSpacing(16);

    QHBoxLayout *modeRow = new QHBoxLayout();
    modeRow->addWidget(new QLabel("Режим редактирования по умолчанию"));
    QComboBox *modeCombo = new QComboBox();
    modeCombo->addItems({"Динамический просмотр", "Редактор", "Просмотр"});
    modeRow->addWidget(modeCombo);
    edLayout->addLayout(modeRow);

    edLayout->addWidget(new QCheckBox(
        "Добавить в строку состояния элемент отображающий текущий режим редактора.", editorPage));
    edLayout->addWidget(new QCheckBox(
        "Отображать имя файла в виде редактируемого заголовка в теле файла.", editorPage));
    edLayout->addWidget(new QCheckBox("Повышать удобство чтения длинных абзацев...", editorPage));
    edLayout->addStretch();
    m_settingsStack->addWidget(editorPage);

    // 5. Файлы и ссылки (Files & Links)
    QWidget *filesLinksPage = new QWidget(this);
    QVBoxLayout *flLayout = new QVBoxLayout(filesLinksPage);
    flLayout->setContentsMargins(24, 24, 24, 24);
    flLayout->setSpacing(16);
    flLayout->addWidget(new QCheckBox("Отображать только существующие файлы", filesLinksPage));
    flLayout->addWidget(new QCheckBox("Отображать вложенные файлы", filesLinksPage));
    flLayout->addStretch();
    m_settingsStack->addWidget(filesLinksPage);

    // 6. Сочетания клавиш (Hotkeys)
    QWidget *hotkeysPage = new QWidget(this);
    QVBoxLayout *hkLayout = new QVBoxLayout(hotkeysPage);
    hkLayout->setContentsMargins(24, 24, 24, 24);
    hkLayout->addWidget(new QLabel("Настройка горячих клавиш и сочетаний", hotkeysPage));
    hkLayout->addStretch();
    m_settingsStack->addWidget(hotkeysPage);

    // 7. Связка ключей (Secure Storage) - matches 134833.png
    QWidget *keysPage = new QWidget(this);
    QVBoxLayout *kLayout = new QVBoxLayout(keysPage);
    kLayout->setContentsMargins(24, 24, 24, 24);
    kLayout->setSpacing(16);
    QGroupBox *secBox = new QGroupBox("Секреты", keysPage);
    QVBoxLayout *sLayout = new QVBoxLayout(secBox);
    QPushButton *addSecBtn = new QPushButton("+");
    addSecBtn->setFixedWidth(30);
    sLayout->addWidget(addSecBtn);
    sLayout->addWidget(new QLabel("Секреты не добавлены. Секреты используются для хранения таких "
                                  "данных, как ключи API и пароли.",
                                  secBox));
    kLayout->addWidget(secBox);
    kLayout->addStretch();
    m_settingsStack->addWidget(keysPage);

    // 8. Встроенные плагины (Core Plugins)
    QWidget *corePluginsPage = new QWidget(this);
    QVBoxLayout *cpLayout = new QVBoxLayout(corePluginsPage);
    cpLayout->setContentsMargins(24, 24, 24, 24);
    cpLayout->addWidget(new QLabel("Встроенные плагины FastHub / Obsidian", corePluginsPage));
    cpLayout->addStretch();
    m_settingsStack->addWidget(corePluginsPage);

    // 9. Сторонние плагины (Community Plugins) - matches 134855.png
    QWidget *comPluginsPage = new QWidget(this);
    QVBoxLayout *comLayout = new QVBoxLayout(comPluginsPage);
    comLayout->setContentsMargins(24, 24, 24, 24);
    comLayout->setSpacing(16);
    comLayout->addWidget(
        new QLabel("Плагины сообщества, безопасность и проверка кода.", comPluginsPage));
    QPushButton *safetyBtn = new QPushButton("Выйти из ограниченного режима");
    safetyBtn->setFixedWidth(220);
    comLayout->addWidget(safetyBtn);
    comLayout->addStretch();
    m_settingsStack->addWidget(comPluginsPage);

    // 10. Быстрый переход (Quick Switcher) - matches 134909.png
    QWidget *qsPage = new QWidget(this);
    QVBoxLayout *qsLayout = new QVBoxLayout(qsPage);
    qsLayout->setContentsMargins(24, 24, 24, 24);
    qsLayout->setSpacing(16);
    qsLayout->addWidget(new QCheckBox("Отображать только существующие файлы", qsPage));
    qsLayout->addWidget(new QCheckBox("Отображать вложенные файлы", qsPage));
    qsLayout->addStretch();
    m_settingsStack->addWidget(qsPage);

    // 11. Восстановление файлов
    QWidget *frPage = new QWidget(this);
    frPage->setLayout(new QVBoxLayout());
    frPage->layout()->addWidget(new QLabel("Восстановление файлов из снимков", frPage));
    m_settingsStack->addWidget(frPage);

    // 12. Ежедневные заметки (Daily Notes) - matches 134919.png
    QWidget *dnPage = new QWidget(this);
    QVBoxLayout *dnLayout = new QVBoxLayout(dnPage);
    dnLayout->setContentsMargins(24, 24, 24, 24);
    dnLayout->setSpacing(16);
    dnLayout->addWidget(new QLabel("Формат даты: 2026-08-29", dnPage));
    dnLayout->addWidget(new QLabel("Пользовательский формат: YYYY-MM-DD", dnPage));
    dnLayout->addStretch();
    m_settingsStack->addWidget(dnPage);

    // 13. Компоновщик заметок
    QWidget *nbPage = new QWidget(this);
    nbPage->setLayout(new QVBoxLayout());
    nbPage->layout()->addWidget(new QLabel("Компоновщик заметок (Note Composer)", nbPage));
    m_settingsStack->addWidget(nbPage);

    // 14. Обратные ссылки
    QWidget *bkPage = new QWidget(this);
    bkPage->setLayout(new QVBoxLayout());
    bkPage->layout()->addWidget(new QLabel("Обратные ссылки (Backlinks)", bkPage));
    m_settingsStack->addWidget(bkPage);

    // 15. Палитра команд
    QWidget *cmdPage = new QWidget(this);
    cmdPage->setLayout(new QVBoxLayout());
    cmdPage->layout()->addWidget(new QLabel("Палитра команд (Command palette)", cmdPage));
    m_settingsStack->addWidget(cmdPage);

    // 16. Предпросмотр страницы
    QWidget *ppPage = new QWidget(this);
    ppPage->setLayout(new QVBoxLayout());
    ppPage->layout()->addWidget(new QLabel("Предпросмотр страницы (Page preview)", ppPage));
    m_settingsStack->addWidget(ppPage);

    // 17. Синхронизация
    QWidget *syncPage = new QWidget(this);
    syncPage->setLayout(new QVBoxLayout());
    syncPage->layout()->addWidget(new QLabel("Синхронизация (Sync)", syncPage));
    m_settingsStack->addWidget(syncPage);

    // 18. Холст (Canvas)
    QWidget *cvPage = new QWidget(this);
    cvPage->setLayout(new QVBoxLayout());
    cvPage->layout()->addWidget(new QLabel("Холст (Canvas settings)", cvPage));
    m_settingsStack->addWidget(cvPage);

    // 19. Шаблоны (Templates)
    QWidget *tempPage = new QWidget(this);
    tempPage->setLayout(new QVBoxLayout());
    tempPage->layout()->addWidget(new QLabel("Шаблоны (Templates settings)", tempPage));
    m_settingsStack->addWidget(tempPage);

    splitter->addWidget(m_settingsStack);
    splitter->setSizes({220, 780});

    mainLayout->addWidget(splitter);

    // Connect left menu item clicks to right stack pages
    connect(m_navMenu, &QListWidget::currentRowChanged, this, [this](int row) {
        // Map row to stack index (accounting for headers)
        // Non-header rows:
        // row 1: Общие (0)
        // row 2: Оформление (1)
        // row 3: Интерфейс (2)
        // row 4: Редактор (3)
        // row 5: Файлы и ссылки (4)
        // row 6: Сочетания клавиш (5)
        // row 7: Связка ключей (6)
        // row 8: Встроенные плагины (7)
        // row 9: Сторонние плагины (8)
        // row 11: Быстрый переход (9)
        // row 12: Восстановление файлов (10)
        // row 13: Ежедневные заметки (11)
        // row 14: Компоновщик заметок (12)
        // row 15: Обратные ссылки (13)
        // row 16: Палитра команд (14)
        // row 17: Предпросмотр страницы (15)
        // row 18: Синхронизация (16)
        // row 19: Холст (17)
        // row 20: Шаблоны (18)
        static const QMap<int, int> rowMap = {{1, 0},   {2, 1},   {3, 2},   {4, 3},   {5, 4},
                                              {6, 5},   {7, 6},   {8, 7},   {9, 8},   {11, 9},
                                              {12, 10}, {13, 11}, {14, 12}, {15, 13}, {16, 14},
                                              {17, 15}, {18, 16}, {19, 17}, {20, 18}};
        if (rowMap.contains(row)) {
            m_settingsStack->setCurrentIndex(rowMap[row]);
        }
    });

    m_navMenu->setCurrentRow(1); // default to Общие
}

void AppearanceWidget::refreshList() {
    if (!m_grid)
        return;
    m_grid->clear();

    for (const auto &theme : ThemeManager::instance().themes()) {
        auto *item = new QListWidgetItem(m_grid);
        item->setData(Qt::UserRole, theme.id);
        item->setSizeHint(QSize(kCardW, kCardH));
        m_grid->setItemWidget(item, buildCard(theme.id, false));
        if (theme.id == ThemeManager::instance().currentThemeId()) {
            item->setSelected(true);
        }
    }

    auto *createItem = new QListWidgetItem(m_grid);
    createItem->setData(Qt::UserRole, QStringLiteral("__create__"));
    createItem->setSizeHint(QSize(kCardW, kCardH));
    m_grid->setItemWidget(createItem, buildCard(QString(), true));

    if (m_search) {
        onFilterChanged(m_search->text());
    }
    markCurrentCard();
}

void AppearanceWidget::markCurrentCard() {
    if (!m_grid)
        return;
    const QString currentId = ThemeManager::instance().currentThemeId();
    for (int i = 0; i < m_grid->count(); ++i) {
        auto *item = m_grid->item(i);
        item->setSelected(item->data(Qt::UserRole).toString() == currentId);
    }
}

void AppearanceWidget::onFilterChanged(const QString &text) {
    if (!m_grid)
        return;
    const QString needle = text.trimmed();
    for (int i = 0; i < m_grid->count(); ++i) {
        auto *item = m_grid->item(i);
        const QString id = item->data(Qt::UserRole).toString();
        if (id == QStringLiteral("__create__")) {
            m_grid->setRowHidden(i, false);
            continue;
        }
        bool found = false;
        for (const auto &t : ThemeManager::instance().themes()) {
            if (t.id == id) {
                found = t.name.contains(needle, Qt::CaseInsensitive);
                break;
            }
        }
        m_grid->setRowHidden(i, !found);
    }
}

void AppearanceWidget::onCardClicked(QListWidgetItem *item) {
    const QString id = item->data(Qt::UserRole).toString();
    if (id == QStringLiteral("__create__")) {
        onCreateCustomTheme();
        return;
    }
    ThemeManager::instance().applyTheme(id);
    markCurrentCard();
}

void AppearanceWidget::onCreateCustomTheme() {
    bool ok = false;
    const QString name = QInputDialog::getText(this, "New Custom Theme",
                                               "Theme name:", QLineEdit::Normal, "My Theme", &ok);
    if (!ok || name.trimmed().isEmpty())
        return;

    const std::vector<std::pair<QString, QColor>> roles = {
        {"bg", QColor("#0B0E14")},     {"panel", QColor("#12161F")},
        {"text", QColor("#E6E9F0")},   {"muted", QColor("#8A93A6")},
        {"accent", QColor("#6E7BFF")}, {"accentHover", QColor("#8B96FF")},
        {"border", QColor("#1E2430")},
    };
    QHash<QString, QString> colors;
    for (const auto &[role, fallback] : roles) {
        const QColor chosen =
            QColorDialog::getColor(fallback, this, QStringLiteral("Pick '%1'").arg(role));
        if (!chosen.isValid())
            return;
        colors.insert(role, chosen.name());
    }

    ThemeInfo info;
    info.id = QStringLiteral("custom-%1").arg(name.trimmed().toLower().replace(' ', '-'));
    info.name = name.trimmed();
    info.isDark = QColor(colors.value("bg")).lightnessF() < 0.5;
    info.colors = colors;

    if (!ThemeManager::instance().saveCustomTheme(info)) {
        QMessageBox::warning(this, "Themes",
                             "Could not save the theme (id conflict or disk error).");
        return;
    }
    refreshList();
}

QWidget *AppearanceWidget::buildCard(const QString &themeId, bool isCreateCard) {
    auto *card = new QWidget(this);
    card->setFixedSize(kCardW, kCardH);
    auto *v = new QVBoxLayout(card);
    v->setContentsMargins(6, 6, 6, 6);
    v->setSpacing(2);

    if (isCreateCard) {
        auto *plus = new QLabel(card);
        plus->setPixmap(
            IconUtil::tintedIcon(":/icons/plus-lg.svg", QColor("#8A93A6"), 30).pixmap(30, 30));
        plus->setAlignment(Qt::AlignCenter);
        plus->setFixedHeight(kPreviewH);
        plus->setStyleSheet("border: 1px dashed #8A93A6; border-radius: 8px;");
        v->addWidget(plus);

        auto *name = new QLabel("Custom theme", card);
        name->setAlignment(Qt::AlignCenter);
        name->setWordWrap(true);
        name->setProperty("role", "muted");
        v->addWidget(name);
        return card;
    }

    const auto &themes = ThemeManager::instance().themes();
    const auto it = std::find_if(themes.begin(), themes.end(),
                                 [&](const ThemeInfo &t) { return t.id == themeId; });
    if (it == themes.end()) {
        return card;
    }
    const ThemeInfo &theme = *it;

    auto *preview = new QLabel(card);
    preview->setPixmap(makePreview(QColor(theme.colors.value("bg")),
                                   QColor(theme.colors.value("accent")), theme.isDark));
    preview->setFixedHeight(kPreviewH);
    v->addWidget(preview);

    auto *row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    auto *name = new QLabel(theme.name, card);
    name->setWordWrap(true);
    name->setStyleSheet("font-family:'Manrope','Inter'; font-size: 11px;");
    row->addWidget(name, 1);

    auto *flip = new QPushButton(card);
    flip->setIcon(IconUtil::tintedIcon(theme.isDark ? QStringLiteral(":/icons/sun.svg")
                                                    : QStringLiteral(":/icons/moon.svg"),
                                       QColor(theme.colors.value("muted", "#8A93A6")), 16));
    flip->setFixedSize(26, 26);
    flip->setToolTip("Switch dark/light");
    const QString flippedId = theme.isDark ? theme.id.chopped(5) + QStringLiteral("light")
                                           : theme.id.chopped(6) + QStringLiteral("dark");
    connect(flip, &QPushButton::clicked, card, [this, flippedId](bool) {
        ThemeManager::instance().applyTheme(flippedId);
        refreshList();
    });
    row->addWidget(flip);
    v->addLayout(row);

    return card;
}

} // namespace fasthub::ui
