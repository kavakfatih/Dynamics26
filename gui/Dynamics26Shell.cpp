#include "Dynamics26Shell.h"

#include <femcae/femcae.h>

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QFont>
#include <QFrame>
#include <QKeySequence>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaObject>
#include <QPlainTextEdit>
#include <QSize>
#include <QSizePolicy>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QTabBar>
#include <QTabWidget>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

#ifndef DYNAMICS26_GUI_MILESTONE
#define DYNAMICS26_GUI_MILESTONE "1.1.0-alpha.1"
#endif

namespace {

constexpr int kNavigatorRoleInspectorPage = Qt::UserRole + 1;
constexpr int kNavigatorOpenResults = -100;

struct WorkspaceParts {
    QTreeWidget *navigator = nullptr;
    QWidget *viewport = nullptr;
    QWidget *legacyInspector = nullptr;
};

struct InspectorShell {
    QFrame *panel = nullptr;
    QTabWidget *tabs = nullptr;
    QLabel *context = nullptr;
    QLabel *emptyState = nullptr;
};

QWidget *makeExpandingSpacer(QWidget *parent)
{
    auto *spacer = new QWidget(parent);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    return spacer;
}

QLabel *makePanelTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    QFont font = label->font();
    font.setBold(true);
    font.setPointSizeF(qMax(9.0, font.pointSizeF() - 1.0));
    label->setFont(font);
    return label;
}

void setStandardIcon(QMainWindow &window, QAction *action, QStyle::StandardPixmap pixmap)
{
    if (action != nullptr && window.style() != nullptr) {
        action->setIcon(window.style()->standardIcon(pixmap));
    }
}

bool invokeMainWindowSlot(QMainWindow &window, const char *slotName)
{
    return QMetaObject::invokeMethod(&window, slotName, Qt::DirectConnection);
}

QAction *makeSlotAction(
    QMainWindow &window,
    const QString &text,
    const QString &toolTip,
    const char *slotName,
    QStyle::StandardPixmap icon)
{
    auto *action = new QAction(text, &window);
    action->setToolTip(toolTip);
    setStandardIcon(window, action, icon);
    const QByteArray target(slotName);
    QObject::connect(action, &QAction::triggered, &window, [&window, target] {
        if (!invokeMainWindowSlot(window, target.constData())) {
            QMessageBox::warning(
                &window,
                QStringLiteral("Dynamics26"),
                QStringLiteral("Komut mevcut engineering katmanına bağlanamadı: %1")
                    .arg(QString::fromLatin1(target)));
        }
    });
    return action;
}

WorkspaceParts detachLegacyWorkspace(QMainWindow &window)
{
    WorkspaceParts parts;
    auto *legacySplitter = qobject_cast<QSplitter *>(window.centralWidget());
    if (legacySplitter == nullptr || legacySplitter->count() < 3) {
        return parts;
    }

    parts.navigator = qobject_cast<QTreeWidget *>(legacySplitter->widget(0));
    parts.viewport = legacySplitter->widget(1);
    parts.legacyInspector = legacySplitter->widget(2);
    if (parts.navigator == nullptr || parts.viewport == nullptr || parts.legacyInspector == nullptr) {
        return WorkspaceParts {};
    }

    // Corrective Alpha.1'in temel farkı: görünür workspace artık eski
    // MainWindow splitter'ını yalnız yeniden etiketlemez. Çalışan widget'lar
    // servis/işlevleri korunarak eski taşıyıcıdan ayrılır ve shell-owned yeni
    // layout'a yerleştirilir.
    parts.navigator->setParent(nullptr);
    parts.viewport->setParent(nullptr);
    parts.legacyInspector->setParent(nullptr);
    legacySplitter->deleteLater();
    return parts;
}

void configureNavigatorWidget(QTreeWidget *navigator)
{
    if (navigator == nullptr) {
        return;
    }

    navigator->setObjectName(QStringLiteral("Dynamics26Navigator"));
    navigator->setHeaderHidden(true);
    navigator->setRootIsDecorated(false);
    navigator->setUniformRowHeights(true);
    navigator->setIndentation(14);
    navigator->setFrameShape(QFrame::NoFrame);
    navigator->setSelectionMode(QAbstractItemView::SingleSelection);
    navigator->setAccessibleName(QStringLiteral("Project Navigator"));

    // Alpha.2'de gerçek project object modeli gelecektir. Corrective Alpha.1
    // ise debug/solver implementation ağacını göstermemeli; yalnız gerçekten
    // bağlı engineering yüzeylerine giden sığ ve anlaşılır bölümler tutulur.
    navigator->clear();

    auto addSection = [navigator](const QString &name, int inspectorPage) {
        auto *item = new QTreeWidgetItem(navigator, {name});
        item->setData(0, kNavigatorRoleInspectorPage, inspectorPage);
        return item;
    };

    addSection(QStringLiteral("Geometri"), 0);
    addSection(QStringLiteral("Malzemeler"), 2);
    addSection(QStringLiteral("Kesitler"), 3);
    addSection(QStringLiteral("Mesh"), 1);

    auto *analyses = addSection(QStringLiteral("Analizler"), 5);
    auto *loads = new QTreeWidgetItem(analyses, {QStringLiteral("Yükler ve Sınır Şartları")});
    loads->setData(0, kNavigatorRoleInspectorPage, 4);
    auto *settings = new QTreeWidgetItem(analyses, {QStringLiteral("Analiz Ayarları")});
    settings->setData(0, kNavigatorRoleInspectorPage, 5);
    analyses->setExpanded(true);

    // MainWindow'ın mevcut result-tree güncelleme sözleşmesi top-level
    // "Sonuçlar" öğesini arar. Bu kullanıcıya anlamlı isim korunarak çalışan
    // verification/result yolu kırılmadan shell'e taşınır.
    addSection(QStringLiteral("Sonuçlar"), kNavigatorOpenResults);
    navigator->clearSelection();
    navigator->setCurrentItem(nullptr);
}

QWidget *wrapNavigator(QTreeWidget *navigator)
{
    auto *panel = new QFrame;
    panel->setObjectName(QStringLiteral("Dynamics26NavigatorPanel"));
    panel->setFrameShape(QFrame::NoFrame);
    panel->setMinimumWidth(220);
    panel->setMaximumWidth(360);
    panel->setAutoFillBackground(true);

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(10, 10, 8, 8);
    layout->setSpacing(6);
    layout->addWidget(makePanelTitle(QStringLiteral("PROJECT"), panel));
    layout->addWidget(navigator, 1);
    return panel;
}

InspectorShell wrapInspector(QWidget *legacyInspector)
{
    InspectorShell shell;
    if (legacyInspector == nullptr) {
        return shell;
    }

    shell.panel = new QFrame;
    shell.panel->setObjectName(QStringLiteral("Dynamics26InspectorPanel"));
    shell.panel->setFrameShape(QFrame::NoFrame);
    shell.panel->setMinimumWidth(300);
    shell.panel->setMaximumWidth(440);
    shell.panel->setAutoFillBackground(true);
    shell.panel->setAccessibleName(QStringLiteral("Inspector"));

    auto *layout = new QVBoxLayout(shell.panel);
    layout->setContentsMargins(10, 10, 10, 8);
    layout->setSpacing(6);
    layout->addWidget(makePanelTitle(QStringLiteral("INSPECTOR"), shell.panel));

    shell.context = new QLabel(QStringLiteral("Seçim yok"), shell.panel);
    shell.context->setWordWrap(true);
    layout->addWidget(shell.context);

    shell.emptyState = new QLabel(
        QStringLiteral("Project Navigator’dan bir öğe seçerek mevcut mühendislik araçlarını görüntüleyin."),
        shell.panel);
    shell.emptyState->setWordWrap(true);
    shell.emptyState->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    shell.emptyState->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    layout->addWidget(shell.emptyState, 1);

    shell.tabs = qobject_cast<QTabWidget *>(legacyInspector);
    if (shell.tabs != nullptr) {
        shell.tabs->setObjectName(QStringLiteral("Dynamics26EngineeringInspector"));
        shell.tabs->setDocumentMode(false);
        shell.tabs->setUsesScrollButtons(false);
        shell.tabs->setAccessibleName(QStringLiteral("Engineering Inspector"));
        if (shell.tabs->tabBar() != nullptr) {
            // Legacy yatay tab strip shell'in ana bilgi mimarisi değildir.
            // Bölüm seçimi Navigator üzerinden yapılır; gerçek contextual
            // property modeli Alpha.2'de gelecektir.
            shell.tabs->tabBar()->hide();
        }
        shell.tabs->hide();
    }
    layout->addWidget(legacyInspector, 1);
    legacyInspector->hide();
    return shell;
}

int tabIndexContaining(QTabWidget *tabs, const QString &text)
{
    if (tabs == nullptr) {
        return -1;
    }
    for (int i = 0; i < tabs->count(); ++i) {
        if (tabs->tabText(i).contains(text, Qt::CaseInsensitive)) {
            return i;
        }
    }
    return -1;
}

void activateUtilityTab(QDockWidget *dock, QTabWidget *tabs, const QString &text)
{
    if (dock == nullptr || tabs == nullptr) {
        return;
    }
    dock->show();
    const int index = tabIndexContaining(tabs, text);
    if (index >= 0) {
        tabs->setCurrentIndex(index);
    }
}

void configureUtilityArea(QDockWidget *dock)
{
    if (dock == nullptr) {
        return;
    }

    dock->setObjectName(QStringLiteral("Dynamics26UtilityArea"));
    dock->setWindowTitle(QStringLiteral("Results & Diagnostics"));
    dock->setAllowedAreas(Qt::BottomDockWidgetArea);
    dock->setFeatures(QDockWidget::DockWidgetClosable);
    dock->setMinimumHeight(150);

    auto *tabs = dock->findChild<QTabWidget *>();
    if (tabs == nullptr) {
        return;
    }

    tabs->setObjectName(QStringLiteral("Dynamics26UtilityTabs"));
    tabs->setDocumentMode(false);
    for (int i = 0; i < tabs->count(); ++i) {
        const QString title = tabs->tabText(i);
        if (title.contains(QStringLiteral("Sonuç"), Qt::CaseInsensitive)) {
            tabs->setTabText(i, QStringLiteral("Results"));
        } else if (title.contains(QStringLiteral("Yakınsama"), Qt::CaseInsensitive)) {
            tabs->setTabText(i, QStringLiteral("Convergence"));
        } else if (title.contains(QStringLiteral("Log"), Qt::CaseInsensitive)) {
            tabs->setTabText(i, QStringLiteral("Log"));
        }
    }
}

void replaceLegacyVisibleLogText(QMainWindow &window)
{
    const auto edits = window.findChildren<QPlainTextEdit *>();
    for (auto *edit : edits) {
        if (edit == nullptr || !edit->isReadOnly()) {
            continue;
        }
        QString text = edit->toPlainText();
        if (!text.contains(QStringLiteral("FEMCAE"))) {
            continue;
        }
        text.replace(QStringLiteral("FEMCAE"), QStringLiteral("Dynamics26"));
        edit->setPlainText(text);
    }
}

void removeLegacyToolbars(QMainWindow &window)
{
    const auto toolbars = window.findChildren<QToolBar *>(QString(), Qt::FindDirectChildrenOnly);
    for (auto *toolbar : toolbars) {
        window.removeToolBar(toolbar);
        toolbar->deleteLater();
    }
}

} // namespace

namespace dynamics26::gui {

void applyApplicationShell(QMainWindow &window)
{
    // MainWindow'ın eski light-only QSS'i constructor içinde kuruluyor. Corrective
    // shell bu global override'ı görünür UI'ya taşımadan temizler; böylece
    // Qt/macOS system palette Light/Dark Mode davranışının kaynağı olur.
    window.setStyleSheet(QString());
    window.setWindowTitle(QStringLiteral("Untitled.d26 — Dynamics26"));
    window.setDocumentMode(true);
    window.setDockNestingEnabled(false);
#ifdef Q_OS_MACOS
    // QOpenGLWidget/QVTKOpenGLNativeWidget ile unified title/toolbar yolu Qt'de
    // güvenli değildir; native frame korunur ve unsupported seçenek zorlanmaz.
    window.setUnifiedTitleAndToolBarOnMac(false);
#endif

    WorkspaceParts parts = detachLegacyWorkspace(window);
    if (parts.navigator != nullptr && parts.viewport != nullptr && parts.legacyInspector != nullptr) {
        configureNavigatorWidget(parts.navigator);
        parts.viewport->setObjectName(QStringLiteral("Dynamics26Viewport"));
        parts.viewport->setMinimumWidth(520);
        parts.viewport->setAccessibleName(QStringLiteral("3D Viewport"));

        QWidget *navigatorPanel = wrapNavigator(parts.navigator);
        InspectorShell inspectorShell = wrapInspector(parts.legacyInspector);

        auto *workspace = new QSplitter(Qt::Horizontal, &window);
        workspace->setObjectName(QStringLiteral("Dynamics26WorkspaceSplitter"));
        workspace->setHandleWidth(1);
        workspace->setChildrenCollapsible(false);
        workspace->addWidget(navigatorPanel);
        workspace->addWidget(parts.viewport);
        workspace->addWidget(inspectorShell.panel);
        workspace->setStretchFactor(0, 0);
        workspace->setStretchFactor(1, 1);
        workspace->setStretchFactor(2, 0);
        workspace->setSizes({240, 980, 340});
        window.setCentralWidget(workspace);

        QDockWidget *utilityDock = nullptr;
        const auto docks = window.findChildren<QDockWidget *>(QString(), Qt::FindDirectChildrenOnly);
        if (!docks.isEmpty()) {
            utilityDock = docks.first();
            configureUtilityArea(utilityDock);
        }
        auto *utilityTabs = utilityDock != nullptr ? utilityDock->findChild<QTabWidget *>() : nullptr;

        if (inspectorShell.tabs != nullptr) {
            QObject::connect(
                parts.navigator,
                &QTreeWidget::currentItemChanged,
                &window,
                [inspectorShell, utilityDock, utilityTabs](QTreeWidgetItem *current, QTreeWidgetItem *) {
                    if (current == nullptr) {
                        inspectorShell.context->setText(QStringLiteral("Seçim yok"));
                        inspectorShell.tabs->hide();
                        inspectorShell.emptyState->show();
                        return;
                    }

                    inspectorShell.context->setText(current->text(0));
                    const QVariant binding = current->data(0, kNavigatorRoleInspectorPage);
                    if (!binding.isValid()) {
                        inspectorShell.tabs->hide();
                        inspectorShell.emptyState->show();
                        return;
                    }

                    const int target = binding.toInt();
                    if (target == kNavigatorOpenResults) {
                        inspectorShell.tabs->hide();
                        inspectorShell.emptyState->setText(
                            QStringLiteral("Sonuçlar ve çözüm tanıları alt utility alanında görüntülenir."));
                        inspectorShell.emptyState->show();
                        activateUtilityTab(utilityDock, utilityTabs, QStringLiteral("Results"));
                        return;
                    }

                    if (target >= 0 && target < inspectorShell.tabs->count()) {
                        inspectorShell.tabs->setCurrentIndex(target);
                        inspectorShell.emptyState->hide();
                        inspectorShell.tabs->show();
                    }
                });
        }

        // Shell-owned actions: legacy QAction metinlerini arama/yeniden etiketleme
        // kaldırıldı. Komutlar yalnız MainWindow'da gerçekten var olan slotlara
        // bağlanır; Alpha.3 command registry gelene kadar bu typed-by-contract
        // köprü kullanılır.
        auto *newAction = makeSlotAction(
            window, QStringLiteral("New"), QStringLiteral("New Project"),
            "createNewProject", QStyle::SP_FileIcon);
        newAction->setShortcut(QKeySequence::New);
        QObject::connect(newAction, &QAction::triggered, &window, [navigator = parts.navigator, inspectorShell] {
            configureNavigatorWidget(navigator);
            inspectorShell.context->setText(QStringLiteral("Seçim yok"));
            inspectorShell.tabs->hide();
            inspectorShell.emptyState->setText(
                QStringLiteral("Project Navigator’dan bir öğe seçerek mevcut mühendislik araçlarını görüntüleyin."));
            inspectorShell.emptyState->show();
        });

        auto *openAction = makeSlotAction(
            window, QStringLiteral("Open…"), QStringLiteral("Open Project"),
            "openProject", QStyle::SP_DialogOpenButton);
        openAction->setShortcut(QKeySequence::Open);

        auto *saveAction = makeSlotAction(
            window, QStringLiteral("Save"), QStringLiteral("Save Project"),
            "saveProject", QStyle::SP_DialogSaveButton);
        saveAction->setShortcut(QKeySequence::Save);

        auto *fitAction = makeSlotAction(
            window, QStringLiteral("Fit View"), QStringLiteral("Fit model to viewport"),
            "resetView", QStyle::SP_BrowserReload);

        auto *linearAction = makeSlotAction(
            window, QStringLiteral("Linear Verification"), QStringLiteral("Run linear verification analysis"),
            "runLinearDemo", QStyle::SP_MediaPlay);
        auto *modalAction = makeSlotAction(
            window, QStringLiteral("Modal Verification"), QStringLiteral("Run modal verification analysis"),
            "runModalDemo", QStyle::SP_MediaPlay);
        auto *nonlinearAction = makeSlotAction(
            window, QStringLiteral("Nonlinear Verification"), QStringLiteral("Run nonlinear verification analysis"),
            "runNonlinearDemo", QStyle::SP_MediaPlay);

        auto *navigatorAction = new QAction(QStringLiteral("Project Navigator"), &window);
        navigatorAction->setCheckable(true);
        navigatorAction->setChecked(true);
        navigatorAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1));
        navigatorAction->setToolTip(QStringLiteral("Show or hide Project Navigator (⌘1)"));
        setStandardIcon(window, navigatorAction, QStyle::SP_FileDialogListView);
        QObject::connect(navigatorAction, &QAction::toggled, navigatorPanel, &QWidget::setVisible);

        auto *inspectorAction = new QAction(QStringLiteral("Inspector"), &window);
        inspectorAction->setCheckable(true);
        inspectorAction->setChecked(true);
        inspectorAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_2));
        inspectorAction->setToolTip(QStringLiteral("Show or hide Inspector (⌘2)"));
        setStandardIcon(window, inspectorAction, QStyle::SP_FileDialogDetailedView);
        QObject::connect(inspectorAction, &QAction::toggled, inspectorShell.panel, &QWidget::setVisible);

        auto *utilityAction = new QAction(QStringLiteral("Results & Diagnostics"), &window);
        utilityAction->setCheckable(true);
        utilityAction->setChecked(false);
        utilityAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_J));
        utilityAction->setToolTip(QStringLiteral("Show or hide Results, Convergence and Log (⌘J)"));
        setStandardIcon(window, utilityAction, QStyle::SP_FileDialogContentsView);
        QObject::connect(utilityAction, &QAction::toggled, &window, [utilityDock](bool visible) {
            if (utilityDock != nullptr) {
                utilityDock->setVisible(visible);
            }
        });
        if (utilityDock != nullptr) {
            QObject::connect(utilityDock, &QDockWidget::visibilityChanged, utilityAction, &QAction::setChecked);
        }

        auto *aboutAction = new QAction(QStringLiteral("About Dynamics26"), &window);
        aboutAction->setMenuRole(QAction::AboutRole);
        QObject::connect(aboutAction, &QAction::triggered, &window, [&window] {
            QMessageBox::about(
                &window,
                QStringLiteral("Dynamics26"),
                QStringLiteral("Dynamics26\nGUI milestone %1\nEngine %2.%3.%4\nC API %5\n\nModern macOS-focused FEA/CAE platform.")
                    .arg(QStringLiteral(DYNAMICS26_GUI_MILESTONE))
                    .arg(fem_version_major()).arg(fem_version_minor()).arg(fem_version_patch())
                    .arg(fem_api_version()));
        });

        auto *quitAction = new QAction(QStringLiteral("Quit Dynamics26"), &window);
        quitAction->setMenuRole(QAction::QuitRole);
        quitAction->setShortcut(QKeySequence::Quit);
        QObject::connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

        QMenuBar *bar = window.menuBar();
        bar->clear();
#ifdef Q_OS_MACOS
        bar->setNativeMenuBar(true);
#endif
        auto *fileMenu = bar->addMenu(QStringLiteral("File"));
        fileMenu->addAction(newAction);
        fileMenu->addAction(openAction);
        fileMenu->addAction(saveAction);
        fileMenu->addSeparator();
        fileMenu->addAction(quitAction);

        auto *viewMenu = bar->addMenu(QStringLiteral("View"));
        viewMenu->addAction(navigatorAction);
        viewMenu->addAction(inspectorAction);
        viewMenu->addAction(utilityAction);
        viewMenu->addSeparator();
        viewMenu->addAction(fitAction);

        auto *analysisMenu = bar->addMenu(QStringLiteral("Analysis"));
        analysisMenu->addAction(linearAction);
        analysisMenu->addAction(modalAction);
        analysisMenu->addAction(nonlinearAction);

        auto *helpMenu = bar->addMenu(QStringLiteral("Help"));
        helpMenu->addAction(aboutAction);

        removeLegacyToolbars(window);
        auto *mainToolbar = new QToolBar(QStringLiteral("Main Toolbar"), &window);
        mainToolbar->setObjectName(QStringLiteral("Dynamics26MainToolbar"));
        mainToolbar->setMovable(false);
        mainToolbar->setFloatable(false);
        mainToolbar->setAllowedAreas(Qt::TopToolBarArea);
        mainToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        mainToolbar->setIconSize(QSize(18, 18));
        window.addToolBar(Qt::TopToolBarArea, mainToolbar);

        mainToolbar->addAction(navigatorAction);
        auto *documentTitle = new QLabel(QStringLiteral("Untitled.d26"), mainToolbar);
        documentTitle->setObjectName(QStringLiteral("Dynamics26DocumentTitle"));
        documentTitle->setAccessibleName(QStringLiteral("Current project"));
        mainToolbar->addWidget(documentTitle);
        mainToolbar->addWidget(makeExpandingSpacer(mainToolbar));
        mainToolbar->addAction(fitAction);
        mainToolbar->addAction(linearAction);
        mainToolbar->addAction(utilityAction);
        mainToolbar->addAction(inspectorAction);

        if (utilityDock != nullptr && utilityTabs != nullptr) {
            QObject::connect(linearAction, &QAction::triggered, &window, [utilityDock, utilityTabs] {
                activateUtilityTab(utilityDock, utilityTabs, QStringLiteral("Results"));
            });
            QObject::connect(modalAction, &QAction::triggered, &window, [utilityDock, utilityTabs] {
                activateUtilityTab(utilityDock, utilityTabs, QStringLiteral("Results"));
            });
            QObject::connect(nonlinearAction, &QAction::triggered, &window, [utilityDock, utilityTabs] {
                activateUtilityTab(utilityDock, utilityTabs, QStringLiteral("Convergence"));
            });
            utilityDock->hide();
        }
    }

    replaceLegacyVisibleLogText(window);

    // Engine/API sürümleri About/Diagnostics içeriğidir; kalıcı status bar
    // viewport alanını tüketmez.
    window.statusBar()->clearMessage();
    window.statusBar()->hide();
}

} // namespace dynamics26::gui
