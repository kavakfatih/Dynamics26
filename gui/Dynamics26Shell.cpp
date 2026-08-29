#include "Dynamics26Shell.h"

#include <femcae/femcae.h>

#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QFrame>
#include <QKeySequence>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSize>
#include <QSizePolicy>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QTabWidget>
#include <QToolBar>
#include <QTreeWidget>
#include <QWidget>

#ifndef DYNAMICS26_GUI_MILESTONE
#define DYNAMICS26_GUI_MILESTONE "1.1.0-alpha.1"
#endif

namespace {

QString normalizedActionText(QString text)
{
    text.remove('&');
    return text.trimmed();
}

QAction *findAction(QMainWindow &window, const QStringList &candidates)
{
    const auto actions = window.findChildren<QAction *>();
    for (auto *action : actions) {
        if (action == nullptr) {
            continue;
        }
        const QString current = normalizedActionText(action->text());
        for (const auto &candidate : candidates) {
            if (current.compare(candidate, Qt::CaseInsensitive) == 0) {
                return action;
            }
        }
    }
    return nullptr;
}

void addActionIfPresent(QMenu *menu, QAction *action)
{
    if (menu != nullptr && action != nullptr) {
        menu->addAction(action);
    }
}

QWidget *makeExpandingSpacer(QWidget *parent)
{
    auto *spacer = new QWidget(parent);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    return spacer;
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
    dock->setWindowTitle(QStringLiteral("Analysis Utility"));
    dock->setAllowedAreas(Qt::BottomDockWidgetArea);
    dock->setFeatures(QDockWidget::DockWidgetClosable);
    dock->setMinimumHeight(150);

    auto *tabs = dock->findChild<QTabWidget *>();
    if (tabs == nullptr) {
        return;
    }

    tabs->setObjectName(QStringLiteral("Dynamics26UtilityTabs"));
    tabs->setDocumentMode(true);

    // Corrective Alpha.1: yalnız çalışan veri yüzeyleri görünür kalır.
    // Solver/Graph/Messages gibi gelecek sürüm placeholder'ları ana UI'ya eklenmez.
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

void setStandardIcon(QMainWindow &window, QAction *action, QStyle::StandardPixmap pixmap)
{
    if (action != nullptr && window.style() != nullptr) {
        action->setIcon(window.style()->standardIcon(pixmap));
    }
}

} // namespace

namespace dynamics26::gui {

void applyApplicationShell(QMainWindow &window)
{
    // İlk Alpha.1'de unifiedTitleAndToolBarOnMac(true) kullanılmıştı. Qt'nin
    // kendi dokümantasyonuna göre bu yol QOpenGLWidget içeren pencerelerde
    // desteklenmez. Dynamics26 VTK viewport'u OpenGL widget kullandığından
    // gerçek macOS testinde görülen blank/siyah panel davranışının önüne geçmek
    // için corrective Alpha.1 normal native macOS frame kullanır.
    window.setStyleSheet(QString());
    window.setWindowTitle(QStringLiteral("Untitled.d26"));
    window.setDocumentMode(true);
    window.setDockNestingEnabled(false);
#ifdef Q_OS_MACOS
    window.setUnifiedTitleAndToolBarOnMac(false);
#endif

    auto *mainSplitter = qobject_cast<QSplitter *>(window.centralWidget());
    QTreeWidget *navigator = nullptr;
    QWidget *viewport = nullptr;
    QWidget *inspector = nullptr;

    if (mainSplitter != nullptr && mainSplitter->count() >= 3) {
        mainSplitter->setObjectName(QStringLiteral("Dynamics26WorkspaceSplitter"));
        mainSplitter->setHandleWidth(1);
        mainSplitter->setChildrenCollapsible(false);

        navigator = qobject_cast<QTreeWidget *>(mainSplitter->widget(0));
        viewport = mainSplitter->widget(1);
        inspector = mainSplitter->widget(2);

        if (navigator != nullptr) {
            navigator->setObjectName(QStringLiteral("Dynamics26Navigator"));
            navigator->setMinimumWidth(210);
            navigator->setMaximumWidth(420);
            navigator->setHeaderHidden(true);
            navigator->setRootIsDecorated(true);
            navigator->setUniformRowHeights(true);
            navigator->setIndentation(14);
            navigator->setFrameShape(QFrame::NoFrame);
            navigator->setAccessibleName(QStringLiteral("Project Navigator"));
        }

        if (viewport != nullptr) {
            viewport->setObjectName(QStringLiteral("Dynamics26Viewport"));
            viewport->setMinimumWidth(480);
            viewport->setAccessibleName(QStringLiteral("3D Viewport"));
        }

        if (inspector != nullptr) {
            inspector->setObjectName(QStringLiteral("Dynamics26Inspector"));
            inspector->setMinimumWidth(300);
            inspector->setMaximumWidth(520);
            inspector->setAccessibleName(QStringLiteral("Inspector"));
            if (auto *tabs = qobject_cast<QTabWidget *>(inspector)) {
                tabs->setDocumentMode(true);
                tabs->setUsesScrollButtons(true);
            }
        }

        mainSplitter->setStretchFactor(0, 0);
        mainSplitter->setStretchFactor(1, 1);
        mainSplitter->setStretchFactor(2, 0);
        mainSplitter->setSizes({250, 930, 340});
    }

    QDockWidget *utilityDock = nullptr;
    const auto docks = window.findChildren<QDockWidget *>(QString(), Qt::FindDirectChildrenOnly);
    if (!docks.isEmpty()) {
        utilityDock = docks.first();
        configureUtilityArea(utilityDock);
    }
    auto *utilityTabs = utilityDock != nullptr ? utilityDock->findChild<QTabWidget *>() : nullptr;

    // Legacy MainWindow'da gerçekten çalışan action'lar korunur. Görünür metin
    // tabanlı bu adaptör geçicidir; alpha.3 gerçek command registry ile değişir.
    QAction *newAction = findAction(window, {QStringLiteral("Yeni"), QStringLiteral("New")});
    QAction *openAction = findAction(window, {QStringLiteral("Aç"), QStringLiteral("Open"), QStringLiteral("Open…")});
    QAction *saveAction = findAction(window, {QStringLiteral("Kaydet"), QStringLiteral("Save")});
    QAction *solveAction = findAction(window, {QStringLiteral("Lineer Analiz"), QStringLiteral("Solve")});
    QAction *modalAction = findAction(window, {QStringLiteral("Modal Analiz"), QStringLiteral("Modal Demo")});
    QAction *nonlinearAction = findAction(window, {QStringLiteral("Nonlinear Static"), QStringLiteral("Nonlinear Demo")});
    QAction *fitAction = findAction(window, {QStringLiteral("Görünümü Sığdır"), QStringLiteral("Fit View")});

    if (newAction != nullptr) {
        newAction->setText(QStringLiteral("New"));
        newAction->setShortcut(QKeySequence::New);
        newAction->setToolTip(QStringLiteral("New Project"));
        setStandardIcon(window, newAction, QStyle::SP_FileIcon);
    }
    if (openAction != nullptr) {
        openAction->setText(QStringLiteral("Open…"));
        openAction->setShortcut(QKeySequence::Open);
        openAction->setToolTip(QStringLiteral("Open Project"));
        setStandardIcon(window, openAction, QStyle::SP_DialogOpenButton);
    }
    if (saveAction != nullptr) {
        saveAction->setText(QStringLiteral("Save"));
        saveAction->setShortcut(QKeySequence::Save);
        saveAction->setToolTip(QStringLiteral("Save Project"));
        setStandardIcon(window, saveAction, QStyle::SP_DialogSaveButton);
    }
    if (solveAction != nullptr) {
        solveAction->setText(QStringLiteral("Solve"));
        solveAction->setToolTip(QStringLiteral("Run the current linear verification analysis"));
        setStandardIcon(window, solveAction, QStyle::SP_MediaPlay);
    }
    if (modalAction != nullptr) {
        modalAction->setText(QStringLiteral("Modal Verification"));
        modalAction->setToolTip(QStringLiteral("Run modal verification analysis"));
        setStandardIcon(window, modalAction, QStyle::SP_MediaPlay);
    }
    if (nonlinearAction != nullptr) {
        nonlinearAction->setText(QStringLiteral("Nonlinear Verification"));
        nonlinearAction->setToolTip(QStringLiteral("Run nonlinear verification analysis"));
        setStandardIcon(window, nonlinearAction, QStyle::SP_MediaPlay);
    }
    if (fitAction != nullptr) {
        fitAction->setText(QStringLiteral("Fit View"));
        fitAction->setToolTip(QStringLiteral("Fit model to viewport"));
        setStandardIcon(window, fitAction, QStyle::SP_BrowserReload);
    }

    auto *navigatorAction = new QAction(QStringLiteral("Project Navigator"), &window);
    navigatorAction->setCheckable(true);
    navigatorAction->setChecked(navigator == nullptr || navigator->isVisible());
    navigatorAction->setShortcut(QKeySequence(Qt::META | Qt::Key_1));
    navigatorAction->setToolTip(QStringLiteral("Show or hide Project Navigator (⌘1)"));
    setStandardIcon(window, navigatorAction, QStyle::SP_FileDialogListView);
    QObject::connect(navigatorAction, &QAction::toggled, &window, [navigator](bool visible) {
        if (navigator != nullptr) {
            navigator->setVisible(visible);
        }
    });

    auto *inspectorAction = new QAction(QStringLiteral("Inspector"), &window);
    inspectorAction->setCheckable(true);
    inspectorAction->setChecked(inspector == nullptr || inspector->isVisible());
    inspectorAction->setShortcut(QKeySequence(Qt::META | Qt::Key_2));
    inspectorAction->setToolTip(QStringLiteral("Show or hide Inspector (⌘2)"));
    setStandardIcon(window, inspectorAction, QStyle::SP_FileDialogDetailedView);
    QObject::connect(inspectorAction, &QAction::toggled, &window, [inspector](bool visible) {
        if (inspector != nullptr) {
            inspector->setVisible(visible);
        }
    });

    auto *utilityAction = new QAction(QStringLiteral("Results & Log"), &window);
    utilityAction->setCheckable(true);
    utilityAction->setChecked(false);
    utilityAction->setShortcut(QKeySequence(Qt::META | Qt::Key_J));
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

    // Menu bar yalnız çalışan komutları içerir. Model/Geometry/Mesh/Search/Undo
    // placeholder'ları gerçek command modeli gelene kadar ana UI'da gösterilmez.
    QMenuBar *bar = window.menuBar();
    bar->clear();

    auto *fileMenu = bar->addMenu(QStringLiteral("File"));
    addActionIfPresent(fileMenu, newAction);
    addActionIfPresent(fileMenu, openAction);
    addActionIfPresent(fileMenu, saveAction);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);

    auto *viewMenu = bar->addMenu(QStringLiteral("View"));
    viewMenu->addAction(navigatorAction);
    viewMenu->addAction(inspectorAction);
    viewMenu->addAction(utilityAction);
    viewMenu->addSeparator();
    addActionIfPresent(viewMenu, fitAction);

    auto *analysisMenu = bar->addMenu(QStringLiteral("Analysis"));
    addActionIfPresent(analysisMenu, solveAction);
    addActionIfPresent(analysisMenu, modalAction);
    addActionIfPresent(analysisMenu, nonlinearAction);

    auto *helpMenu = bar->addMenu(QStringLiteral("Help"));
    helpMenu->addAction(aboutAction);

    QToolBar *mainToolbar = nullptr;
    const auto directToolbars = window.findChildren<QToolBar *>(QString(), Qt::FindDirectChildrenOnly);
    if (!directToolbars.isEmpty()) {
        mainToolbar = directToolbars.first();
    }
    if (mainToolbar == nullptr) {
        mainToolbar = new QToolBar(QStringLiteral("Main Toolbar"), &window);
        window.addToolBar(Qt::TopToolBarArea, mainToolbar);
    }

    // Apple HIG'e paralel corrective yaklaşım: tek satır, az sayıda komut,
    // monochrome/system icon, içerik önceliği. Eski workspace text-strip ve
    // command-hint satırı tamamen kaldırılmıştır.
    mainToolbar->setObjectName(QStringLiteral("Dynamics26MainToolbar"));
    mainToolbar->setWindowTitle(QStringLiteral("Main Toolbar"));
    mainToolbar->setMovable(false);
    mainToolbar->setFloatable(false);
    mainToolbar->setAllowedAreas(Qt::TopToolBarArea);
    mainToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    mainToolbar->setIconSize(QSize(18, 18));
    mainToolbar->clear();

    mainToolbar->addAction(navigatorAction);

    auto *documentTitle = new QLabel(QStringLiteral("Untitled.d26"), mainToolbar);
    documentTitle->setObjectName(QStringLiteral("Dynamics26DocumentTitle"));
    documentTitle->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    documentTitle->setMinimumWidth(120);
    documentTitle->setAccessibleName(QStringLiteral("Current project"));
    mainToolbar->addWidget(documentTitle);

    mainToolbar->addWidget(makeExpandingSpacer(mainToolbar));

    if (fitAction != nullptr) {
        mainToolbar->addAction(fitAction);
    }
    if (solveAction != nullptr) {
        mainToolbar->addAction(solveAction);
    }
    mainToolbar->addAction(utilityAction);
    mainToolbar->addAction(inspectorAction);

    if (solveAction != nullptr && utilityDock != nullptr && utilityTabs != nullptr) {
        QObject::connect(solveAction, &QAction::triggered, &window, [utilityDock, utilityTabs] {
            activateUtilityTab(utilityDock, utilityTabs, QStringLiteral("Results"));
        });
    }
    if (modalAction != nullptr && utilityDock != nullptr && utilityTabs != nullptr) {
        QObject::connect(modalAction, &QAction::triggered, &window, [utilityDock, utilityTabs] {
            activateUtilityTab(utilityDock, utilityTabs, QStringLiteral("Results"));
        });
    }
    if (nonlinearAction != nullptr && utilityDock != nullptr && utilityTabs != nullptr) {
        QObject::connect(nonlinearAction, &QAction::triggered, &window, [utilityDock, utilityTabs] {
            activateUtilityTab(utilityDock, utilityTabs, QStringLiteral("Convergence"));
        });
    }

    if (utilityDock != nullptr) {
        utilityDock->hide();
    }

    replaceLegacyVisibleLogText(window);

    // Engine/API sürümleri About/Diagnostics içeriğidir; kalıcı alt satır olarak
    // viewport alanını tüketmez.
    window.statusBar()->clearMessage();
    window.statusBar()->hide();
}

} // namespace dynamics26::gui
