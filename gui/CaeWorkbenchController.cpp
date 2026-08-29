#include "CaeWorkbenchController.h"

#include "GeometryPanel.h"
#include "PrePostPanel.h"

#include <QAbstractItemView>
#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMainWindow>
#include <QMetaObject>
#include <QPalette>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSet>
#include <QSize>
#include <QSizePolicy>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
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

constexpr int kInspectorRole = Qt::UserRole + 1;
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

struct DetailsUi {
    QFrame *panel = nullptr;
    QLabel *objectTitle = nullptr;
    QStackedWidget *pages = nullptr;
    QLabel *geometrySource = nullptr;
    QLabel *geometryBodies = nullptr;
    QLabel *geometryStatus = nullptr;
    QDoubleSpinBox *length = nullptr;
    QDoubleSpinBox *width = nullptr;
    QDoubleSpinBox *height = nullptr;
    QSpinBox *nx = nullptr;
    QSpinBox *ny = nullptr;
    QSpinBox *nz = nullptr;
    QLabel *meshNodes = nullptr;
    QLabel *meshElements = nullptr;
    QLabel *meshStatus = nullptr;
    QComboBox *analysisType = nullptr;
    QLabel *analysisStatus = nullptr;
    QLabel *resultStatus = nullptr;
    QLabel *resultU = nullptr;
    QLabel *resultVm = nullptr;
};

QAction *actionContaining(QObject &root, const QString &needle)
{
    for (auto *action : root.findChildren<QAction *>()) {
        if (action != nullptr && action->text().contains(needle, Qt::CaseInsensitive)) return action;
    }
    return nullptr;
}

QTreeWidgetItem *addItem(QTreeWidgetItem *parent, const QString &text, WorkbenchContext context,
                         int inspectorPage, QStyle *style, QStyle::StandardPixmap icon)
{
    auto *item = new QTreeWidgetItem(parent, {text});
    item->setData(0, kContextRole, static_cast<int>(context));
    if (inspectorPage != -999) item->setData(0, kInspectorRole, inspectorPage);
    if (style != nullptr) item->setIcon(0, style->standardIcon(icon));
    return item;
}

QTreeWidgetItem *addTop(QTreeWidget *tree, const QString &text, WorkbenchContext context,
                        int inspectorPage, QStyle *style, QStyle::StandardPixmap icon)
{
    auto *item = new QTreeWidgetItem(tree, {text});
    item->setData(0, kContextRole, static_cast<int>(context));
    if (inspectorPage != -999) item->setData(0, kInspectorRole, inspectorPage);
    if (style != nullptr) item->setIcon(0, style->standardIcon(icon));
    return item;
}

WorkbenchContext contextFor(QTreeWidgetItem *item)
{
    if (item == nullptr) return WorkbenchContext::Model;
    const QVariant value = item->data(0, kContextRole);
    return value.isValid() ? static_cast<WorkbenchContext>(value.toInt()) : WorkbenchContext::Model;
}

QString contextTitle(WorkbenchContext context)
{
    switch (context) {
    case WorkbenchContext::Geometry: return QStringLiteral("Geometri");
    case WorkbenchContext::Material: return QStringLiteral("Malzemeler");
    case WorkbenchContext::Section: return QStringLiteral("Kesitler");
    case WorkbenchContext::Connections: return QStringLiteral("Bağlantılar");
    case WorkbenchContext::Mesh: return QStringLiteral("Mesh");
    case WorkbenchContext::Loads: return QStringLiteral("Yük / Sınır Şartı");
    case WorkbenchContext::Analysis: return QStringLiteral("Statik Yapısal");
    case WorkbenchContext::Results: return QStringLiteral("Çözüm");
    case WorkbenchContext::Model:
    default: return QStringLiteral("Model");
    }
}

QTreeWidgetItem *findContextItem(QTreeWidget *tree, WorkbenchContext context, const QString &text = QString())
{
    if (tree == nullptr) return nullptr;
    for (QTreeWidgetItemIterator it(tree); *it != nullptr; ++it) {
        auto *item = *it;
        if (contextFor(item) == context && (text.isEmpty() || item->text(0) == text)) return item;
    }
    return nullptr;
}

void configureModelTree(QMainWindow &window, QTreeWidget *tree)
{
    if (tree == nullptr) return;
    tree->clear();
    tree->setHeaderHidden(true);
    tree->setRootIsDecorated(true);
    tree->setIndentation(15);
    tree->setUniformRowHeights(true);
    tree->setAnimated(false);
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    tree->setAccessibleName(QStringLiteral("Model Ağacı"));

    auto *model = addTop(tree, QStringLiteral("Model"), WorkbenchContext::Model, -999,
                         window.style(), QStyle::SP_DirHomeIcon);
    QFont rootFont = tree->font();
    rootFont.setBold(true);
    model->setFont(0, rootFont);
    addItem(model, QStringLiteral("Geometri"), WorkbenchContext::Geometry, 0, window.style(), QStyle::SP_FileIcon);
    auto *materials = addItem(model, QStringLiteral("Malzemeler"), WorkbenchContext::Material, 2, window.style(), QStyle::SP_FileDialogDetailedView);
    addItem(materials, QStringLiteral("Malzeme 1"), WorkbenchContext::Material, 2, window.style(), QStyle::SP_FileIcon);
    addItem(model, QStringLiteral("Kesitler"), WorkbenchContext::Section, 3, window.style(), QStyle::SP_FileDialogContentsView);
    addItem(model, QStringLiteral("Bağlantılar"), WorkbenchContext::Connections, -999, window.style(), QStyle::SP_DirLinkIcon);
    addItem(model, QStringLiteral("Mesh"), WorkbenchContext::Mesh, 1, window.style(), QStyle::SP_ComputerIcon);
    model->setExpanded(true);
    materials->setExpanded(true);

    auto *analysis = addTop(tree, QStringLiteral("Statik Yapısal 1"), WorkbenchContext::Analysis, 5,
                            window.style(), QStyle::SP_ArrowForward);
    analysis->setFont(0, rootFont);
    addItem(analysis, QStringLiteral("Analiz Ayarları"), WorkbenchContext::Analysis, 5, window.style(), QStyle::SP_FileDialogInfoView);
    addItem(analysis, QStringLiteral("Sabit Mesnet"), WorkbenchContext::Loads, 4, window.style(), QStyle::SP_DialogApplyButton);
    addItem(analysis, QStringLiteral("Kuvvet"), WorkbenchContext::Loads, 4, window.style(), QStyle::SP_ArrowRight);
    addItem(analysis, QStringLiteral("Çözüm"), WorkbenchContext::Results, kOpenResults, window.style(), QStyle::SP_DialogApplyButton);
    analysis->setExpanded(true);
    tree->clearSelection();
}

QLabel *sectionTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    QFont font = label->font();
    font.setBold(true);
    font.setPointSizeF(qMax(9.0, font.pointSizeF() - 1.0));
    label->setFont(font);
    label->setForegroundRole(QPalette::PlaceholderText);
    return label;
}

QLabel *valueLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

QWidget *formSection(const QString &title, const QList<QPair<QString, QWidget *>> &rows, QWidget *parent)
{
    auto *host = new QWidget(parent);
    auto *layout = new QVBoxLayout(host);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);
    layout->addWidget(sectionTitle(title, host));
    auto *form = new QFormLayout;
    form->setContentsMargins(0, 0, 0, 0);
    form->setHorizontalSpacing(10);
    form->setVerticalSpacing(7);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    for (const auto &row : rows) form->addRow(row.first, row.second);
    layout->addLayout(form);
    return host;
}

QWidget *simplePage(const QString &title, const QString &body, QWidget *parent)
{
    auto *page = new QWidget(parent);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    layout->addWidget(sectionTitle(title, page));
    auto *label = valueLabel(body, page);
    label->setForegroundRole(QPalette::PlaceholderText);
    layout->addWidget(label);
    layout->addStretch(1);
    return page;
}

void refreshGeometryDetails(const DetailsUi &ui, GeometryPanel *geometry)
{
    if (geometry == nullptr || ui.geometrySource == nullptr) return;
    const bool ready = geometry->hasCadGeometry();
    ui.geometrySource->setText(geometry->currentStepPath().isEmpty()
        ? QStringLiteral("—") : QFileInfo(geometry->currentStepPath()).fileName());
    ui.geometryBodies->setText(QString::number(geometry->bodyCount()));
    ui.geometryStatus->setText(ready ? QStringLiteral("Güncel") : QStringLiteral("Geometri içe aktarılmadı"));
}

void refreshMeshDetails(const DetailsUi &ui, PrePostPanel *prePost)
{
    if (prePost == nullptr || ui.length == nullptr) return;
    const QSignalBlocker b1(ui.length), b2(ui.width), b3(ui.height), b4(ui.nx), b5(ui.ny), b6(ui.nz);
    ui.length->setValue(prePost->lengthMm());
    ui.width->setValue(prePost->widthMm());
    ui.height->setValue(prePost->heightMm());
    ui.nx->setValue(prePost->divisionsX());
    ui.ny->setValue(prePost->divisionsY());
    ui.nz->setValue(prePost->divisionsZ());
    ui.meshNodes->setText(QString::number(prePost->meshNodeCount()));
    ui.meshElements->setText(QString::number(prePost->meshElementCount()));
    ui.meshStatus->setText(prePost->hasMesh() ? QStringLiteral("Güncel") : QStringLiteral("Oluşturulmadı"));
}

DetailsUi buildDetailsSurface(QMainWindow &window, GeometryPanel *geometry, PrePostPanel *prePost)
{
    DetailsUi ui;
    ui.panel = window.findChild<QFrame *>(QStringLiteral("Dynamics26InspectorPanel"));
    if (ui.panel == nullptr) return ui;
    ui.panel->setAccessibleName(QStringLiteral("Detaylar"));
    ui.panel->setMinimumWidth(290);
    ui.panel->setMaximumWidth(390);

    // Eski GeometryPanel/PrePostPanel/Material editor widget'ları yaşamaya devam
    // eder ve project/backend state'inin sahibidir; fakat artık görünür Details
    // yüzeyi olarak kullanılmazlar. Bu, legacy formu yeni kutuya taşımak yerine
    // gerçek CAE Inspector ile backend'i ayıran Alpha.1 recovery adımıdır.
    for (auto *child : ui.panel->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly)) child->hide();

    auto *root = new QWidget(ui.panel);
    root->setObjectName(QStringLiteral("Dynamics26CompactDetails"));
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(2, 0, 2, 4);
    rootLayout->setSpacing(6);
    rootLayout->addWidget(sectionTitle(QStringLiteral("DETAYLAR"), root));
    ui.objectTitle = new QLabel(QStringLiteral("Model"), root);
    QFont objectFont = ui.objectTitle->font();
    objectFont.setBold(true);
    objectFont.setPointSizeF(objectFont.pointSizeF() + 1.5);
    ui.objectTitle->setFont(objectFont);
    rootLayout->addWidget(ui.objectTitle);

    ui.pages = new QStackedWidget(root);
    rootLayout->addWidget(ui.pages, 1);

    ui.pages->addWidget(simplePage(QStringLiteral("Model"),
        QStringLiteral("Model ağacından bir mühendislik nesnesi seçin. Seçim, Detaylar ve Graphics bağlamını birlikte değiştirir."), ui.pages));

    auto *geometryPage = new QWidget(ui.pages);
    auto *geometryLayout = new QVBoxLayout(geometryPage);
    geometryLayout->setContentsMargins(0, 0, 0, 0);
    geometryLayout->setSpacing(12);
    ui.geometrySource = valueLabel(QStringLiteral("—"), geometryPage);
    ui.geometryBodies = valueLabel(QStringLiteral("0"), geometryPage);
    ui.geometryStatus = valueLabel(QStringLiteral("Geometri içe aktarılmadı"), geometryPage);
    geometryLayout->addWidget(formSection(QStringLiteral("TANIM"), {
        {QStringLiteral("Kaynak"), ui.geometrySource},
        {QStringLiteral("Gövde"), ui.geometryBodies},
        {QStringLiteral("Durum"), ui.geometryStatus}}, geometryPage));
    geometryLayout->addWidget(formSection(QStringLiteral("GÖRÜNTÜLEME"), {
        {QStringLiteral("Temsil"), valueLabel(QStringLiteral("Shaded with Edges"), geometryPage)},
        {QStringLiteral("CAD / Mesh"), valueLabel(QStringLiteral("Ayrı veri katmanları"), geometryPage)}}, geometryPage));
    auto *geometryHint = valueLabel(QStringLiteral("STEP / STP eklemek veya değiştirmek için üst araç çubuğundaki İçe Aktar komutunu kullanın."), geometryPage);
    geometryHint->setForegroundRole(QPalette::PlaceholderText);
    geometryLayout->addWidget(geometryHint);
    geometryLayout->addStretch(1);
    ui.pages->addWidget(geometryPage);

    ui.pages->addWidget(simplePage(QStringLiteral("Malzeme"),
        QStringLiteral("Malzeme 1 proje modelinde tanımlıdır. Profesyonel material inspector ve hyperelastic kartları Beta.1 kapsamında ayrı Details yüzeyine taşınacaktır."), ui.pages));
    ui.pages->addWidget(simplePage(QStringLiteral("Kesit"),
        QStringLiteral("Kesit tanımları Model > Kesitler altında yönetilir. DXF tabanlı kesit motoru korunmuştur."), ui.pages));
    ui.pages->addWidget(simplePage(QStringLiteral("Bağlantılar"),
        QStringLiteral("Contact ve bağlantı nesneleri bu model grubu altında yönetilecektir."), ui.pages));

    auto *meshPage = new QWidget(ui.pages);
    auto *meshLayout = new QVBoxLayout(meshPage);
    meshLayout->setContentsMargins(0, 0, 0, 0);
    meshLayout->setSpacing(12);
    auto makeDim = [meshPage](double value) { auto *s = new QDoubleSpinBox(meshPage); s->setRange(0.001, 1.0e6); s->setDecimals(3); s->setSuffix(QStringLiteral(" mm")); s->setValue(value); return s; };
    auto makeDiv = [meshPage](int value) { auto *s = new QSpinBox(meshPage); s->setRange(1, 100); s->setValue(value); return s; };
    ui.length = makeDim(prePost ? prePost->lengthMm() : 100.0);
    ui.width = makeDim(prePost ? prePost->widthMm() : 20.0);
    ui.height = makeDim(prePost ? prePost->heightMm() : 20.0);
    ui.nx = makeDiv(prePost ? prePost->divisionsX() : 2);
    ui.ny = makeDiv(prePost ? prePost->divisionsY() : 1);
    ui.nz = makeDiv(prePost ? prePost->divisionsZ() : 1);
    ui.meshNodes = valueLabel(QStringLiteral("0"), meshPage);
    ui.meshElements = valueLabel(QStringLiteral("0"), meshPage);
    ui.meshStatus = valueLabel(QStringLiteral("Oluşturulmadı"), meshPage);
    meshLayout->addWidget(formSection(QStringLiteral("TANIM"), {
        {QStringLiteral("Yöntem"), valueLabel(QStringLiteral("Structured HEX8"), meshPage)},
        {QStringLiteral("Uzunluk"), ui.length}, {QStringLiteral("Genişlik"), ui.width}, {QStringLiteral("Yükseklik"), ui.height},
        {QStringLiteral("Nx"), ui.nx}, {QStringLiteral("Ny"), ui.ny}, {QStringLiteral("Nz"), ui.nz}}, meshPage));
    meshLayout->addWidget(formSection(QStringLiteral("İSTATİSTİK"), {
        {QStringLiteral("Düğüm"), ui.meshNodes}, {QStringLiteral("Eleman"), ui.meshElements}, {QStringLiteral("Durum"), ui.meshStatus}}, meshPage));
    auto *meshHint = valueLabel(QStringLiteral("Tanımı değiştirdikten sonra üst araç çubuğundaki Mesh Oluştur komutunu kullanın."), meshPage);
    meshHint->setForegroundRole(QPalette::PlaceholderText);
    meshLayout->addWidget(meshHint);
    meshLayout->addStretch(1);
    ui.pages->addWidget(meshPage);

    ui.pages->addWidget(simplePage(QStringLiteral("Yük / Sınır Şartı"),
        QStringLiteral("Verified baseline şu anda x-min yüzeyini sabitler ve x-max yüzeyine toplam X kuvveti uygular. Nesne tabanlı scoping Alpha.2+ kapsamında genişletilecektir."), ui.pages));

    auto *analysisPage = new QWidget(ui.pages);
    auto *analysisLayout = new QVBoxLayout(analysisPage);
    analysisLayout->setContentsMargins(0, 0, 0, 0);
    analysisLayout->setSpacing(12);
    ui.analysisType = new QComboBox(analysisPage);
    ui.analysisType->addItems({QStringLiteral("Lineer Statik"), QStringLiteral("Modal"), QStringLiteral("Nonlineer Statik")});
    ui.analysisStatus = valueLabel(QStringLiteral("Çözülebilir"), analysisPage);
    analysisLayout->addWidget(formSection(QStringLiteral("TANIM"), {
        {QStringLiteral("Analiz Türü"), ui.analysisType},
        {QStringLiteral("Formülasyon"), valueLabel(QStringLiteral("Automatic"), analysisPage)},
        {QStringLiteral("Durum"), ui.analysisStatus}}, analysisPage));
    auto *advanced = new QToolButton(analysisPage);
    advanced->setText(QStringLiteral("Gelişmiş Çözücü Ayarları"));
    advanced->setArrowType(Qt::RightArrow);
    advanced->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    advanced->setEnabled(false);
    advanced->setToolTip(QStringLiteral("Beta.1 engineering inspector kapsamında etkinleştirilecek."));
    analysisLayout->addWidget(advanced);
    analysisLayout->addStretch(1);
    ui.pages->addWidget(analysisPage);

    auto *resultsPage = new QWidget(ui.pages);
    auto *resultsLayout = new QVBoxLayout(resultsPage);
    resultsLayout->setContentsMargins(0, 0, 0, 0);
    resultsLayout->setSpacing(12);
    ui.resultStatus = valueLabel(QStringLiteral("Henüz çözüm yok"), resultsPage);
    ui.resultU = valueLabel(QStringLiteral("—"), resultsPage);
    ui.resultVm = valueLabel(QStringLiteral("—"), resultsPage);
    resultsLayout->addWidget(formSection(QStringLiteral("ÇÖZÜM"), {
        {QStringLiteral("Durum"), ui.resultStatus},
        {QStringLiteral("Toplam Deformasyon"), ui.resultU},
        {QStringLiteral("Eşdeğer Gerilme"), ui.resultVm}}, resultsPage));
    auto *resultHint = valueLabel(QStringLiteral("Çözüm ağacından bir sonuç nesnesi seçildiğinde contour Graphics alanında gösterilir."), resultsPage);
    resultHint->setForegroundRole(QPalette::PlaceholderText);
    resultsLayout->addWidget(resultHint);
    resultsLayout->addStretch(1);
    ui.pages->addWidget(resultsPage);

    if (auto *layout = qobject_cast<QVBoxLayout *>(ui.panel->layout())) layout->addWidget(root, 1);

    if (prePost != nullptr) {
        const auto pushMeshDefinition = [ui, prePost] {
            prePost->setStructuredMeshDefinition(ui.length->value(), ui.width->value(), ui.height->value(),
                                                 ui.nx->value(), ui.ny->value(), ui.nz->value());
        };
        QObject::connect(ui.length, qOverload<double>(&QDoubleSpinBox::valueChanged), root, [pushMeshDefinition](double){ pushMeshDefinition(); });
        QObject::connect(ui.width, qOverload<double>(&QDoubleSpinBox::valueChanged), root, [pushMeshDefinition](double){ pushMeshDefinition(); });
        QObject::connect(ui.height, qOverload<double>(&QDoubleSpinBox::valueChanged), root, [pushMeshDefinition](double){ pushMeshDefinition(); });
        QObject::connect(ui.nx, qOverload<int>(&QSpinBox::valueChanged), root, [pushMeshDefinition](int){ pushMeshDefinition(); });
        QObject::connect(ui.ny, qOverload<int>(&QSpinBox::valueChanged), root, [pushMeshDefinition](int){ pushMeshDefinition(); });
        QObject::connect(ui.nz, qOverload<int>(&QSpinBox::valueChanged), root, [pushMeshDefinition](int){ pushMeshDefinition(); });
    }

    refreshGeometryDetails(ui, geometry);
    refreshMeshDetails(ui, prePost);
    return ui;
}

int pageForContext(WorkbenchContext context)
{
    return static_cast<int>(context);
}

QWidget *recomposeGraphicsFirstWorkspace(QMainWindow &window, QTreeWidget *tree, const DetailsUi &details, QAction *fitAction)
{
    auto *oldWorkspace = window.findChild<QSplitter *>(QStringLiteral("Dynamics26WorkspaceSplitter"));
    auto *navigatorPanel = window.findChild<QFrame *>(QStringLiteral("Dynamics26NavigatorPanel"));
    auto *viewport = window.findChild<QWidget *>(QStringLiteral("Dynamics26Viewport"));
    if (oldWorkspace == nullptr || navigatorPanel == nullptr || viewport == nullptr || details.panel == nullptr) return nullptr;

    const auto navLabels = navigatorPanel->findChildren<QLabel *>(QString(), Qt::FindDirectChildrenOnly);
    if (!navLabels.isEmpty()) navLabels.first()->setText(QStringLiteral("MODEL"));
    navigatorPanel->setMinimumWidth(220);
    navigatorPanel->setMaximumWidth(320);

    auto *graphicsHost = new QWidget;
    graphicsHost->setObjectName(QStringLiteral("Dynamics26GraphicsHost"));
    auto *grid = new QGridLayout(graphicsHost);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(0);
    viewport->setParent(graphicsHost);
    grid->addWidget(viewport, 0, 0);

    auto *overlay = new QFrame(graphicsHost);
    overlay->setObjectName(QStringLiteral("Dynamics26GraphicsOverlay"));
    overlay->setFrameShape(QFrame::StyledPanel);
    overlay->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    auto *overlayLayout = new QHBoxLayout(overlay);
    overlayLayout->setContentsMargins(7, 3, 4, 3);
    overlayLayout->setSpacing(5);
    auto *context = new QLabel(QStringLiteral("Model"), overlay);
    context->setObjectName(QStringLiteral("Dynamics26GraphicsContext"));
    context->setForegroundRole(QPalette::PlaceholderText);
    overlayLayout->addWidget(context);
    if (fitAction != nullptr) {
        auto *fit = new QToolButton(overlay);
        fit->setDefaultAction(fitAction);
        fit->setAutoRaise(true);
        fit->setToolButtonStyle(Qt::ToolButtonIconOnly);
        overlayLayout->addWidget(fit);
    }
    grid->addWidget(overlay, 0, 0, Qt::AlignTop | Qt::AlignLeft);

    navigatorPanel->setParent(nullptr);
    details.panel->setParent(nullptr);
    graphicsHost->setParent(nullptr);
    auto *workspace = new QSplitter(Qt::Horizontal, &window);
    workspace->setObjectName(QStringLiteral("Dynamics26WorkspaceSplitterV2"));
    workspace->setHandleWidth(1);
    workspace->setChildrenCollapsible(false);
    workspace->addWidget(navigatorPanel);
    workspace->addWidget(graphicsHost);
    workspace->addWidget(details.panel);
    workspace->setStretchFactor(0, 0);
    workspace->setStretchFactor(1, 1);
    workspace->setStretchFactor(2, 0);
    workspace->setSizes({240, 900, 320});
    window.setCentralWidget(workspace);
    oldWorkspace->deleteLater();

    QObject::connect(tree, &QTreeWidget::currentItemChanged, graphicsHost,
                     [context](QTreeWidgetItem *current, QTreeWidgetItem *) { context->setText(contextTitle(contextFor(current))); });
    return graphicsHost;
}

QString normalizedResultName(const QString &source)
{
    if (source.contains(QStringLiteral("Mesh max |u|"), Qt::CaseInsensitive)) return QStringLiteral("Toplam Deformasyon");
    if (source.contains(QStringLiteral("Mesh max von Mises"), Qt::CaseInsensitive)) return QStringLiteral("Eşdeğer Gerilme");
    if (source.contains(QStringLiteral("ΣRx"), Qt::CaseInsensitive)) return QStringLiteral("Reaksiyon Kuvveti");
    if (source.contains(QStringLiteral("Probe Node"), Qt::CaseInsensitive)) return QStringLiteral("Probe Düğümü");
    if (source.contains(QStringLiteral("Probe ux"), Qt::CaseInsensitive)) return QStringLiteral("Probe Deplasmanı X");
    return source;
}

int utilityTabIndex(QTabWidget *tabs, const QString &needle)
{
    if (tabs == nullptr) return -1;
    for (int i=0;i<tabs->count();++i) if (tabs->tabText(i).contains(needle, Qt::CaseInsensitive)) return i;
    return -1;
}

void normalizeResultTable(QTabWidget *tabs)
{
    const int resultTab = utilityTabIndex(tabs, QStringLiteral("Sonuç"));
    auto *table = resultTab >= 0 ? qobject_cast<QTableWidget *>(tabs->widget(resultTab)) : nullptr;
    if (table == nullptr) return;
    if (auto *header = table->horizontalHeader()) { header->setSectionResizeMode(0, QHeaderView::ResizeToContents); header->setSectionResizeMode(1, QHeaderView::Stretch); }
    if (auto *vertical = table->verticalHeader()) vertical->hide();
    for (int row=0;row<table->rowCount();++row) if (auto *item=table->item(row,0)) item->setText(normalizedResultName(item->text()));
}

void syncSolutionTree(QTreeWidget *tree, QTabWidget *tabs)
{
    auto *solution = findContextItem(tree, WorkbenchContext::Results, QStringLiteral("Çözüm"));
    const int resultTab = utilityTabIndex(tabs, QStringLiteral("Sonuç"));
    auto *table = resultTab >= 0 ? qobject_cast<QTableWidget *>(tabs->widget(resultTab)) : nullptr;
    if (solution == nullptr || table == nullptr) return;
    while (solution->childCount() > 0) delete solution->takeChild(0);
    QSet<QString> names;
    for (int row=0;row<table->rowCount();++row) {
        auto *item=table->item(row,0); if (item==nullptr || item->text().trimmed().isEmpty()) continue;
        const QString name=normalizedResultName(item->text().trimmed()); if (names.contains(name)) continue;
        names.insert(name); auto *child=new QTreeWidgetItem(solution,{name});
        child->setData(0,kInspectorRole,kOpenResults); child->setData(0,kContextRole,static_cast<int>(WorkbenchContext::Results));
    }
    solution->setExpanded(!names.isEmpty());
}

QAction *makeAction(QMainWindow &window, const QString &text, const QString &tip, QStyle::StandardPixmap icon)
{
    auto *action=new QAction(text,&window); action->setToolTip(tip); if(window.style())action->setIcon(window.style()->standardIcon(icon)); return action;
}

void configureGlobalToolbar(QMainWindow &window, QTreeWidget *tree, GeometryPanel *geometry, PrePostPanel *prePost,
                            const DetailsUi &details)
{
    auto *toolbar=window.findChild<QToolBar *>(QStringLiteral("Dynamics26MainToolbar")); if(toolbar==nullptr)return;
    QAction *navigatorAction=actionContaining(window,QStringLiteral("Proje Gezgini"));
    QAction *fitAction=actionContaining(window,QStringLiteral("Görünüme Sığdır"));
    QAction *diagnosticsAction=actionContaining(window,QStringLiteral("Sonuçlar ve Tanılama"));
    QAction *inspectorAction=actionContaining(window,QStringLiteral("Özellikler"));

    auto *importAction=makeAction(window,QStringLiteral("İçe Aktar"),QStringLiteral("STEP / STP geometri içe aktar"),QStyle::SP_DialogOpenButton);
    QObject::connect(importAction,&QAction::triggered,&window,[geometry,tree]{if(auto*i=findContextItem(tree,WorkbenchContext::Geometry))tree->setCurrentItem(i);if(geometry)QMetaObject::invokeMethod(geometry,"importStep",Qt::QueuedConnection);});
    auto *meshAction=makeAction(window,QStringLiteral("Mesh Oluştur"),QStringLiteral("Structured HEX8 mesh oluştur"),QStyle::SP_ComputerIcon);
    QObject::connect(meshAction,&QAction::triggered,&window,[prePost,tree]{if(auto*i=findContextItem(tree,WorkbenchContext::Mesh))tree->setCurrentItem(i);if(prePost)QMetaObject::invokeMethod(prePost,"generateMesh",Qt::QueuedConnection);});
    auto *solveAction=makeAction(window,QStringLiteral("Çöz"),QStringLiteral("Statik Yapısal 1 çöz"),QStyle::SP_MediaPlay);
    const auto refreshSolve=[solveAction,details]{const bool linear=details.analysisType==nullptr||details.analysisType->currentIndex()==0;solveAction->setEnabled(linear);solveAction->setToolTip(linear?QStringLiteral("Statik Yapısal 1 çöz"):QStringLiteral("Bu analiz türü Alpha.1 GUI solve akışına henüz bağlı değil."));};
    refreshSolve();
    if(details.analysisType)QObject::connect(details.analysisType,qOverload<int>(&QComboBox::currentIndexChanged),&window,[refreshSolve](int){refreshSolve();});
    QObject::connect(solveAction,&QAction::triggered,&window,[prePost,tree]{if(auto*i=findContextItem(tree,WorkbenchContext::Analysis,QStringLiteral("Statik Yapısal 1")))tree->setCurrentItem(i);if(prePost)QMetaObject::invokeMethod(prePost,"solveLinear",Qt::QueuedConnection);});

    toolbar->clear(); toolbar->setIconSize(QSize(18,18)); toolbar->setMinimumHeight(38); toolbar->setMaximumHeight(42);
    if(navigatorAction)toolbar->addAction(navigatorAction); toolbar->addSeparator();
    auto addTextButton=[toolbar](QAction *action){auto*b=new QToolButton(toolbar);b->setDefaultAction(action);b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);b->setAutoRaise(true);toolbar->addWidget(b);};
    addTextButton(importAction); addTextButton(meshAction); if(fitAction)toolbar->addAction(fitAction);
    auto *spacer=new QWidget(toolbar);spacer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);toolbar->addWidget(spacer);
    auto *solveButton=new QToolButton(toolbar);solveButton->setDefaultAction(solveAction);solveButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);solveButton->setAutoRaise(false);toolbar->addWidget(solveButton);
    toolbar->addSeparator();if(diagnosticsAction)toolbar->addAction(diagnosticsAction);if(inspectorAction)toolbar->addAction(inspectorAction);
}

void configureUtilityWorkspace(QMainWindow &window, PrePostPanel *prePost, QTreeWidget *tree, const DetailsUi &details)
{
    auto *dock=window.findChild<QDockWidget *>(QStringLiteral("Dynamics26UtilityArea"));
    auto *tabs=dock?dock->findChild<QTabWidget *>(QStringLiteral("Dynamics26UtilityTabs")):nullptr;
    if(!dock||!tabs)return;
    if(tabs->count()>0)tabs->setTabText(0,QStringLiteral("Sonuçlar"));
    if(tabs->count()>1)tabs->setTabText(1,QStringLiteral("Yakınsama"));
    if(tabs->count()>2)tabs->setTabText(2,QStringLiteral("Mesajlar / Solver"));
    auto *minimalTitle=new QWidget(dock);minimalTitle->setFixedHeight(1);dock->setTitleBarWidget(minimalTitle);dock->hide();QTimer::singleShot(0,dock,[dock]{dock->hide();});
    auto *status=window.statusBar();if(!status)return;status->clearMessage();status->setSizeGripEnabled(false);status->setMinimumHeight(22);status->setMaximumHeight(24);status->showMessage(QStringLiteral("Hazır"));status->show();
    auto *diagnostics=status->findChild<QToolButton *>(QStringLiteral("Dynamics26DiagnosticsHandle"));if(!diagnostics){diagnostics=new QToolButton(status);diagnostics->setObjectName(QStringLiteral("Dynamics26DiagnosticsHandle"));diagnostics->setText(QStringLiteral("Tanılama"));diagnostics->setCheckable(true);diagnostics->setAutoRaise(true);status->addPermanentWidget(diagnostics);}diagnostics->setChecked(false);QObject::connect(diagnostics,&QToolButton::toggled,dock,&QWidget::setVisible);QObject::connect(dock,&QDockWidget::visibilityChanged,diagnostics,&QToolButton::setChecked);
    normalizeResultTable(tabs);syncSolutionTree(tree,tabs);
    if(prePost){QObject::connect(prePost,&PrePostPanel::message,status,[status,details,prePost,dock,tabs](const QString &message){refreshMeshDetails(details,prePost);static const QRegularExpression p(QStringLiteral("structured mesh oluşturuldu: (\\d+) node, (\\d+) HEX8"),QRegularExpression::CaseInsensitiveOption);const auto m=p.match(message);if(m.hasMatch()){status->showMessage(QStringLiteral("%1 düğüm • %2 HEX8 • Hazır").arg(m.captured(1),m.captured(2)));return;}if(message.contains(QStringLiteral("başarısız"),Qt::CaseInsensitive)||message.contains(QStringLiteral("hata"),Qt::CaseInsensitive)){status->showMessage(QStringLiteral("Hata • %1").arg(message));if(dock&&tabs){const int i=utilityTabIndex(tabs,QStringLiteral("Mesaj"));if(i>=0)tabs->setCurrentIndex(i);dock->show();}}});
        QObject::connect(prePost,&PrePostPanel::solveCompleted,status,[status,tabs,tree,details](double maxU,double maxVm,double,qlonglong,double){status->showMessage(QStringLiteral("Çözüm tamamlandı • umax %1 mm • von Mises %2 MPa").arg(maxU,0,'g',5).arg(maxVm,0,'g',5));normalizeResultTable(tabs);syncSolutionTree(tree,tabs);if(details.resultStatus){details.resultStatus->setText(QStringLiteral("Güncel"));details.resultU->setText(QStringLiteral("%1 mm").arg(maxU,0,'g',6));details.resultVm->setText(QStringLiteral("%1 MPa").arg(maxVm,0,'g',6));}});}
}

void connectWorkbenchContext(QMainWindow &window,QTreeWidget *tree,GeometryPanel *geometry,PrePostPanel *prePost,const DetailsUi &details)
{
    if(!tree||!details.pages)return;
    QObject::connect(tree,&QTreeWidget::currentItemChanged,&window,[geometry,prePost,details](QTreeWidgetItem *current,QTreeWidgetItem *){const auto context=contextFor(current);details.objectTitle->setText(current?current->text(0):QStringLiteral("Model"));details.pages->setCurrentIndex(pageForContext(context));if(context==WorkbenchContext::Geometry){refreshGeometryDetails(details,geometry);if(geometry&&geometry->showCurrentGeometry())return;}if(context==WorkbenchContext::Mesh)refreshMeshDetails(details,prePost);if(context==WorkbenchContext::Results){if(prePost&&!prePost->showResultsPreview())prePost->showMeshPreview();return;}if(prePost&&prePost->showMeshPreview())return;if(geometry)geometry->showCurrentGeometry();});
    if(geometry)QObject::connect(geometry,&GeometryPanel::message,&window,[details,geometry](const QString &){refreshGeometryDetails(details,geometry);});
    if(details.analysisType){if(auto *legacy=findAnalysisTypeCombo(window)){details.analysisType->setCurrentIndex(legacy->currentIndex());QObject::connect(details.analysisType,qOverload<int>(&QComboBox::currentIndexChanged),legacy,[legacy,details](int index){legacy->setCurrentIndex(index);details.analysisStatus->setText(index==0?QStringLiteral("Çözülebilir"):QStringLiteral("GUI solve akışı henüz etkin değil"));});}}
}

QComboBox *findAnalysisTypeCombo(QMainWindow &window)
{
    for(auto *combo:window.findChildren<QComboBox *>()){const bool linear=combo->findText(QStringLiteral("Linear Static"))>=0||combo->findText(QStringLiteral("Lineer Statik"))>=0;const bool modal=combo->findText(QStringLiteral("Modal / Free Vibration"))>=0||combo->findText(QStringLiteral("Modal / Serbest Titreşim"))>=0;if(linear&&modal)return combo;}return nullptr;
}

} // namespace

namespace dynamics26::gui {

void installCaeWorkbenchController(QMainWindow &window)
{
    auto *tree=window.findChild<QTreeWidget *>(QStringLiteral("Dynamics26Navigator"));
    auto *geometry=window.findChild<GeometryPanel *>();
    auto *prePost=window.findChild<PrePostPanel *>();
    configureModelTree(window,tree);
    DetailsUi details=buildDetailsSurface(window,geometry,prePost);
    QAction *fitAction=actionContaining(window,QStringLiteral("Görünüme Sığdır"));
    recomposeGraphicsFirstWorkspace(window,tree,details,fitAction);
    configureGlobalToolbar(window,tree,geometry,prePost,details);
    configureUtilityWorkspace(window,prePost,tree,details);
    connectWorkbenchContext(window,tree,geometry,prePost,details);
    if(auto *geometryItem=findContextItem(tree,WorkbenchContext::Geometry))tree->setCurrentItem(geometryItem);
}

} // namespace dynamics26::gui
