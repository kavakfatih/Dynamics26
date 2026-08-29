#include "CaeWorkbenchController.h"

#include "GeometryPanel.h"
#include "PrePostPanel.h"

#include <QAbstractItemView>
#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QMainWindow>
#include <QMetaObject>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSize>
#include <QSizePolicy>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

namespace {

constexpr int kInspectorRole = Qt::UserRole + 1; // Dynamics26Shell ile ortak sözleşme.
constexpr int kContextRole = Qt::UserRole + 2;
constexpr int kOpenResults = -100;

enum class WorkbenchContext {
    Model = 0,
    Geometry,
    Material,
    Section,
    Connections,
    Mesh,
    Loads,
    Analysis,
    Results
};

QAction *actionContaining(QObject &root, const QString &needle)
{
    const auto actions = root.findChildren<QAction *>();
    for (auto *action : actions) {
        if (action != nullptr && action->text().contains(needle, Qt::CaseInsensitive)) {
            return action;
        }
    }
    return nullptr;
}

QTreeWidgetItem *makeItem(
    QTreeWidgetItem *parent,
    const QString &text,
    int inspectorPage,
    WorkbenchContext context,
    QStyle *style,
    QStyle::StandardPixmap pixmap)
{
    auto *item = new QTreeWidgetItem(parent, {text});
    if (inspectorPage != -999) {
        item->setData(0, kInspectorRole, inspectorPage);
    }
    item->setData(0, kContextRole, static_cast<int>(context));
    if (style != nullptr) {
        item->setIcon(0, style->standardIcon(pixmap));
    }
    return item;
}

QTreeWidgetItem *makeTopItem(
    QTreeWidget *tree,
    const QString &text,
    int inspectorPage,
    WorkbenchContext context,
    QStyle *style,
    QStyle::StandardPixmap pixmap)
{
    auto *item = new QTreeWidgetItem(tree, {text});
    if (inspectorPage != -999) {
        item->setData(0, kInspectorRole, inspectorPage);
    }
    item->setData(0, kContextRole, static_cast<int>(context));
    if (style != nullptr) {
        item->setIcon(0, style->standardIcon(pixmap));
    }
    return item;
}

WorkbenchContext contextFor(QTreeWidgetItem *item)
{
    if (item == nullptr) {
        return WorkbenchContext::Model;
    }
    const QVariant value = item->data(0, kContextRole);
    if (!value.isValid()) {
        return WorkbenchContext::Model;
    }
    return static_cast<WorkbenchContext>(value.toInt());
}

QString contextTitle(WorkbenchContext context)
{
    switch (context) {
    case WorkbenchContext::Geometry: return QStringLiteral("Geometry");
    case WorkbenchContext::Material: return QStringLiteral("Materials");
    case WorkbenchContext::Section: return QStringLiteral("Sections");
    case WorkbenchContext::Connections: return QStringLiteral("Connections");
    case WorkbenchContext::Mesh: return QStringLiteral("Mesh");
    case WorkbenchContext::Loads: return QStringLiteral("Loads / BC");
    case WorkbenchContext::Analysis: return QStringLiteral("Static Structural");
    case WorkbenchContext::Results: return QStringLiteral("Results");
    case WorkbenchContext::Model:
    default: return QStringLiteral("Model");
    }
}

QTreeWidgetItem *findContextItem(QTreeWidget *tree, WorkbenchContext context, const QString &text = QString())
{
    if (tree == nullptr) {
        return nullptr;
    }
    for (QTreeWidgetItemIterator it(tree); *it != nullptr; ++it) {
        auto *item = *it;
        if (contextFor(item) != context) {
            continue;
        }
        if (text.isEmpty() || item->text(0) == text) {
            return item;
        }
    }
    return nullptr;
}

void configureModelTree(QMainWindow &window, QTreeWidget *tree)
{
    if (tree == nullptr) {
        return;
    }

    tree->clear();
    tree->setHeaderHidden(true);
    tree->setRootIsDecorated(true);
    tree->setIndentation(16);
    tree->setUniformRowHeights(true);
    tree->setAnimated(false);
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    tree->setAccessibleName(QStringLiteral("Model Ağacı"));

    QStyle *style = window.style();

    auto *model = makeTopItem(
        tree, QStringLiteral("Model"), -999, WorkbenchContext::Model,
        style, QStyle::SP_DirHomeIcon);
    QFont rootFont = tree->font();
    rootFont.setBold(true);
    model->setFont(0, rootFont);

    makeItem(model, QStringLiteral("Geometry"), 0, WorkbenchContext::Geometry,
             style, QStyle::SP_FileIcon);
    makeItem(model, QStringLiteral("Materials"), 2, WorkbenchContext::Material,
             style, QStyle::SP_FileDialogDetailedView);
    makeItem(model, QStringLiteral("Sections"), 3, WorkbenchContext::Section,
             style, QStyle::SP_FileDialogContentsView);
    auto *connections = makeItem(model, QStringLiteral("Connections"), -999, WorkbenchContext::Connections,
                                 style, QStyle::SP_DirLinkIcon);
    connections->setToolTip(0, QStringLiteral("Contact ve bağlantı nesneleri bu model grubu altında yönetilecek."));
    makeItem(model, QStringLiteral("Mesh"), 1, WorkbenchContext::Mesh,
             style, QStyle::SP_ComputerIcon);
    model->setExpanded(true);

    auto *analysis = makeTopItem(
        tree, QStringLiteral("Static Structural 1"), 5, WorkbenchContext::Analysis,
        style, QStyle::SP_ArrowForward);
    analysis->setFont(0, rootFont);

    makeItem(analysis, QStringLiteral("Analysis Settings"), 5, WorkbenchContext::Analysis,
             style, QStyle::SP_FileDialogInfoView);
    makeItem(analysis, QStringLiteral("Loads and Boundary Conditions"), 4, WorkbenchContext::Loads,
             style, QStyle::SP_ArrowRight);
    auto *solution = makeItem(analysis, QStringLiteral("Solution"), kOpenResults, WorkbenchContext::Results,
                              style, QStyle::SP_DialogApplyButton);
    auto *empty = new QTreeWidgetItem(solution, {QStringLiteral("Henüz sonuç yok")});
    empty->setData(0, kContextRole, static_cast<int>(WorkbenchContext::Results));
    empty->setFlags(empty->flags() & ~Qt::ItemIsSelectable);
    empty->setForeground(0, window.palette().brush(QPalette::PlaceholderText));
    analysis->setExpanded(true);
    solution->setExpanded(true);

    tree->clearSelection();
    tree->setCurrentItem(nullptr);
}

int utilityTabIndex(QTabWidget *tabs, const QString &needle)
{
    if (tabs == nullptr) {
        return -1;
    }
    for (int i = 0; i < tabs->count(); ++i) {
        if (tabs->tabText(i).contains(needle, Qt::CaseInsensitive)) {
            return i;
        }
    }
    return -1;
}

void showUtilityTab(QDockWidget *dock, QTabWidget *tabs, const QString &needle)
{
    if (dock == nullptr || tabs == nullptr) {
        return;
    }
    const int index = utilityTabIndex(tabs, needle);
    if (index >= 0) {
        tabs->setCurrentIndex(index);
    }
    dock->show();
}

QString normalizedResultName(const QString &source)
{
    if (source.contains(QStringLiteral("Mesh max |u|"), Qt::CaseInsensitive)
        || source.contains(QStringLiteral("Maksimum Toplam Deformasyon"), Qt::CaseInsensitive)) {
        return QStringLiteral("Total Deformation");
    }
    if (source.contains(QStringLiteral("Mesh max von Mises"), Qt::CaseInsensitive)
        || source.contains(QStringLiteral("Maksimum von Mises"), Qt::CaseInsensitive)) {
        return QStringLiteral("Equivalent Stress");
    }
    if (source.contains(QStringLiteral("ΣRx"), Qt::CaseInsensitive)
        || source.contains(QStringLiteral("Reaksiyon"), Qt::CaseInsensitive)) {
        return QStringLiteral("Reaction Force");
    }
    if (source.contains(QStringLiteral("Probe"), Qt::CaseInsensitive)) {
        return source.contains(QStringLiteral("Node"), Qt::CaseInsensitive)
            ? QStringLiteral("Probe Node")
            : QStringLiteral("Probe Displacement");
    }
    return source;
}

void normalizeResultTable(QTabWidget *tabs)
{
    const int resultTab = utilityTabIndex(tabs, QStringLiteral("Sonuç"));
    auto *table = resultTab >= 0 ? qobject_cast<QTableWidget *>(tabs->widget(resultTab)) : nullptr;
    if (table == nullptr) {
        return;
    }
    if (auto *header = table->horizontalHeader()) {
        header->setMinimumSectionSize(120);
        header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(1, QHeaderView::Stretch);
    }
    if (auto *vertical = table->verticalHeader()) {
        vertical->hide();
    }
    for (int row = 0; row < table->rowCount(); ++row) {
        if (auto *item = table->item(row, 0)) {
            item->setText(normalizedResultName(item->text()));
        }
    }
}

void syncSolutionTree(QTreeWidget *tree, QTabWidget *tabs)
{
    if (tree == nullptr || tabs == nullptr) {
        return;
    }
    auto *solution = findContextItem(tree, WorkbenchContext::Results, QStringLiteral("Solution"));
    if (solution == nullptr) {
        return;
    }

    const int resultTab = utilityTabIndex(tabs, QStringLiteral("Sonuç"));
    auto *table = resultTab >= 0 ? qobject_cast<QTableWidget *>(tabs->widget(resultTab)) : nullptr;
    if (table == nullptr) {
        return;
    }

    while (solution->childCount() > 0) {
        delete solution->takeChild(0);
    }

    QSet<QString> names;
    for (int row = 0; row < table->rowCount(); ++row) {
        auto *item = table->item(row, 0);
        if (item == nullptr || item->text().trimmed().isEmpty()) {
            continue;
        }
        const QString name = normalizedResultName(item->text().trimmed());
        if (names.contains(name)) {
            continue;
        }
        names.insert(name);
        auto *child = new QTreeWidgetItem(solution, {name});
        child->setData(0, kInspectorRole, kOpenResults);
        child->setData(0, kContextRole, static_cast<int>(WorkbenchContext::Results));
    }

    if (names.isEmpty()) {
        auto *empty = new QTreeWidgetItem(solution, {QStringLiteral("Henüz sonuç yok")});
        empty->setData(0, kContextRole, static_cast<int>(WorkbenchContext::Results));
        empty->setFlags(empty->flags() & ~Qt::ItemIsSelectable);
    }
    solution->setExpanded(true);
}

void polishLegacyEngineeringPages(GeometryPanel *geometry, PrePostPanel *prePost)
{
    if (geometry != nullptr) {
        for (auto *group : geometry->findChildren<QGroupBox *>()) {
            group->setFlat(true);
            if (group->title().contains(QStringLiteral("CAD Geometry"), Qt::CaseInsensitive)) {
                group->setTitle(QStringLiteral("CAD Geometry"));
            } else if (group->title().contains(QStringLiteral("Custom Section"), Qt::CaseInsensitive)) {
                group->setTitle(QStringLiteral("Section / DXF"));
            }
        }
        for (auto *label : geometry->findChildren<QLabel *>()) {
            if (label->text().contains(QStringLiteral("CAD B-Rep"), Qt::CaseInsensitive)) {
                label->hide();
            }
        }
    }

    if (prePost != nullptr) {
        for (auto *group : prePost->findChildren<QGroupBox *>()) {
            group->setFlat(true);
            if (group->title().contains(QStringLiteral("Structured HEX8 Mesh"), Qt::CaseInsensitive)) {
                group->setTitle(QStringLiteral("Structured HEX8"));
            } else if (group->title().contains(QStringLiteral("Geometry Assignment"), Qt::CaseInsensitive)) {
                group->hide();
            }
        }
        for (auto *button : prePost->findChildren<QPushButton *>()) {
            const QString text = button->text();
            if (text.contains(QStringLiteral("CSV"), Qt::CaseInsensitive)
                || text.contains(QStringLiteral("VTK"), Qt::CaseInsensitive)
                || text.contains(QStringLiteral("Kesit Probe"), Qt::CaseInsensitive)) {
                button->hide();
            }
        }
    }
}

QComboBox *findAnalysisTypeCombo(QMainWindow &window)
{
    for (auto *combo : window.findChildren<QComboBox *>()) {
        const bool linear = combo->findText(QStringLiteral("Linear Static")) >= 0
            || combo->findText(QStringLiteral("Lineer Statik")) >= 0;
        const bool modal = combo->findText(QStringLiteral("Modal / Free Vibration")) >= 0
            || combo->findText(QStringLiteral("Modal / Serbest Titreşim")) >= 0;
        if (linear && modal) {
            return combo;
        }
    }
    return nullptr;
}

QComboBox *findModeSelector(QWidget *page)
{
    if (page == nullptr) {
        return nullptr;
    }
    for (auto *combo : page->findChildren<QComboBox *>()) {
        if (combo->findText(QStringLiteral("Mode 1")) >= 0
            && combo->findText(QStringLiteral("Mode 2")) >= 0) {
            return combo;
        }
    }
    return nullptr;
}

QComboBox *findFormulationCombo(QMainWindow &window)
{
    for (auto *combo : window.findChildren<QComboBox *>()) {
        for (int i = 0; i < combo->count(); ++i) {
            const QString item = combo->itemText(i);
            if (item.contains(QStringLiteral("Displacement-only"), Qt::CaseInsensitive)
                || item.contains(QStringLiteral("Mixed u-p"), Qt::CaseInsensitive)
                || item.contains(QStringLiteral("Nearly Incompressible"), Qt::CaseInsensitive)) {
                return combo;
            }
        }
    }
    return nullptr;
}

QPushButton *configureAnalysisContext(QMainWindow &window, PrePostPanel *prePost)
{
    auto *analysisType = findAnalysisTypeCombo(window);
    if (analysisType == nullptr) {
        return nullptr;
    }

    QWidget *page = analysisType->parentWidget();
    auto *form = page != nullptr ? qobject_cast<QFormLayout *>(page->layout()) : nullptr;
    if (page == nullptr || form == nullptr) {
        return nullptr;
    }

    analysisType->setItemText(0, QStringLiteral("Linear Static"));
    analysisType->setItemText(1, QStringLiteral("Modal"));
    analysisType->setItemText(2, QStringLiteral("Nonlinear Static"));

    if (auto *formulation = findFormulationCombo(window)) {
        for (int i = 0; i < formulation->count(); ++i) {
            const QString text = formulation->itemText(i);
            if (text.contains(QStringLiteral("Displacement-only"), Qt::CaseInsensitive)) {
                formulation->setItemText(i, QStringLiteral("Automatic / Standard"));
            } else if (text.contains(QStringLiteral("Mixed u-p"), Qt::CaseInsensitive)) {
                formulation->setItemText(i, QStringLiteral("Nearly Incompressible"));
            }
        }
    }

    auto *modeSelector = findModeSelector(page);
    QWidget *modeLabel = modeSelector != nullptr ? form->labelForField(modeSelector) : nullptr;
    auto *advanced = page->findChild<QToolButton *>(QStringLiteral("Dynamics26AdvancedSolverDisclosure"));

    auto *solve = page->findChild<QPushButton *>(QStringLiteral("Dynamics26IntegratedSolve"));
    if (solve == nullptr) {
        solve = new QPushButton(QStringLiteral("Solve"), page);
        solve->setObjectName(QStringLiteral("Dynamics26IntegratedSolve"));
        form->insertRow(1, solve);
    }

    auto *reason = page->findChild<QLabel *>(QStringLiteral("Dynamics26SolveAvailabilityReason"));
    if (reason == nullptr) {
        reason = new QLabel(page);
        reason->setObjectName(QStringLiteral("Dynamics26SolveAvailabilityReason"));
        reason->setWordWrap(true);
        reason->setForegroundRole(QPalette::PlaceholderText);
        form->insertRow(2, reason);
    }

    if (prePost != nullptr) {
        QObject::connect(solve, &QPushButton::clicked, prePost, [prePost] {
            QMetaObject::invokeMethod(prePost, "solveLinear", Qt::QueuedConnection);
        });
    }

    const auto refresh = [analysisType, modeSelector, modeLabel, advanced, solve, reason] {
        const int index = analysisType->currentIndex();
        const bool linear = index == 0;
        const bool modal = index == 1;
        const bool nonlinear = index == 2;

        if (modeSelector != nullptr) modeSelector->setVisible(modal);
        if (modeLabel != nullptr) modeLabel->setVisible(modal);
        if (advanced != nullptr) {
            if (!nonlinear) advanced->setChecked(false);
            advanced->setVisible(nonlinear);
        }
        solve->setEnabled(linear);
        if (linear) {
            reason->hide();
        } else {
            reason->setText(modal
                ? QStringLiteral("Modal GUI solve workflow is not enabled in this Alpha.1 preview.")
                : QStringLiteral("Nonlinear GUI solve workflow is not enabled in this Alpha.1 preview."));
            reason->show();
        }
    };
    refresh();
    QObject::connect(analysisType, qOverload<int>(&QComboBox::currentIndexChanged), page,
                     [refresh](int) { refresh(); });
    return solve;
}

QWidget *installGraphicsHeader(QMainWindow &window, QTreeWidget *tree, QAction *fitAction)
{
    auto *workspace = window.findChild<QSplitter *>(QStringLiteral("Dynamics26WorkspaceSplitter"));
    if (workspace == nullptr || workspace->count() < 3) {
        return nullptr;
    }

    QWidget *viewport = workspace->widget(1);
    if (viewport == nullptr || viewport->objectName() == QStringLiteral("Dynamics26GraphicsHost")) {
        return viewport;
    }

    auto *host = new QWidget(workspace);
    host->setObjectName(QStringLiteral("Dynamics26GraphicsHost"));
    auto *layout = new QVBoxLayout(host);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *bar = new QFrame(host);
    bar->setObjectName(QStringLiteral("Dynamics26GraphicsBar"));
    bar->setFrameShape(QFrame::NoFrame);
    bar->setMinimumHeight(30);
    bar->setMaximumHeight(32);
    auto *barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(8, 2, 6, 2);
    barLayout->setSpacing(6);

    auto *title = new QLabel(QStringLiteral("GRAPHICS"), bar);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    titleFont.setPointSizeF(qMax(9.0, titleFont.pointSizeF() - 1.0));
    title->setFont(titleFont);
    barLayout->addWidget(title);

    auto *context = new QLabel(QStringLiteral("Model"), bar);
    context->setObjectName(QStringLiteral("Dynamics26GraphicsContext"));
    context->setForegroundRole(QPalette::PlaceholderText);
    barLayout->addWidget(context);
    barLayout->addStretch(1);

    if (fitAction != nullptr) {
        auto *fit = new QToolButton(bar);
        fit->setDefaultAction(fitAction);
        fit->setAutoRaise(true);
        fit->setToolButtonStyle(Qt::ToolButtonIconOnly);
        fit->setToolTip(QStringLiteral("Fit View"));
        barLayout->addWidget(fit);
    }

    auto *line = new QFrame(host);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    workspace->replaceWidget(1, host);
    viewport->setParent(host);
    layout->addWidget(bar);
    layout->addWidget(line);
    layout->addWidget(viewport, 1);

    if (tree != nullptr) {
        QObject::connect(tree, &QTreeWidget::currentItemChanged, host,
                         [context](QTreeWidgetItem *current, QTreeWidgetItem *) {
            context->setText(contextTitle(contextFor(current)));
        });
    }
    return host;
}

void configureDetailsHeader(QMainWindow &window)
{
    auto *panel = window.findChild<QFrame *>(QStringLiteral("Dynamics26InspectorPanel"));
    if (panel == nullptr) {
        return;
    }
    panel->setAccessibleName(QStringLiteral("Details"));
    const auto labels = panel->findChildren<QLabel *>(QString(), Qt::FindDirectChildrenOnly);
    if (!labels.isEmpty()) {
        labels.first()->setText(QStringLiteral("DETAILS"));
    }
}

QAction *makeAction(QMainWindow &window, const QString &text, const QString &tip, QStyle::StandardPixmap icon)
{
    auto *action = new QAction(text, &window);
    action->setToolTip(tip);
    if (window.style() != nullptr) {
        action->setIcon(window.style()->standardIcon(icon));
    }
    return action;
}

void configureGlobalToolbar(
    QMainWindow &window,
    QTreeWidget *tree,
    GeometryPanel *geometry,
    PrePostPanel *prePost,
    QPushButton *integratedSolve)
{
    auto *toolbar = window.findChild<QToolBar *>(QStringLiteral("Dynamics26MainToolbar"));
    if (toolbar == nullptr) {
        return;
    }

    QAction *navigatorAction = actionContaining(window, QStringLiteral("Proje Gezgini"));
    QAction *fitAction = actionContaining(window, QStringLiteral("Görünüme Sığdır"));
    QAction *diagnosticsAction = actionContaining(window, QStringLiteral("Sonuçlar ve Tanılama"));
    QAction *inspectorAction = actionContaining(window, QStringLiteral("Özellikler"));

    auto *importAction = makeAction(
        window, QStringLiteral("Import"), QStringLiteral("Import STEP / STP geometry"), QStyle::SP_DialogOpenButton);
    importAction->setEnabled(geometry != nullptr);
    QObject::connect(importAction, &QAction::triggered, &window, [geometry, tree] {
        if (auto *item = findContextItem(tree, WorkbenchContext::Geometry)) tree->setCurrentItem(item);
        if (geometry != nullptr) QMetaObject::invokeMethod(geometry, "importStep", Qt::QueuedConnection);
    });

    auto *meshAction = makeAction(
        window, QStringLiteral("Generate Mesh"), QStringLiteral("Generate the configured Structured HEX8 mesh"),
        QStyle::SP_ComputerIcon);
    meshAction->setEnabled(prePost != nullptr);
    QObject::connect(meshAction, &QAction::triggered, &window, [prePost, tree] {
        if (auto *item = findContextItem(tree, WorkbenchContext::Mesh)) tree->setCurrentItem(item);
        if (prePost != nullptr) QMetaObject::invokeMethod(prePost, "generateMesh", Qt::QueuedConnection);
    });

    auto *solveAction = makeAction(
        window, QStringLiteral("Solve"), QStringLiteral("Solve the selected integrated linear analysis"),
        QStyle::SP_MediaPlay);
    const auto refreshSolve = [solveAction, integratedSolve] {
        const bool enabled = integratedSolve != nullptr && integratedSolve->isEnabled();
        solveAction->setEnabled(enabled);
        solveAction->setToolTip(enabled
            ? QStringLiteral("Solve Static Structural 1")
            : QStringLiteral("The selected analysis is not connected to the GUI solve workflow yet."));
    };
    refreshSolve();
    QObject::connect(solveAction, &QAction::triggered, &window, [integratedSolve, tree] {
        if (auto *item = findContextItem(tree, WorkbenchContext::Analysis, QStringLiteral("Static Structural 1"))) {
            tree->setCurrentItem(item);
        }
        if (integratedSolve != nullptr && integratedSolve->isEnabled()) integratedSolve->click();
    });
    if (auto *analysisType = findAnalysisTypeCombo(window)) {
        QObject::connect(analysisType, qOverload<int>(&QComboBox::currentIndexChanged), &window,
                         [refreshSolve](int) { refreshSolve(); });
    }

    toolbar->clear();
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolbar->setIconSize(QSize(18, 18));
    toolbar->setMinimumHeight(34);
    toolbar->setMaximumHeight(38);

    if (navigatorAction != nullptr) toolbar->addAction(navigatorAction);
    toolbar->addSeparator();
    toolbar->addAction(importAction);
    toolbar->addAction(meshAction);
    if (fitAction != nullptr) toolbar->addAction(fitAction);

    auto *spacer = new QWidget(toolbar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    auto *solveButton = new QToolButton(toolbar);
    solveButton->setObjectName(QStringLiteral("Dynamics26WorkbenchSolveButton"));
    solveButton->setDefaultAction(solveAction);
    solveButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    solveButton->setAutoRaise(false);
    toolbar->addWidget(solveButton);

    toolbar->addSeparator();
    if (diagnosticsAction != nullptr) toolbar->addAction(diagnosticsAction);
    if (inspectorAction != nullptr) toolbar->addAction(inspectorAction);
}

void configureUtilityWorkspace(QMainWindow &window, PrePostPanel *prePost, QTreeWidget *tree)
{
    auto *dock = window.findChild<QDockWidget *>(QStringLiteral("Dynamics26UtilityArea"));
    auto *tabs = dock != nullptr ? dock->findChild<QTabWidget *>(QStringLiteral("Dynamics26UtilityTabs")) : nullptr;
    if (dock == nullptr || tabs == nullptr) {
        return;
    }

    if (tabs->count() > 0) tabs->setTabText(0, QStringLiteral("Results"));
    if (tabs->count() > 1) tabs->setTabText(1, QStringLiteral("Convergence"));
    if (tabs->count() > 2) tabs->setTabText(2, QStringLiteral("Messages / Solver"));

    auto *minimalTitle = new QWidget(dock);
    minimalTitle->setFixedHeight(1);
    dock->setTitleBarWidget(minimalTitle);
    dock->hide();
    QTimer::singleShot(0, dock, [dock] { dock->hide(); });

    QStatusBar *status = window.statusBar();
    if (status == nullptr) {
        return;
    }
    status->clearMessage();
    status->setSizeGripEnabled(false);
    status->setMinimumHeight(22);
    status->setMaximumHeight(24);
    status->showMessage(QStringLiteral("Ready"));
    status->show();

    auto *diagnostics = status->findChild<QToolButton *>(QStringLiteral("Dynamics26DiagnosticsHandle"));
    if (diagnostics == nullptr) {
        diagnostics = new QToolButton(status);
        diagnostics->setObjectName(QStringLiteral("Dynamics26DiagnosticsHandle"));
        diagnostics->setText(QStringLiteral("Diagnostics"));
        diagnostics->setCheckable(true);
        diagnostics->setAutoRaise(true);
        diagnostics->setToolTip(QStringLiteral("Show or hide Results / Convergence / Messages"));
        status->addPermanentWidget(diagnostics);
    }
    diagnostics->setChecked(false);
    QObject::connect(diagnostics, &QToolButton::toggled, dock, &QWidget::setVisible);
    QObject::connect(dock, &QDockWidget::visibilityChanged, diagnostics, &QToolButton::setChecked);

    normalizeResultTable(tabs);
    syncSolutionTree(tree, tabs);
    const int resultTab = utilityTabIndex(tabs, QStringLiteral("Result"));
    if (resultTab >= 0) {
        if (auto *table = qobject_cast<QTableWidget *>(tabs->widget(resultTab))) {
            QObject::connect(table, &QTableWidget::itemChanged, &window, [tree, tabs](QTableWidgetItem *) {
                QTimer::singleShot(0, tree, [tree, tabs] { syncSolutionTree(tree, tabs); });
            });
        }
    }

    if (prePost != nullptr) {
        QObject::connect(prePost, &PrePostPanel::message, status,
            [status, dock, tabs](const QString &message) {
                static const QRegularExpression meshPattern(
                    QStringLiteral("structured mesh oluşturuldu: (\\d+) node, (\\d+) HEX8"),
                    QRegularExpression::CaseInsensitiveOption);
                const auto match = meshPattern.match(message);
                if (match.hasMatch()) {
                    status->showMessage(QStringLiteral("%1 nodes • %2 HEX8 • Ready")
                        .arg(match.captured(1), match.captured(2)));
                    return;
                }
                if (message.contains(QStringLiteral("başarısız"), Qt::CaseInsensitive)
                    || message.contains(QStringLiteral("hata"), Qt::CaseInsensitive)) {
                    status->showMessage(QStringLiteral("Error • %1").arg(message));
                    showUtilityTab(dock, tabs, QStringLiteral("Messages"));
                }
            });

        QObject::connect(prePost, &PrePostPanel::solveCompleted, status,
            [status, dock, tabs, tree](double maxU, double maxVm, double, qlonglong, double) {
                status->showMessage(QStringLiteral("Solved • umax %1 mm • von Mises %2 MPa")
                    .arg(maxU, 0, 'g', 5).arg(maxVm, 0, 'g', 5));
                normalizeResultTable(tabs);
                syncSolutionTree(tree, tabs);
                showUtilityTab(dock, tabs, QStringLiteral("Result"));
            });
    }
}

void connectViewportContext(QMainWindow &window, QTreeWidget *tree, GeometryPanel *geometry, PrePostPanel *prePost)
{
    if (tree == nullptr) {
        return;
    }

    QObject::connect(tree, &QTreeWidget::currentItemChanged, &window,
        [geometry, prePost](QTreeWidgetItem *current, QTreeWidgetItem *) {
            const WorkbenchContext context = contextFor(current);
            if (context == WorkbenchContext::Results) {
                if (prePost != nullptr && !prePost->showResultsPreview()) {
                    (void)prePost->showMeshPreview();
                }
                return;
            }
            if (context == WorkbenchContext::Geometry) {
                if (geometry != nullptr && geometry->showCurrentGeometry()) return;
                if (prePost != nullptr) (void)prePost->showMeshPreview();
                return;
            }
            if (prePost != nullptr && prePost->showMeshPreview()) return;
            if (geometry != nullptr) (void)geometry->showCurrentGeometry();
        });

    if (prePost != nullptr) {
        QObject::connect(prePost, &PrePostPanel::solveCompleted, &window,
            [tree, prePost](double, double, double, qlonglong, double) {
                if (contextFor(tree->currentItem()) != WorkbenchContext::Results) {
                    (void)prePost->showMeshPreview();
                }
            });
    }
}

} // namespace

namespace dynamics26::gui {

void installCaeWorkbenchController(QMainWindow &window)
{
    auto *tree = window.findChild<QTreeWidget *>(QStringLiteral("Dynamics26Navigator"));
    auto *geometry = window.findChild<GeometryPanel *>();
    auto *prePost = window.findChild<PrePostPanel *>();

    configureModelTree(window, tree);
    configureDetailsHeader(window);
    polishLegacyEngineeringPages(geometry, prePost);
    QPushButton *integratedSolve = configureAnalysisContext(window, prePost);

    QAction *fitAction = actionContaining(window, QStringLiteral("Görünüme Sığdır"));
    installGraphicsHeader(window, tree, fitAction);
    configureGlobalToolbar(window, tree, geometry, prePost, integratedSolve);
    configureUtilityWorkspace(window, prePost, tree);
    connectViewportContext(window, tree, geometry, prePost);
}

} // namespace dynamics26::gui
