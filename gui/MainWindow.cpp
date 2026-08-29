#include "MainWindow.h"
#include "ProjectFileMigrator.h"
#include "ViewportWidget.h"
#include "MaterialCurveWidget.h"
#include "GeometryPanel.h"
#include "PrePostPanel.h"

#include <femcae/femcae.h>

#include <cmath>

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVector>
#include <QPointF>
#include <QSplitter>
#include <QStatusBar>
#include <QSpinBox>
#include <QTabWidget>
#include <QTimer>
#include <QTableWidget>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {
QDoubleSpinBox *makeSpin(double value, double minimum, double maximum, int decimals, const QString &suffix)
{
    auto *spin = new QDoubleSpinBox;
    spin->setRange(minimum, maximum);
    spin->setDecimals(decimals);
    spin->setValue(value);
    spin->setSuffix(suffix);
    spin->setSingleStep(qMax(0.001, value * 0.05));
    return spin;
}
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(tr("FEMCAE — CAD / Mesh / Linear / Modal / Nonlinear / Contact"));
    resize(1440, 900);

    auto *newAction = new QAction(tr("Yeni"), this);
    auto *openAction = new QAction(tr("Aç"), this);
    auto *saveAction = new QAction(tr("Kaydet"), this);
    auto *solveAction = new QAction(tr("Lineer Analiz"), this);
    auto *modalAction = new QAction(tr("Modal Analiz"), this);
    auto *nonlinearAction = new QAction(tr("Nonlinear Static"), this);
    auto *fitAction = new QAction(tr("Görünümü Sığdır"), this);
    connect(newAction, &QAction::triggered, this, &MainWindow::createNewProject);
    connect(openAction, &QAction::triggered, this, &MainWindow::openProject);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveProject);
    connect(solveAction, &QAction::triggered, this, &MainWindow::runLinearDemo);
    connect(modalAction, &QAction::triggered, this, &MainWindow::runModalDemo);
    connect(nonlinearAction, &QAction::triggered, this, &MainWindow::runNonlinearDemo);
    connect(fitAction, &QAction::triggered, this, &MainWindow::resetView);

    auto *fileMenu = menuBar()->addMenu(tr("Dosya"));
    fileMenu->addAction(newAction);
    fileMenu->addAction(openAction);
    fileMenu->addAction(saveAction);
    auto *analysisMenu = menuBar()->addMenu(tr("Analiz"));
    analysisMenu->addAction(solveAction);
    analysisMenu->addAction(modalAction);
    analysisMenu->addAction(nonlinearAction);

    auto *toolbar = addToolBar(tr("Ana Araçlar"));
    toolbar->setMovable(false);
    toolbar->addAction(newAction);
    toolbar->addAction(openAction);
    toolbar->addAction(saveAction);
    toolbar->addSeparator();
    toolbar->addAction(solveAction);
    toolbar->addAction(modalAction);
    toolbar->addAction(nonlinearAction);
    toolbar->addAction(fitAction);

    auto *splitter = new QSplitter(this);
    modelTree_ = new QTreeWidget(splitter);
    modelTree_->setHeaderHidden(true);
    modelTree_->setMinimumWidth(230);
    buildModelTree();

    viewport_ = new ViewportWidget(splitter);
    auto *inspector = createInspector();
    inspector->setMinimumWidth(300);
    splitter->addWidget(inspector);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    setCentralWidget(splitter);

    auto *bottomDock = new QDockWidget(tr("Sonuçlar ve Çözüm Günlüğü"), this);
    bottomDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    auto *bottomTabs = new QTabWidget(bottomDock);
    results_ = new QTableWidget(0, 2, bottomTabs);
    results_->setHorizontalHeaderLabels({tr("Sonuç"), tr("Değer")});
    results_->horizontalHeader()->setStretchLastSection(true);
    convergenceHistory_ = new QTableWidget(0, 7, bottomTabs);
    convergenceHistory_->setHorizontalHeaderLabels({tr("Attempt"), tr("Iter"), tr("Load Factor"), tr("Rel. R"), tr("Rel. Δu"), tr("α"), tr("Durum")});
    convergenceHistory_->horizontalHeader()->setStretchLastSection(true);
    log_ = new QPlainTextEdit(bottomTabs);
    log_->setReadOnly(true);
    bottomTabs->addTab(results_, tr("Sonuçlar"));
    bottomTabs->addTab(convergenceHistory_, tr("Nonlinear Yakınsama"));
    bottomTabs->addTab(log_, tr("Log"));
    bottomDock->setWidget(bottomTabs);
    addDockWidget(Qt::BottomDockWidgetArea, bottomDock);

    statusBar()->showMessage(tr("FEMCAE Engine %1.%2.%3  •  C API %4")
        .arg(fem_version_major()).arg(fem_version_minor()).arg(fem_version_patch()).arg(fem_api_version()));

    modalTimer_ = new QTimer(this);
    modalTimer_->setInterval(40);
    connect(modalTimer_, &QTimer::timeout, this, &MainWindow::animateModalMode);

    applyMacStyle();
    appendLog(tr("FEMCAE Verified Engineering Release GUI başlatıldı. CAD geometry/display tessellation/FEM mesh ayrımı korunur; Mesh / Pre-Post sekmesi structured HEX8 → assignment → solve → result/export akışını sağlar."));
}

QWidget *MainWindow::createInspector()
{
    auto *tabs = new QTabWidget;
    geometryPanel_ = new GeometryPanel(tabs);
    geometryPanel_->setTessellationConsumer([this](const femcae::geometry::GeometryTessellation &tessellation) {
        if (viewport_ != nullptr) viewport_->showGeometryTessellation(tessellation);
    });
    connect(geometryPanel_, &GeometryPanel::message, this, [this](const QString &text) { appendLog(text); });
    tabs->addTab(geometryPanel_, tr("Geometri"));
    prePostPanel_ = new PrePostPanel(tabs);
    prePostPanel_->setViewportConsumer([this](const femcae::meshing::SimulationMesh &mesh, const femcae::meshing::ResultDatabase &results) {
        if (viewport_ != nullptr) viewport_->showSimulationMeshResult(mesh, results);
    });
    connect(prePostPanel_, &PrePostPanel::message, this, [this](const QString &text) { appendLog(text); });
    connect(prePostPanel_, &PrePostPanel::solveCompleted, this, [this](double maxUmm, double maxVmMPa, double reactionX, qlonglong probeNode, double probeUxMm) {
        results_->setRowCount(5);
        const QString names[5] = {tr("Mesh max |u|"), tr("Mesh max von Mises"), tr("ΣRx"), tr("Probe Node"), tr("Probe ux")};
        const QString vals[5] = {QString::number(maxUmm,'g',10)+" mm", QString::number(maxVmMPa,'g',10)+" MPa", QString::number(reactionX,'g',10)+" N", QString::number(probeNode), QString::number(probeUxMm,'g',10)+" mm"};
        for (int i=0;i<5;++i) { results_->setItem(i,0,new QTableWidgetItem(names[i])); results_->setItem(i,1,new QTableWidgetItem(vals[i])); }
    });
    tabs->addTab(prePostPanel_, tr("Mesh / Pre-Post"));
    tabs->addTab(createMaterialEditor(), tr("Malzeme"));
    tabs->addTab(createSectionEditor(), tr("Kesit"));
    tabs->addTab(createLoadBcEditor(), tr("Yük / BC"));
    tabs->addTab(createAnalysisEditor(), tr("Analiz"));
    return tabs;
}

QWidget *MainWindow::createMaterialEditor()
{
    auto *page = new QWidget;
    auto *outer = new QVBoxLayout(page);
    auto *layout = new QFormLayout;
    materialModel_ = new QComboBox(page);
    materialModel_->addItems({tr("Linear Elastic — Isotropic"), tr("Neo-Hookean"), tr("Mooney-Rivlin"), tr("Yeoh"), tr("Ogden")});
    youngGPa_ = makeSpin(210.0, 0.001, 10000.0, 3, tr(" GPa"));
    poisson_ = makeSpin(0.30, -0.99, 0.4999, 4, QString());
    densityKgM3_ = makeSpin(7850.0, 0.001, 1.0e8, 3, tr(" kg/m³"));
    bulkMPa_ = makeSpin(2000.0, 0.001, 1.0e7, 3, tr(" MPa"));
    c10MPa_ = makeSpin(1.0, 0.0, 1.0e6, 6, tr(" MPa"));
    c01MPa_ = makeSpin(0.25, 0.0, 1.0e6, 6, tr(" MPa"));
    c20MPa_ = makeSpin(0.10, -1.0e6, 1.0e6, 6, tr(" MPa"));
    c30MPa_ = makeSpin(0.01, -1.0e6, 1.0e6, 6, tr(" MPa"));
    ogdenTerms_ = new QSpinBox(page); ogdenTerms_->setRange(1,3); ogdenTerms_->setValue(2);
    for (int i=0; i<3; ++i) {
        ogdenMuMPa_[i] = makeSpin(i==0 ? 1.5 : (i==1 ? 0.5 : 0.1), -1.0e6, 1.0e6, 6, tr(" MPa"));
        ogdenAlpha_[i] = makeSpin(i==0 ? 2.0 : (i==1 ? -2.0 : 4.0), -100.0, 100.0, 5, QString());
    }
    layout->addRow(tr("Model"), materialModel_);
    layout->addRow(tr("Young Modülü"), youngGPa_);
    layout->addRow(tr("Poisson"), poisson_);
    layout->addRow(tr("Yoğunluk"), densityKgM3_);
    layout->addRow(tr("Bulk Modulus K"), bulkMPa_);
    layout->addRow(tr("C10"), c10MPa_);
    layout->addRow(tr("C01"), c01MPa_);
    layout->addRow(tr("C20"), c20MPa_);
    layout->addRow(tr("C30"), c30MPa_);
    layout->addRow(tr("Ogden Terim Sayısı"), ogdenTerms_);
    for (int i=0; i<3; ++i) {
        layout->addRow(tr("Ogden μ%1").arg(i+1), ogdenMuMPa_[i]);
        layout->addRow(tr("Ogden α%1").arg(i+1), ogdenAlpha_[i]);
    }
    auto *preview = new QPushButton(tr("Malzeme Eğrisini Önizle"), page);
    materialValidation_ = new QLabel(tr("Lineer izotropik malzeme seçili."), page);
    materialValidation_->setWordWrap(true);
    materialCurve_ = new MaterialCurveWidget(page);
    layout->addRow(preview);
    layout->addRow(materialValidation_);
    outer->addLayout(layout);
    outer->addWidget(materialCurve_);
    auto *note = new QLabel(tr("Hyperelastic önizleme: J=1 kabulüyle isochoric uniaxial nominal stress–stretch eğrisi. "
                               "K penalty parametresi mixed u-p formülasyonundan ayrıdır. StVK yalnız verification malzemesi olarak kalır."), page);
    note->setWordWrap(true);
    outer->addWidget(note);
    connect(materialModel_, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::materialModelChanged);
    connect(ogdenTerms_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int){ materialModelChanged(materialModel_->currentIndex()); });
    connect(preview, &QPushButton::clicked, this, &MainWindow::runMaterialPreview);
    materialModelChanged(0);
    return page;
}


void MainWindow::materialModelChanged(int index)
{
    const bool linear = index == 0;
    youngGPa_->setEnabled(linear); poisson_->setEnabled(linear);
    bulkMPa_->setEnabled(!linear);
    c10MPa_->setEnabled(index >= 1 && index <= 3);
    c01MPa_->setEnabled(index == 2);
    c20MPa_->setEnabled(index == 3);
    c30MPa_->setEnabled(index == 3);
    ogdenTerms_->setEnabled(index == 4);
    for (int i=0; i<3; ++i) {
        const bool enabled = index == 4 && i < ogdenTerms_->value();
        ogdenMuMPa_[i]->setEnabled(enabled);
        ogdenAlpha_[i]->setEnabled(enabled);
    }
    if (linear) {
        materialValidation_->setText(tr("Lineer izotropik malzeme: E/ν. Hyperelastic curve preview yalnız nonlinear modeller için çalışır."));
        materialCurve_->clearCurve();
    } else {
        materialValidation_->setText(tr("Parametreleri kontrol etmek için Malzeme Eğrisini Önizle düğmesini kullanın."));
    }
}

void MainWindow::runMaterialPreview()
{
    const int model = materialModel_->currentIndex();
    if (model == 0) {
        materialValidation_->setText(tr("Lineer model için hyperelastic preview uygulanmaz."));
        materialCurve_->clearCurve();
        return;
    }
    std::array<double, 6> params {};
    int count = 0;
    if (model == 1) {
        params[0] = c10MPa_->value()*1.0e6; count = 1;
    } else if (model == 2) {
        params[0] = c10MPa_->value()*1.0e6; params[1] = c01MPa_->value()*1.0e6; count = 2;
    } else if (model == 3) {
        params[0] = c10MPa_->value()*1.0e6; params[1] = c20MPa_->value()*1.0e6; params[2] = c30MPa_->value()*1.0e6; count = 3;
    } else {
        count = 2*ogdenTerms_->value();
        for (int i=0; i<ogdenTerms_->value(); ++i) {
            params[2*i] = ogdenMuMPa_[i]->value()*1.0e6;
            params[2*i+1] = ogdenAlpha_[i]->value();
        }
    }
    const double bulk = bulkMPa_->value()*1.0e6;
    double g0 = 0.0;
    const int validation = fem_hyperelastic_validate(model, bulk, count, params.data(), &g0);
    if (validation != 0) {
        materialValidation_->setText(tr("Engine parametre doğrulaması başarısız. Status: %1").arg(validation));
        materialCurve_->clearCurve();
        return;
    }
    QVector<QPointF> points;
    for (int i=0; i<=40; ++i) {
        const double stretch = 1.0 + 0.025*i;
        double nominal = 0.0, energy = 0.0;
        const int rc = fem_hyperelastic_isochoric_uniaxial_preview(model, bulk, count, params.data(), stretch, &nominal, &energy);
        if (rc != 0) {
            materialValidation_->setText(tr("Preview hesaplaması başarısız. Status: %1").arg(rc));
            materialCurve_->clearCurve();
            return;
        }
        points.push_back(QPointF(stretch, nominal/1.0e6));
    }
    materialCurve_->setCurve(points, tr("Stretch λ"), tr("Nominal Stress [MPa]"));
    materialValidation_->setText(tr("Engine validation: OK  •  Initial shear modulus G₀ = %1 MPa").arg(g0/1.0e6, 0, 'g', 8));
    appendLog(tr("Material Studio preview güncellendi: %1, G0=%2 MPa").arg(materialModel_->currentText()).arg(g0/1.0e6,0,'g',8));
}

QWidget *MainWindow::createSectionEditor()
{
    auto *page = new QWidget;
    auto *layout = new QFormLayout(page);
    auto *type = new QComboBox(page);
    type->addItems({tr("Truss / Bar"), tr("Plane Thickness"), tr("2B Beam")});
    areaMm2_ = makeSpin(100.0, 0.001, 1.0e9, 3, tr(" mm²"));
    lengthMm_ = makeSpin(1000.0, 0.001, 1.0e9, 3, tr(" mm"));
    layout->addRow(tr("Kesit Tipi"), type);
    layout->addRow(tr("Alan"), areaMm2_);
    layout->addRow(tr("Demo Uzunluğu"), lengthMm_);
    return page;
}

QWidget *MainWindow::createLoadBcEditor()
{
    auto *page = new QWidget;
    auto *layout = new QFormLayout(page);
    forceN_ = makeSpin(1000.0, -1.0e12, 1.0e12, 3, tr(" N"));
    auto *bc = new QComboBox(page);
    bc->addItems({tr("Sol uç sabit"), tr("Nodal prescribed displacement")});
    auto *run = new QPushButton(tr("Lineer Demo Analizini Çalıştır"), page);
    solveSummary_ = new QLabel(tr("Henüz çözüm yok."), page);
    solveSummary_->setWordWrap(true);
    connect(run, &QPushButton::clicked, this, &MainWindow::runLinearDemo);
    layout->addRow(tr("Yük"), forceN_);
    layout->addRow(tr("Sınır Şartı"), bc);
    layout->addRow(run);
    layout->addRow(solveSummary_);
    return page;
}


QWidget *MainWindow::createAnalysisEditor()
{
    auto *page = new QWidget;
    auto *layout = new QFormLayout(page);
    auto *analysisType = new QComboBox(page);
    analysisType->addItems({tr("Linear Static"), tr("Modal / Free Vibration"), tr("Nonlinear Static / Large Displacement")});

    auto *runModal = new QPushButton(tr("Modal Demo Analizini Çalıştır"), page);
    connect(runModal, &QPushButton::clicked, this, &MainWindow::runModalDemo);

    modeSelector_ = new QComboBox(page);
    modeSelector_->addItems({tr("Mode 1"), tr("Mode 2")});
    modeSelector_->setEnabled(false);
    connect(modeSelector_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::modalSelectionChanged);

    modalSummary_ = new QLabel(tr("Henüz modal çözüm yok."), page);
    modalSummary_->setWordWrap(true);

    nonlinearFormulation_ = new QComboBox(page);
    nonlinearFormulation_->addItems({tr("Displacement-only / penalty"), tr("Mixed u-p HEX8/P0 verification"), tr("Contact / Friction verification")});
    nonlinearMethod_ = new QComboBox(page);
    nonlinearMethod_->addItems({tr("Full Newton-Raphson"), tr("Modified Newton")});
    mixedShearGamma_ = makeSpin(0.12, -0.80, 0.80, 4, QString());
    contactEnforcement_ = new QComboBox(page);
    contactEnforcement_->addItems({tr("Penalty"), tr("Augmented Lagrangian")});
    contactPenaltyFactor_ = makeSpin(100.0, 1.0, 100000.0, 1, QString());
    nonlinearInitialIncrement_ = makeSpin(0.25, 0.0001, 1.0, 4, QString());
    nonlinearMinimumIncrement_ = makeSpin(0.01, 0.000001, 1.0, 6, QString());
    nonlinearMaximumIncrement_ = makeSpin(0.50, 0.0001, 1.0, 4, QString());
    nonlinearMaxIterations_ = new QSpinBox(page);
    nonlinearMaxIterations_->setRange(1, 200);
    nonlinearMaxIterations_->setValue(25);
    nonlinearLineSearch_ = new QCheckBox(tr("Backtracking line search"), page);
    nonlinearLineSearch_->setChecked(true);
    nonlinearAdaptive_ = new QCheckBox(tr("Adaptive increment / cutback"), page);
    nonlinearAdaptive_->setChecked(true);
    auto *runNonlinear = new QPushButton(tr("Nonlinear Large-Displacement Demo Çalıştır"), page);
    connect(runNonlinear, &QPushButton::clicked, this, &MainWindow::runNonlinearDemo);
    nonlinearSummary_ = new QLabel(tr("Henüz nonlinear çözüm yok."), page);
    nonlinearSummary_->setWordWrap(true);

    auto *scope = new QLabel(tr("V1.0 solver çekirdeği: displacement-only, mixed u-p ve contact/friction alt sistemleri birbirinden ayrıdır; CAD/geometry katmanı solver meshinden bağımsızdır. "
                                "Mixed çözümde element-associated P0 pressure DOF, block-aware convergence ve symmetric-indefinite direct solver kullanılır.\n"
                                "GUI mixed seçimi şu an manufactured simple-shear verification preset'idir; arbitrary mixed mesh preprocessor değildir."), page);
    scope->setWordWrap(true);
    layout->addRow(tr("Analiz Türü"), analysisType);
    layout->addRow(runModal);
    layout->addRow(tr("Gösterilecek Mod"), modeSelector_);
    layout->addRow(modalSummary_);
    layout->addRow(new QLabel(tr("Nonlinear Static / Large Displacement"), page));
    layout->addRow(tr("Formulation"), nonlinearFormulation_);
    layout->addRow(tr("Mixed Shear γ"), mixedShearGamma_);
    layout->addRow(tr("Contact Enforcement"), contactEnforcement_);
    layout->addRow(tr("Contact k_n / E (unit cube preset)"), contactPenaltyFactor_);
    layout->addRow(tr("Newton Yöntemi"), nonlinearMethod_);
    layout->addRow(tr("İlk Load Increment"), nonlinearInitialIncrement_);
    layout->addRow(tr("Minimum Increment"), nonlinearMinimumIncrement_);
    layout->addRow(tr("Maximum Increment"), nonlinearMaximumIncrement_);
    layout->addRow(tr("Max Iteration"), nonlinearMaxIterations_);
    layout->addRow(nonlinearLineSearch_);
    layout->addRow(nonlinearAdaptive_);
    layout->addRow(runNonlinear);
    layout->addRow(nonlinearSummary_);
    layout->addRow(scope);
    return page;
}

void MainWindow::buildModelTree()
{
    modelTree_->clear();
    const QStringList roots = {tr("Model"), tr("Malzemeler"), tr("Kesitler"), tr("Yükler"), tr("Sınır Şartları"), tr("Mesh"), tr("Analiz"), tr("Sonuçlar")};
    for (const auto &name : roots) {
        auto *item = new QTreeWidgetItem(modelTree_, {name});
        if (name == tr("Model")) {
            new QTreeWidgetItem(item, {tr("CAD Geometry")});
            new QTreeWidgetItem(item, {tr("FEM Düğümler / Elemanlar")});
        } else if (name == tr("Mesh")) {
            new QTreeWidgetItem(item, {tr("Structured HEX8 / External C3D8")});
            new QTreeWidgetItem(item, {tr("Quality / Provenance / Local Sizing")});
        } else if (name == tr("Sonuçlar")) {
            new QTreeWidgetItem(item, {tr("Displacement")});
            new QTreeWidgetItem(item, {tr("von Mises")});
            new QTreeWidgetItem(item, {tr("Probe / Section Cut / Export")});
        } else if (name == tr("Analiz")) {
            new QTreeWidgetItem(item, {tr("Linear Static")});
            new QTreeWidgetItem(item, {tr("Modal / Free Vibration")});
            auto *nl = new QTreeWidgetItem(item, {tr("Nonlinear Static / Large Displacement")});
            new QTreeWidgetItem(nl, {tr("Displacement-only / penalty")});
            new QTreeWidgetItem(nl, {tr("Mixed u-p HEX8/P0")});
        }
    }
    modelTree_->expandAll();
}

void MainWindow::runLinearDemo()
{
    if (modalTimer_ != nullptr) { modalTimer_->stop(); }
    modalReady_ = false;
    if (modeSelector_ != nullptr) { modeSelector_->setEnabled(false); }
    const double e = youngGPa_->value() * 1.0e9;
    const double area = areaMm2_->value() * 1.0e-6;
    const double length = lengthMm_->value() * 1.0e-3;
    const double force = forceN_->value();
    double displacement = 0.0;
    double stress = 0.0;
    double reaction = 0.0;
    const int rc = fem_demo_axial_bar(e, area, length, force, &displacement, &stress, &reaction);
    if (rc != 0) {
        solveSummary_->setText(tr("Çözüm başarısız. Engine status: %1").arg(rc));
        appendLog(tr("Axial bar demo başarısız: status=%1").arg(rc));
        return;
    }

    solveSummary_->setText(tr("Tip deplasman: %1 mm\nAxial stress: %2 MPa\nMesnet reaksiyonu: %3 N")
        .arg(displacement * 1000.0, 0, 'g', 10).arg(stress / 1.0e6, 0, 'g', 10).arg(reaction, 0, 'g', 10));
    results_->setRowCount(3);
    results_->setItem(0, 0, new QTableWidgetItem(tr("Tip Deplasman")));
    results_->setItem(0, 1, new QTableWidgetItem(QString::number(displacement * 1000.0, 'g', 10) + " mm"));
    results_->setItem(1, 0, new QTableWidgetItem(tr("Axial Stress")));
    results_->setItem(1, 1, new QTableWidgetItem(QString::number(stress / 1.0e6, 'g', 10) + " MPa"));
    results_->setItem(2, 0, new QTableWidgetItem(tr("Mesnet Reaksiyonu")));
    results_->setItem(2, 1, new QTableWidgetItem(QString::number(reaction, 'g', 10) + " N"));
    viewport_->showAxialBarResult(length, displacement, stress);
    updateResultTree(displacement * 1000.0, stress / 1.0e6, reaction);
    appendLog(tr("Linear demo çözüldü: E=%1 GPa, A=%2 mm², L=%3 mm, F=%4 N")
        .arg(youngGPa_->value()).arg(areaMm2_->value()).arg(lengthMm_->value()).arg(force));
}


void MainWindow::runModalDemo()
{
    const double e = youngGPa_->value() * 1.0e9;
    const double rho = densityKgM3_->value();
    const double area = areaMm2_->value() * 1.0e-6;
    const double length = lengthMm_->value() * 1.0e-3;

    double f1 = 0.0;
    double f2 = 0.0;
    double mid1 = 0.0;
    double tip1 = 0.0;
    double mid2 = 0.0;
    double tip2 = 0.0;
    const int rc = fem_demo_axial_modal(e, rho, area, length,
        &f1, &f2, &mid1, &tip1, &mid2, &tip2);
    if (rc != 0) {
        modalReady_ = false;
        modeSelector_->setEnabled(false);
        modalTimer_->stop();
        modalSummary_->setText(tr("Modal çözüm başarısız. Engine status: %1").arg(rc));
        appendLog(tr("Axial modal demo başarısız: status=%1").arg(rc));
        return;
    }

    modalFrequenciesHz_ = {f1, f2};
    modalMid_ = {mid1, mid2};
    modalTip_ = {tip1, tip2};
    modalReady_ = true;
    modalPhase_ = 0.0;
    modeSelector_->setEnabled(true);

    modalSummary_->setText(tr("Mode 1: %1 Hz\nMode 2: %2 Hz\nMode shape'lar M-normalized; viewport görsel ölçeği normalize edilir.")
        .arg(f1, 0, 'g', 10).arg(f2, 0, 'g', 10));

    results_->setRowCount(2);
    results_->setItem(0, 0, new QTableWidgetItem(tr("Mode 1 Frekansı")));
    results_->setItem(0, 1, new QTableWidgetItem(QString::number(f1, 'g', 10) + " Hz"));
    results_->setItem(1, 0, new QTableWidgetItem(tr("Mode 2 Frekansı")));
    results_->setItem(1, 1, new QTableWidgetItem(QString::number(f2, 'g', 10) + " Hz"));

    updateModalResultTree();
    showSelectedModalFrame(0.0);
    modalTimer_->start();
    appendLog(tr("Modal demo çözüldü: E=%1 GPa, ρ=%2 kg/m³, A=%3 mm², L=%4 mm; f1=%5 Hz, f2=%6 Hz")
        .arg(youngGPa_->value()).arg(rho).arg(areaMm2_->value()).arg(lengthMm_->value())
        .arg(f1, 0, 'g', 8).arg(f2, 0, 'g', 8));
}


void MainWindow::runNonlinearDemo()
{
    if (modalTimer_ != nullptr) { modalTimer_->stop(); }
    modalReady_ = false;
    if (modeSelector_ != nullptr) { modeSelector_->setEnabled(false); }

    if (nonlinearFormulation_ != nullptr && nonlinearFormulation_->currentIndex() == 2) {
        const double e = youngGPa_->value() * 1.0e9;
        const double nu = poisson_->value();
        const double kn = e * contactPenaltyFactor_->value();
        const double compressionForce = std::abs(forceN_->value());
        const int enforcement = contactEnforcement_->currentIndex() + 1;
        double penetration = 0.0;
        double normalForce = 0.0;
        int activeContacts = 0;
        int iterations = 0;
        const int rc = fem_demo_contact_hex8(e, nu, kn, compressionForce, enforcement,
                                              &penetration, &normalForce, &activeContacts, &iterations);
        if (rc != 0) {
            nonlinearSummary_->setText(tr("Contact doğrulama çözümü başarısız. Engine status: %1").arg(rc));
            appendLog(tr("Rigid-master contact demo başarısız: status=%1").arg(rc));
            return;
        }
        nonlinearSummary_->setText(tr("Rigid-Master Contact
Enforcement: %1
Aktif contact: %2
Max penetration: %3 mm
Normal contact force: %4 N
Newton corrections: %5")
            .arg(contactEnforcement_->currentText()).arg(activeContacts).arg(penetration*1000.0,0,'g',8)
            .arg(normalForce,0,'g',10).arg(iterations));
        results_->setRowCount(5);
        const QStringList names = {tr("Enforcement"), tr("Active Contact Points"), tr("Maximum Penetration"),
                                   tr("Total Normal Contact Force"), tr("Newton Corrections")};
        const QStringList values = {contactEnforcement_->currentText(), QString::number(activeContacts),
                                    QString::number(penetration*1000.0,'g',10)+" mm",
                                    QString::number(normalForce,'g',10)+" N", QString::number(iterations)};
        for (int row=0; row<names.size(); ++row) {
            results_->setItem(row,0,new QTableWidgetItem(names[row]));
            results_->setItem(row,1,new QTableWidgetItem(values[row]));
        }
        convergenceHistory_->setRowCount(0);
        viewport_->showContactHex8Result(penetration);
        appendLog(tr("Contact verification çözüldü: enforcement=%1, active=%2, penetration=%3 mm, N=%4 N")
            .arg(contactEnforcement_->currentText()).arg(activeContacts).arg(penetration*1000.0,0,'g',8).arg(normalForce,0,'g',10));
        return;
    }

    if (nonlinearFormulation_ != nullptr && nonlinearFormulation_->currentIndex() == 1) {
        const double c10 = c10MPa_->value() * 1.0e6;
        const double bulk = bulkMPa_->value() * 1.0e6;
        const double requestedGamma = mixedShearGamma_->value();
        double recoveredGamma = 0.0;
        double pressure = 0.0;
        double loadFactor = 0.0;
        double pressureResidual = 0.0;
        int iterations = 0;
        const int rc = fem_demo_mixed_up_hex8_shear(c10, bulk, requestedGamma, &recoveredGamma, &pressure,
                                                     &loadFactor, &pressureResidual, &iterations);
        if (rc != 0) {
            nonlinearSummary_->setText(tr("Mixed u-p doğrulama çözümü başarısız. Engine status: %1").arg(rc));
            appendLog(tr("Mixed u-p HEX8/P0 demo başarısız: status=%1").arg(rc));
            return;
        }
        nonlinearSummary_->setText(tr("Mixed u-p HEX8/P0\nİstenen γ: %1\nÇözülen γ: %2\nElement pressure: %3 MPa\nPressure residual: %4\nNewton corrections: %5")
            .arg(requestedGamma,0,'g',8).arg(recoveredGamma,0,'g',8).arg(pressure/1.0e6,0,'g',8)
            .arg(pressureResidual,0,'g',8).arg(iterations));
        results_->setRowCount(5);
        const QStringList names = {tr("Requested shear γ"), tr("Recovered shear γ"), tr("Element P0 Pressure"),
                                   tr("Pressure Residual Norm"), tr("Completed Load Factor")};
        const QStringList values = {QString::number(requestedGamma,'g',10), QString::number(recoveredGamma,'g',10),
                                    QString::number(pressure/1.0e6,'g',10)+" MPa", QString::number(pressureResidual,'g',10),
                                    QString::number(loadFactor,'g',10)};
        for (int row=0; row<names.size(); ++row) {
            results_->setItem(row,0,new QTableWidgetItem(names[row]));
            results_->setItem(row,1,new QTableWidgetItem(values[row]));
        }
        convergenceHistory_->setRowCount(0);
        viewport_->showMixedShearHex8Result(recoveredGamma);
        appendLog(tr("Mixed u-p verification çözüldü: γ=%1, p=%2 MPa, pressure residual=%3, corrections=%4")
            .arg(recoveredGamma,0,'g',8).arg(pressure/1.0e6,0,'g',8).arg(pressureResidual,0,'g',8).arg(iterations));
        return;
    }

    const double e = youngGPa_->value() * 1.0e9;
    const double nu = poisson_->value();
    const double area = areaMm2_->value() * 1.0e-6;
    const double length = lengthMm_->value() * 1.0e-3;
    const double force = forceN_->value();
    const double initialIncrement = nonlinearInitialIncrement_->value();
    const double minimumIncrement = nonlinearMinimumIncrement_->value();
    const double maximumIncrement = nonlinearMaximumIncrement_->value();
    const int method = nonlinearMethod_->currentIndex() + 1;
    const int lineSearch = nonlinearLineSearch_->isChecked() ? 1 : 0;
    const int adaptive = nonlinearAdaptive_->isChecked() ? 1 : 0;
    const int maxIterations = nonlinearMaxIterations_->value();

    constexpr int historyCapacity = 512;
    std::array<int, historyCapacity> attempts {};
    std::array<int, historyCapacity> iterations {};
    std::array<int, historyCapacity> converged {};
    std::array<double, historyCapacity> loadFactors {};
    std::array<double, historyCapacity> relativeResiduals {};
    std::array<double, historyCapacity> relativeDisplacements {};
    std::array<double, historyCapacity> alphas {};
    double displacement = 0.0;
    double completedLoadFactor = 0.0;
    double finalResidualNorm = 0.0;
    int acceptedSteps = 0;
    int totalIterations = 0;
    int cutbacks = 0;
    int historyCount = 0;

    const int rc = fem_demo_nonlinear_hex8(e, nu, area, length, force, initialIncrement,
        minimumIncrement, maximumIncrement, method, lineSearch, maxIterations, adaptive, &displacement, &completedLoadFactor,
        &finalResidualNorm, &acceptedSteps, &totalIterations, &cutbacks,
        historyCapacity, &historyCount, attempts.data(), iterations.data(), loadFactors.data(),
        relativeResiduals.data(), relativeDisplacements.data(), alphas.data(), converged.data());
    if (rc != 0) {
        nonlinearSummary_->setText(tr("Nonlinear çözüm başarısız. Engine status: %1").arg(rc));
        appendLog(tr("Nonlinear HEX8 demo başarısız: status=%1").arg(rc));
        return;
    }

    nonlinearSummary_->setText(tr("Tip deplasman: %1 mm\nLoad factor: %2\nAccepted step: %3\nNewton correction: %4\nCutback: %5\nFinal |R|: %6")
        .arg(displacement * 1000.0, 0, 'g', 10)
        .arg(completedLoadFactor, 0, 'g', 8)
        .arg(acceptedSteps).arg(totalIterations).arg(cutbacks)
        .arg(finalResidualNorm, 0, 'g', 8));

    results_->setRowCount(6);
    const QStringList names = {tr("Tip Deplasman"), tr("Completed Load Factor"), tr("Accepted Steps"),
                               tr("Newton Corrections"), tr("Cutbacks"), tr("Final Residual Norm")};
    const QStringList values = {QString::number(displacement * 1000.0, 'g', 10) + " mm",
                                QString::number(completedLoadFactor, 'g', 10), QString::number(acceptedSteps),
                                QString::number(totalIterations), QString::number(cutbacks),
                                QString::number(finalResidualNorm, 'g', 10)};
    for (int row = 0; row < names.size(); ++row) {
        results_->setItem(row, 0, new QTableWidgetItem(names[row]));
        results_->setItem(row, 1, new QTableWidgetItem(values[row]));
    }

    convergenceHistory_->setRowCount(historyCount);
    for (int row = 0; row < historyCount; ++row) {
        convergenceHistory_->setItem(row, 0, new QTableWidgetItem(QString::number(attempts[row])));
        convergenceHistory_->setItem(row, 1, new QTableWidgetItem(QString::number(iterations[row])));
        convergenceHistory_->setItem(row, 2, new QTableWidgetItem(QString::number(loadFactors[row], 'g', 8)));
        convergenceHistory_->setItem(row, 3, new QTableWidgetItem(QString::number(relativeResiduals[row], 'g', 6)));
        convergenceHistory_->setItem(row, 4, new QTableWidgetItem(QString::number(relativeDisplacements[row], 'g', 6)));
        convergenceHistory_->setItem(row, 5, new QTableWidgetItem(QString::number(alphas[row], 'g', 6)));
        convergenceHistory_->setItem(row, 6, new QTableWidgetItem(converged[row] ? tr("Converged") : tr("Iterating")));
    }

    viewport_->showNonlinearHex8Result(length, area, displacement);
    updateNonlinearResultTree(displacement * 1000.0, completedLoadFactor, acceptedSteps, totalIterations, cutbacks);
    appendLog(tr("Nonlinear demo çözüldü: method=%1, Δλ0=%2, steps=%3, corrections=%4, cutbacks=%5, history=%6")
        .arg(nonlinearMethod_->currentText()).arg(initialIncrement).arg(acceptedSteps)
        .arg(totalIterations).arg(cutbacks).arg(historyCount));
}

void MainWindow::animateModalMode()
{
    if (!modalReady_) {
        return;
    }
    modalPhase_ += 0.12;
    if (modalPhase_ > 6.283185307179586) {
        modalPhase_ -= 6.283185307179586;
    }
    showSelectedModalFrame(std::sin(modalPhase_));
}

void MainWindow::modalSelectionChanged(int index)
{
    Q_UNUSED(index)
    if (modalReady_) {
        modalPhase_ = 0.0;
        showSelectedModalFrame(0.0);
    }
}

void MainWindow::showSelectedModalFrame(double phase)
{
    if (!modalReady_) {
        return;
    }
    const int index = qBound(0, modeSelector_->currentIndex(), 1);
    const double scale = qMax(std::abs(modalMid_[index]), std::abs(modalTip_[index]));
    const double mid = scale > 0.0 ? modalMid_[index] / scale : 0.0;
    const double tip = scale > 0.0 ? modalTip_[index] / scale : 0.0;
    viewport_->showAxialBarMode(lengthMm_->value() * 1.0e-3, mid, tip, phase);
}

void MainWindow::updateModalResultTree()
{
    for (int i = 0; i < modelTree_->topLevelItemCount(); ++i) {
        auto *root = modelTree_->topLevelItem(i);
        if (root->text(0) != tr("Sonuçlar")) {
            continue;
        }
        while (root->childCount() > 0) {
            delete root->takeChild(0);
        }
        new QTreeWidgetItem(root, {tr("Mode 1: %1 Hz").arg(modalFrequenciesHz_[0], 0, 'g', 8)});
        new QTreeWidgetItem(root, {tr("Mode 2: %1 Hz").arg(modalFrequenciesHz_[1], 0, 'g', 8)});
        root->setExpanded(true);
        break;
    }
}



void MainWindow::updateNonlinearResultTree(double displacementMm, double loadFactor, int steps, int iterations, int cutbacks)
{
    for (int i = 0; i < modelTree_->topLevelItemCount(); ++i) {
        auto *root = modelTree_->topLevelItem(i);
        if (root->text(0) != tr("Sonuçlar")) {
            continue;
        }
        while (root->childCount() > 0) {
            delete root->takeChild(0);
        }
        auto *nonlinear = new QTreeWidgetItem(root, {tr("Nonlinear Static")});
        new QTreeWidgetItem(nonlinear, {tr("Tip Deplasman: %1 mm").arg(displacementMm, 0, 'g', 8)});
        new QTreeWidgetItem(nonlinear, {tr("Load Factor: %1").arg(loadFactor, 0, 'g', 8)});
        new QTreeWidgetItem(nonlinear, {tr("Accepted Steps: %1").arg(steps)});
        new QTreeWidgetItem(nonlinear, {tr("Newton Corrections: %1").arg(iterations)});
        new QTreeWidgetItem(nonlinear, {tr("Cutbacks: %1").arg(cutbacks)});
        nonlinear->setExpanded(true);
        root->setExpanded(true);
        break;
    }
}

void MainWindow::updateResultTree(double displacementMm, double stressMPA, double reactionN)
{
    for (int i = 0; i < modelTree_->topLevelItemCount(); ++i) {
        auto *root = modelTree_->topLevelItem(i);
        if (root->text(0) != tr("Sonuçlar")) {
            continue;
        }
        while (root->childCount() > 0) {
            delete root->takeChild(0);
        }
        new QTreeWidgetItem(root, {tr("Tip Deplasman: %1 mm").arg(displacementMm, 0, 'g', 8)});
        new QTreeWidgetItem(root, {tr("Axial Stress: %1 MPa").arg(stressMPA, 0, 'g', 8)});
        new QTreeWidgetItem(root, {tr("Mesnet Reaksiyonu: %1 N").arg(reactionN, 0, 'g', 8)});
        root->setExpanded(true);
        break;
    }
}

void MainWindow::resetView()
{
    viewport_->resetCamera();
    appendLog(tr("Viewport kamera sıfırlandı."));
}

void MainWindow::createNewProject()
{
    buildModelTree();
    if (geometryPanel_ != nullptr) { geometryPanel_->clearProject(); }
    if (prePostPanel_ != nullptr) { prePostPanel_->clearProject(); }
    if (modalTimer_ != nullptr) { modalTimer_->stop(); }
    modalReady_ = false;
    if (modeSelector_ != nullptr) { modeSelector_->setEnabled(false); }
    results_->setRowCount(0);
    if (convergenceHistory_ != nullptr) { convergenceHistory_->setRowCount(0); }
    solveSummary_->setText(tr("Henüz çözüm yok."));
    if (modalSummary_ != nullptr) { modalSummary_->setText(tr("Henüz modal çözüm yok.")); }
    if (nonlinearSummary_ != nullptr) { nonlinearSummary_->setText(tr("Henüz nonlinear çözüm yok.")); }
    appendLog(tr("Yeni boş proje görünümü oluşturuldu."));
}


void MainWindow::openProject()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("FEMCAE Projesi Aç"), QString(),
        tr("FEMCAE Project (*.femcae.json);;JSON (*.json)"));
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        appendLog(tr("Proje açılamadı: %1").arg(path));
        return;
    }
    constexpr qint64 kMaximumProjectBytes = 16 * 1024 * 1024;
    if (file.size() < 2 || file.size() > kMaximumProjectBytes) {
        appendLog(tr("Proje dosyası boş veya izin verilen boyut sınırını aşıyor: %1").arg(path));
        return;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        appendLog(tr("Proje JSON bozuk: %1 (offset %2)").arg(parseError.errorString()).arg(parseError.offset));
        return;
    }
    const auto migration = femcae::gui::ProjectFileMigrator::migrate(
        document.object(), fem_project_schema_version());
    if (!migration.ok) {
        appendLog(tr("Proje schema doğrulama/migration hatası: %1").arg(migration.message));
        return;
    }
    if (migration.migrated) {
        appendLog(tr("Proje migration: %1").arg(migration.message));
    }
    const QJsonObject root = migration.project;
    const QJsonObject material = root.value("material").toObject();
    const QJsonObject section = root.value("section").toObject();
    const QJsonObject load = root.value("load").toObject();
    const QJsonObject nonlinear = root.value("nonlinear").toObject();
    const QJsonObject geometry = root.value("geometry").toObject();
    const QJsonObject prepost = root.value("prepost").toObject();
    youngGPa_->setValue(material.value("young_gpa").toDouble(210.0));
    poisson_->setValue(material.value("poisson").toDouble(0.30));
    densityKgM3_->setValue(material.value("density_kg_m3").toDouble(7850.0));
    materialModel_->setCurrentIndex(qBound(0, material.value("studio_model_index").toInt(0), 4));
    bulkMPa_->setValue(material.value("bulk_mpa").toDouble(2000.0));
    c10MPa_->setValue(material.value("c10_mpa").toDouble(1.0));
    c01MPa_->setValue(material.value("c01_mpa").toDouble(0.25));
    c20MPa_->setValue(material.value("c20_mpa").toDouble(0.10));
    c30MPa_->setValue(material.value("c30_mpa").toDouble(0.01));
    ogdenTerms_->setValue(qBound(1, material.value("ogden_terms").toInt(2), 3));
    for (int i=0; i<3; ++i) {
        ogdenMuMPa_[i]->setValue(material.value(QString("ogden_mu%1_mpa").arg(i+1)).toDouble(i==0?1.5:(i==1?0.5:0.1)));
        ogdenAlpha_[i]->setValue(material.value(QString("ogden_alpha%1").arg(i+1)).toDouble(i==0?2.0:(i==1?-2.0:4.0)));
    }
    materialModelChanged(materialModel_->currentIndex());
    areaMm2_->setValue(section.value("area_mm2").toDouble(100.0));
    lengthMm_->setValue(section.value("length_mm").toDouble(1000.0));
    forceN_->setValue(load.value("force_n").toDouble(1000.0));
    nonlinearInitialIncrement_->setValue(nonlinear.value("initial_increment").toDouble(0.25));
    nonlinearMinimumIncrement_->setValue(nonlinear.value("minimum_increment").toDouble(0.01));
    nonlinearMaximumIncrement_->setValue(nonlinear.value("maximum_increment").toDouble(0.50));
    nonlinearMaxIterations_->setValue(nonlinear.value("max_iterations").toInt(25));
    nonlinearMethod_->setCurrentIndex(qBound(0, nonlinear.value("method").toInt(1)-1, 1));
    nonlinearLineSearch_->setChecked(nonlinear.value("line_search").toBool(true));
    nonlinearAdaptive_->setChecked(nonlinear.value("adaptive_stepping").toBool(true));
    if (geometryPanel_ != nullptr) { geometryPanel_->loadProjectJson(geometry); }
    if (prePostPanel_ != nullptr) { prePostPanel_->loadProjectJson(prepost); }
    appendLog(tr("Proje açıldı: %1").arg(path));
}

void MainWindow::saveProject()
{
    QString path = QFileDialog::getSaveFileName(this, tr("FEMCAE Projesini Kaydet"), "linear-demo.femcae.json",
        tr("FEMCAE Project (*.femcae.json);;JSON (*.json)"));
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(".json", Qt::CaseInsensitive)) {
        path += ".femcae.json";
    }
    QJsonObject material;
    material["model"] = "linear_isotropic";
    material["young_gpa"] = youngGPa_->value();
    material["poisson"] = poisson_->value();
    material["density_kg_m3"] = densityKgM3_->value();
    material["studio_model_index"] = materialModel_->currentIndex();
    material["bulk_mpa"] = bulkMPa_->value();
    material["c10_mpa"] = c10MPa_->value();
    material["c01_mpa"] = c01MPa_->value();
    material["c20_mpa"] = c20MPa_->value();
    material["c30_mpa"] = c30MPa_->value();
    material["ogden_terms"] = ogdenTerms_->value();
    for (int i=0; i<3; ++i) {
        material[QString("ogden_mu%1_mpa").arg(i+1)] = ogdenMuMPa_[i]->value();
        material[QString("ogden_alpha%1").arg(i+1)] = ogdenAlpha_[i]->value();
    }
    QJsonObject section;
    section["type"] = "truss";
    section["area_mm2"] = areaMm2_->value();
    section["length_mm"] = lengthMm_->value();
    QJsonObject load;
    load["force_n"] = forceN_->value();
    QJsonObject nonlinear;
    nonlinear["method"] = nonlinearMethod_->currentIndex() + 1;
    nonlinear["initial_increment"] = nonlinearInitialIncrement_->value();
    nonlinear["minimum_increment"] = nonlinearMinimumIncrement_->value();
    nonlinear["maximum_increment"] = nonlinearMaximumIncrement_->value();
    nonlinear["max_iterations"] = nonlinearMaxIterations_->value();
    nonlinear["line_search"] = nonlinearLineSearch_->isChecked();
    nonlinear["adaptive_stepping"] = nonlinearAdaptive_->isChecked();
    QJsonObject root;
    root["application_version"] = QString("%1.%2.%3").arg(fem_version_major()).arg(fem_version_minor()).arg(fem_version_patch());
    root["project_schema"] = fem_project_schema_version();
    root["analysis"] = "linear_static_axial_demo";
    root["material"] = material;
    root["section"] = section;
    root["load"] = load;
    root["nonlinear"] = nonlinear;
    if (geometryPanel_ != nullptr) { root["geometry"] = geometryPanel_->projectJson(); }
    if (prePostPanel_ != nullptr) { root["prepost"] = prePostPanel_->projectJson(); }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        appendLog(tr("Proje kaydedilemedi: %1").arg(path));
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    appendLog(tr("Proje kaydedildi: %1").arg(path));
}

void MainWindow::appendLog(const QString &message)
{
    if (log_ != nullptr) {
        log_->appendPlainText(message);
    }
}

void MainWindow::applyMacStyle()
{
    setStyleSheet(R"(
        QMainWindow { background: #f5f5f7; }
        QToolBar { background: #fbfbfd; border: 0; border-bottom: 1px solid #d7d7dc; spacing: 6px; padding: 5px; }
        QTreeWidget, QTabWidget::pane, QPlainTextEdit, QTableWidget { background: #ffffff; border: 1px solid #d7d7dc; }
        QTreeWidget { outline: 0; }
        QTreeWidget::item { padding: 4px 6px; }
        QTreeWidget::item:selected { background: #e8f1ff; color: #111111; }
        QTabBar::tab { padding: 7px 12px; margin: 0; }
        QPushButton { padding: 7px 12px; border: 1px solid #c7c7cc; border-radius: 5px; background: #ffffff; }
        QPushButton:pressed { background: #ededf0; }
        QDoubleSpinBox, QComboBox { min-height: 25px; border: 1px solid #c7c7cc; border-radius: 4px; padding: 2px 6px; background: #ffffff; }
        QLabel#viewportPlaceholder { color: #68686d; background: #f7f7f9; border: 1px solid #d7d7dc; }
        QStatusBar { border-top: 1px solid #d7d7dc; background: #fbfbfd; }
    )");
}
