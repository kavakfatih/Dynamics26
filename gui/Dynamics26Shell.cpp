#include "Dynamics26Shell.h"

#include <femcae/femcae.h>

#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QDockWidget>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSize>
#include <QSizePolicy>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
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

QLabel *makeUtilityPlaceholder(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    return label;
}

int tabIndexByText(QTabWidget *tabs, const QString &text)
{
    if (tabs == nullptr) {
        return -1;
    }
    for (int i = 0; i < tabs->count(); ++i) {
        if (tabs->tabText(i).compare(text, Qt::CaseInsensitive) == 0) {
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
    const int index = tabIndexByText(tabs, text);
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
    dock->setWindowTitle(QStringLiteral("Utility"));
    dock->setAllowedAreas(Qt::BottomDockWidgetArea);
    dock->setFeatures(QDockWidget::DockWidgetClosable);
    dock->setMinimumHeight(150);

    auto *tabs = dock->findChild<QTabWidget *>();
    if (tabs == nullptr) {
        return;
    }
    tabs->setObjectName(QStringLiteral("Dynamics26UtilityTabs"));

    QWidget *tablePage = nullptr;
    QWidget *convergencePage = nullptr;
    QWidget *logPage = nullptr;

    for (int i = 0; i < tabs->count(); ++i) {
        const QString title = tabs->tabText(i);
        QWidget *page = tabs->widget(i);
        if (title.contains(QStringLiteral("Sonuç"), Qt::CaseInsensitive) ||
            title.compare(QStringLiteral("Table"), Qt::CaseInsensitive) == 0) {
            tablePage = page;
        } else if (title.contains(QStringLiteral("Yakınsama"), Qt::CaseInsensitive) ||
                   title.contains(QStringLiteral("Convergence"), Qt::CaseInsensitive)) {
            convergencePage = page;
        } else if (title.compare(QStringLiteral("Log"), Qt::CaseInsensitive) == 0) {
            logPage = page;
        }
    }

    while (tabs->count() > 0) {
        tabs->removeTab(0);
    }

    tabs->addTab(makeUtilityPlaceholder(
        QStringLiteral("Solver monitor V1.1.0-beta.2 paketinde gerçek solver-state modeliyle bağlanacak."), tabs),
        QStringLiteral("Solver"));

    if (convergencePage == nullptr) {
        convergencePage = makeUtilityPlaceholder(QStringLiteral("Convergence data"), tabs);
    }
    tabs->addTab(convergencePage, QStringLiteral("Convergence"));

    tabs->addTab(makeUtilityPlaceholder(
        QStringLiteral("Graph workspace V1.1.0-beta.2 / beta.3 paketlerinde etkinleşecek."), tabs),
        QStringLiteral("Graph"));

    if (tablePage == nullptr) {
        tablePage = makeUtilityPlaceholder(QStringLiteral("Result tables"), tabs);
    }
    tabs->addTab(tablePage, QStringLiteral("Table"));

    tabs->addTab(makeUtilityPlaceholder(
        QStringLiteral("Model validation, warning ve error mesajları burada toplanacak."), tabs),
        QStringLiteral("Messages"));

    if (logPage == nullptr) {
        logPage = makeUtilityPlaceholder(QStringLiteral("Application log"), tabs);
    }
    tabs->addTab(logPage, QStringLiteral("Log"));
    tabs->setCurrentIndex(0);
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

} // namespace

namespace dynamics26::gui {

void applyApplicationShell(QMainWindow &window)
{
    // V1.0 GUI'si yalnız Light Mode'a göre hard-code edilmiş bir global QSS
    // kullanıyordu. Alpha.1'de bu QSS temizlenerek Qt'nin native macOS style,
    // semantic palette ve Light/Dark Mode davranışına geri dönülür.
    window.setStyleSheet(QString());
    window.setWindowTitle(QStringLiteral("Untitled.d26"));
    window.setDocumentMode(true);
    window.setDockNestingEnabled(false);
#ifdef Q_OS_MACOS
    window.setUnifiedTitleAndToolBarOnMac(true);
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
            navigator->setMinimumWidth(190);
            navigator->setUniformRowHeights(true);
        }
        if (viewport != nullptr) {
            viewport->setObjectName(QStringLiteral("Dynamics26Viewport"));
            viewport->setMinimumWidth(420);
        }
        if (inspector != nullptr) {
            inspector->setObjectName(QStringLiteral("Dynamics26Inspector"));
            inspector->setMinimumWidth(260);
        }
        mainSplitter->setStretchFactor(0, 0);
        mainSplitter->setStretchFactor(1, 1);
        mainSplitter->setStretchFactor(2, 0);
        mainSplitter->setSizes({240, 900, 320});
    }

    QDockWidget *utilityDock = nullptr;
    const auto docks = window.findChildren<QDockWidget *>(QString(), Qt::FindDirectChildrenOnly);
    if (!docks.isEmpty()) {
        utilityDock = docks.first();
        configureUtilityArea(utilityDock);
    }
    auto *utilityTabs = utilityDock != nullptr ? utilityDock->findChild<QTabWidget *>() : nullptr;

    QAction *newAction = findAction(window, {QStringLiteral("Yeni"), QStringLiteral("New")});
    QAction *openAction = findAction(window, {QStringLiteral("Aç"), QStringLiteral("Open")});
    QAction *saveAction = findAction(window, {QStringLiteral("Kaydet"), QStringLiteral("Save")});
    QAction *solveAction = findAction(window, {QStringLiteral("Lineer Analiz"), QStringLiteral("Solve")});
    QAction *modalAction = findAction(window, {QStringLiteral("Modal Analiz"), QStringLiteral("Modal")});
    QAction *nonlinearAction = findAction(window, {QStringLiteral("Nonlinear Static"), QStringLiteral("Nonlinear")});
    QAction *fitAction = findAction(window, {QStringLiteral("Görünümü Sığdır"), QStringLiteral("Fit View")});

    if (newAction != nullptr) {
        newAction->setText(QStringLiteral("New"));
        newAction->setShortcut(QKeySequence::New);
    }
    if (openAction != nullptr) {
        openAction->setText(QStringLiteral("Open…"));
        openAction->setShortcut(QKeySequence::Open);
    }
    if (saveAction != nullptr) {
        saveAction->setText(QStringLiteral("Save"));
        saveAction->setShortcut(QKeySequence::Save);
    }
    if (solveAction != nullptr) {
        solveAction->setText(QStringLiteral("Solve"));
        solveAction->setToolTip(QStringLiteral("Current V1.0 linear demo solve path; unified analysis command arrives in V1.1.0-alpha.3."));
    }
    if (modalAction != nullptr) {
        modalAction->setText(QStringLiteral("Modal Demo"));
    }
    if (nonlinearAction != nullptr) {
        nonlinearAction->setText(QStringLiteral("Nonlinear Demo"));
    }
    if (fitAction != nullptr) {
        fitAction->setText(QStringLiteral("Fit View"));
    }

    auto *navigatorAction = new QAction(QStringLiteral("Navigator"), &window);
    navigatorAction->setCheckable(true);
    navigatorAction->setChecked(navigator == nullptr || navigator->isVisible());
    navigatorAction->setShortcut(QKeySequence(Qt::META | Qt::Key_1));
    QObject::connect(navigatorAction, &QAction::toggled, &window, [navigator](bool visible) {
        if (navigator != nullptr) {
            navigator->setVisible(visible);
        }
    });

    auto *inspectorAction = new QAction(QStringLiteral("Inspector"), &window);
    inspectorAction->setCheckable(true);
    inspectorAction->setChecked(inspector == nullptr || inspector->isVisible());
    inspectorAction->setShortcut(QKeySequence(Qt::META | Qt::Key_2));
    QObject::connect(inspectorAction, &QAction::toggled, &window, [inspector](bool visible) {
        if (inspector != nullptr) {
            inspector->setVisible(visible);
        }
    });

    auto *utilityAction = new QAction(QStringLiteral("Bottom Utility Area"), &window);
    utilityAction->setCheckable(true);
    utilityAction->setChecked(false);
    utilityAction->setShortcut(QKeySequence(Qt::META | Qt::Key_J));
    QObject::connect(utilityAction, &QAction::toggled, &window, [utilityDock](bool visible) {
        if (utilityDock != nullptr) {
            utilityDock->setVisible(visible);
        }
    });
    if (utilityDock != nullptr) {
        QObject::connect(utilityDock, &QDockWidget::visibilityChanged, utilityAction, &QAction::setChecked);
    }

    auto *undoAction = new QAction(QStringLiteral("Undo"), &window);
    undoAction->setShortcut(QKeySequence::Undo);
    undoAction->setEnabled(false);
    undoAction->setToolTip(QStringLiteral("Unified command/undo model is scheduled for V1.1.0-alpha.3."));

    auto *redoAction = new QAction(QStringLiteral("Redo"), &window);
    redoAction->setShortcut(QKeySequence::Redo);
    redoAction->setEnabled(false);
    redoAction->setToolTip(QStringLiteral("Unified command/undo model is scheduled for V1.1.0-alpha.3."));

    auto *settingsAction = new QAction(QStringLiteral("Settings…"), &window);
    settingsAction->setMenuRole(QAction::PreferencesRole);
    settingsAction->setShortcut(QKeySequence::Preferences);
    QObject::connect(settingsAction, &QAction::triggered, &window, [&window] {
        window.statusBar()->showMessage(QStringLiteral("Dynamics26 Settings workspace is planned for a later V1.1 package."), 5000);
    });

    auto *aboutAction = new QAction(QStringLiteral("About Dynamics26"), &window);
    aboutAction->setMenuRole(QAction::AboutRole);
    QObject::connect(aboutAction, &QAction::triggered, &window, [&window] {
        QMessageBox::about(&window, QStringLiteral("Dynamics26"),
            QStringLiteral("Dynamics26\nGUI milestone %1\nEngine %2.%3.%4\n\nModern macOS-focused FEA/CAE platform.")
                .arg(QStringLiteral(DYNAMICS26_GUI_MILESTONE))
                .arg(fem_version_major()).arg(fem_version_minor()).arg(fem_version_patch()));
    });

    auto *quitAction = new QAction(QStringLiteral("Quit Dynamics26"), &window);
    quitAction->setMenuRole(QAction::QuitRole);
    quitAction->setShortcut(QKeySequence::Quit);
    QObject::connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    auto *helpAction = new QAction(QStringLiteral("Dynamics26 Help"), &window);
    QObject::connect(helpAction, &QAction::triggered, &window, [&window] {
        window.statusBar()->showMessage(QStringLiteral("Contextual engineering help is scheduled for the V1.1 Inspector packages."), 5000);
    });

    QMenuBar *bar = window.menuBar();
    bar->clear();

    auto *fileMenu = bar->addMenu(QStringLiteral("File"));
    addActionIfPresent(fileMenu, newAction);
    addActionIfPresent(fileMenu, openAction);
    addActionIfPresent(fileMenu, saveAction);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);

    auto *editMenu = bar->addMenu(QStringLiteral("Edit"));
    editMenu->addAction(undoAction);
    editMenu->addAction(redoAction);
    editMenu->addSeparator();
    editMenu->addAction(settingsAction);

    auto *viewMenu = bar->addMenu(QStringLiteral("View"));
    viewMenu->addAction(navigatorAction);
    viewMenu->addAction(inspectorAction);
    viewMenu->addAction(utilityAction);
    viewMenu->addSeparator();
    addActionIfPresent(viewMenu, fitAction);

    auto addPlannedMenu = [bar](const QString &title, const QString &message) {
        auto *menu = bar->addMenu(title);
        auto *planned = menu->addAction(message);
        planned->setEnabled(false);
        return menu;
    };

    addPlannedMenu(QStringLiteral("Model"), QStringLiteral("Model commands — V1.1.0-alpha.2/3"));
    addPlannedMenu(QStringLiteral("Geometry"), QStringLiteral("Geometry commands — V1.1.0-alpha.3"));
    addPlannedMenu(QStringLiteral("Mesh"), QStringLiteral("Mesh commands — V1.1.0-alpha.3"));

    auto *analysisMenu = bar->addMenu(QStringLiteral("Analysis"));
    addActionIfPresent(analysisMenu, solveAction);
    addActionIfPresent(analysisMenu, modalAction);
    addActionIfPresent(analysisMenu, nonlinearAction);

    addPlannedMenu(QStringLiteral("Window"), QStringLiteral("Workspace window controls — V1.1.0-rc.1"));
    auto *helpMenu = bar->addMenu(QStringLiteral("Help"));
    helpMenu->addAction(helpAction);
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

    mainToolbar->setObjectName(QStringLiteral("Dynamics26MainToolbar"));
    mainToolbar->setWindowTitle(QStringLiteral("Main Toolbar"));
    mainToolbar->setMovable(false);
    mainToolbar->setFloatable(false);
    mainToolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mainToolbar->setIconSize(QSize(16, 16));
    mainToolbar->clear();
    mainToolbar->addAction(navigatorAction);
    mainToolbar->addWidget(makeExpandingSpacer(mainToolbar));

    auto *documentTitle = new QLabel(QStringLiteral("Untitled.d26"), mainToolbar);
    documentTitle->setObjectName(QStringLiteral("Dynamics26DocumentTitle"));
    documentTitle->setAlignment(Qt::AlignCenter);
    documentTitle->setToolTip(QStringLiteral("Document/project title. Project menu actions are added in a later V1.1 package."));
    mainToolbar->addWidget(documentTitle);
    mainToolbar->addWidget(makeExpandingSpacer(mainToolbar));

    auto *search = new QLineEdit(mainToolbar);
    search->setObjectName(QStringLiteral("Dynamics26GlobalSearch"));
    search->setPlaceholderText(QStringLiteral("Search"));
    search->setClearButtonEnabled(true);
    search->setMaximumWidth(220);
    search->setToolTip(QStringLiteral("Command/model/documentation search becomes active in V1.1.0-alpha.3."));
    QObject::connect(search, &QLineEdit::returnPressed, &window, [&window] {
        window.statusBar()->showMessage(QStringLiteral("Global Search is a V1.1.0-alpha.3 feature."), 4000);
    });
    mainToolbar->addWidget(search);
    if (solveAction != nullptr) {
        mainToolbar->addAction(solveAction);
    }
    mainToolbar->addAction(aboutAction);

    auto *contextBar = new QToolBar(QStringLiteral("Context Bar"), &window);
    contextBar->setObjectName(QStringLiteral("Dynamics26ContextBar"));
    contextBar->setMovable(false);
    contextBar->setFloatable(false);
    contextBar->setAllowedAreas(Qt::TopToolBarArea);

    auto *contextContainer = new QWidget(contextBar);
    auto *contextLayout = new QVBoxLayout(contextContainer);
    contextLayout->setContentsMargins(6, 2, 6, 4);
    contextLayout->setSpacing(2);

    auto *tabsRow = new QHBoxLayout;
    tabsRow->setContentsMargins(0, 0, 0, 0);
    tabsRow->setSpacing(2);
    auto *buttonGroup = new QButtonGroup(contextContainer);
    buttonGroup->setExclusive(true);

    auto *commandsLabel = new QLabel(QStringLiteral("Select  ·  Measure  ·  View  ·  Validate Model"), contextContainer);
    commandsLabel->setObjectName(QStringLiteral("Dynamics26ContextCommandHint"));

    const QStringList areas = {
        QStringLiteral("Home"), QStringLiteral("Geometry"), QStringLiteral("Materials"),
        QStringLiteral("Connections"), QStringLiteral("Mesh"), QStringLiteral("Physics"),
        QStringLiteral("Analysis"), QStringLiteral("Results")
    };
    const QStringList commandHints = {
        QStringLiteral("Select  ·  Measure  ·  View  ·  Validate Model"),
        QStringLiteral("Import  ·  Create  ·  Modify  ·  Repair  ·  Inspect  ·  Reference"),
        QStringLiteral("Create  ·  Assign  ·  Model  ·  Experimental Data  ·  Calibration"),
        QStringLiteral("Create  ·  Contact  ·  Detection  ·  Inspect"),
        QStringLiteral("Generate  ·  Dimension  ·  Method  ·  Controls  ·  Quality"),
        QStringLiteral("Structural  ·  Loads  ·  Constraints  ·  Initial State  ·  Formulation"),
        QStringLiteral("Study  ·  Steps  ·  Nonlinear  ·  Solver  ·  Restart"),
        QStringLiteral("Deformation  ·  Stress  ·  Strain  ·  Contact  ·  Evaluate  ·  Explore")
    };

    for (int i = 0; i < areas.size(); ++i) {
        auto *button = new QToolButton(contextContainer);
        button->setText(areas[i]);
        button->setCheckable(true);
        button->setAutoRaise(true);
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        buttonGroup->addButton(button, i);
        tabsRow->addWidget(button);
        QObject::connect(button, &QToolButton::clicked, contextContainer, [commandsLabel, commandHints, i] {
            commandsLabel->setText(commandHints[i]);
        });
        if (i == 0) {
            button->setChecked(true);
        }
    }
    tabsRow->addStretch(1);
    contextLayout->addLayout(tabsRow);
    contextLayout->addWidget(commandsLabel);
    contextContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    contextBar->addWidget(contextContainer);

    window.addToolBarBreak(Qt::TopToolBarArea);
    window.addToolBar(Qt::TopToolBarArea, contextBar);

    if (solveAction != nullptr && utilityDock != nullptr && utilityTabs != nullptr) {
        QObject::connect(solveAction, &QAction::triggered, &window, [utilityDock, utilityTabs] {
            activateUtilityTab(utilityDock, utilityTabs, QStringLiteral("Table"));
        });
    }
    if (modalAction != nullptr && utilityDock != nullptr && utilityTabs != nullptr) {
        QObject::connect(modalAction, &QAction::triggered, &window, [utilityDock, utilityTabs] {
            activateUtilityTab(utilityDock, utilityTabs, QStringLiteral("Table"));
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
    window.statusBar()->showMessage(
        QStringLiteral("Dynamics26 GUI %1  •  Engine %2.%3.%4  •  C API %5")
            .arg(QStringLiteral(DYNAMICS26_GUI_MILESTONE))
            .arg(fem_version_major()).arg(fem_version_minor()).arg(fem_version_patch())
            .arg(fem_api_version()));
}

} // namespace dynamics26::gui
