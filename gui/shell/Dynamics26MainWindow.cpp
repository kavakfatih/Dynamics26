#include "Dynamics26MainWindow.h"

#include "../commands/ContactCommands.h"
#include "../commands/DomainCommands.h"
#include "../commands/NamedSelectionCommands.h"
#include "../core/CaeIcons.h"
#include "../core/DependencyEngine.h"
#include "../core/DocumentCommandManager.h"
#include "../core/ProjectModel.h"
#include "../core/UiTheme.h"
#include "../details/BoundaryConditionDetails.h"
#include "../details/GeometryDetails.h"
#include "../details/ResultDetails.h"
#include "../services/AnalysisService.h"
#include "../services/ContactService.h"
#include "../services/GeometryService.h"
#include "../services/MaterialService.h"
#include "../services/MeshService.h"
#include "../services/NamedSelectionService.h"
#include "CommandRegistry.h"
#include "DetailsHost.h"
#include "EngineeringStatusBar.h"
#include "GraphicsWorkspace.h"
#include "ProjectNavigator.h"
#include "../ProjectFileMigrator.h"

#include <femcae/femcae.h>

#include <QAction>
#include <QApplication>
#include <QElapsedTimer>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QMimeData>
#include <QOperatingSystemVersion>
#include <QPlainTextEdit>
#include <QSettings>
#include <QSplitter>
#include <QSysInfo>
#include <QToolBar>
#include <QUndoStack>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QFontDatabase>

#include <array>
#include <cmath>
#include <exception>
#include <vector>

namespace d26 {
namespace {

// Parametrik kutu gövdesinin GÖRÜNTÜLEME üçgenlemesi.
// Bu veri yalnız ekran içindir: solver elemanı veya CAD B-Rep değildir.
femcae::geometry::GeometryTessellation makeBoxDisplayTessellation(const double lengthMm, const double widthMm,
                                                                  const double heightMm)
{
    femcae::geometry::GeometryTessellation tessellation;
    const double lx = lengthMm * 1.0e-3;
    const double ly = widthMm * 1.0e-3;
    const double lz = heightMm * 1.0e-3;
    tessellation.points = {{0, 0, 0},   {lx, 0, 0},   {lx, ly, 0},   {0, ly, 0},
                           {0, 0, lz},  {lx, 0, lz},  {lx, ly, lz},  {0, ly, lz}};
    const std::array<std::array<std::uint32_t, 3>, 12> faces{{{0, 3, 2}, {0, 2, 1},
                                                              {4, 5, 6}, {4, 6, 7},
                                                              {0, 1, 5}, {0, 5, 4},
                                                              {1, 2, 6}, {1, 6, 5},
                                                              {2, 3, 7}, {2, 7, 6},
                                                              {3, 0, 4}, {3, 4, 7}}};
    tessellation.triangles.assign(faces.begin(), faces.end());
    return tessellation;
}

// Kutu yüzünün dışa doğru birim normali; yük oku ve mesnet sembolü yönü için.
void outwardNormal(const BoxFace face, double &x, double &y, double &z)
{
    x = y = z = 0.0;
    switch (face) {
    case BoxFace::XMin: x = -1.0; break;
    case BoxFace::XMax: x = 1.0; break;
    case BoxFace::YMin: y = -1.0; break;
    case BoxFace::YMax: y = 1.0; break;
    case BoxFace::ZMin: z = -1.0; break;
    case BoxFace::ZMax: z = 1.0; break;
    }
}

// Gerçek FEM boundary facet provenance'ından yüzün MODEL İÇİNE bakan birim
// normalini çıkarır. Named Selection CAD Face kimliği BoxFace olmak zorunda
// değildir; persistent consumer görselleştirmesi legacy eksen enum'una dönmez.
bool inwardBoundaryNormal(const femcae::meshing::SimulationMesh &mesh,
                          const femcae::geometry::GeometryEntityId geometryId,
                          double &x, double &y, double &z)
{
    x = y = z = 0.0;
    for (const auto &facet : mesh.boundaryFacets) {
        if (facet.sourceGeometryId != geometryId) {
            continue;
        }
        const auto *n0 = mesh.findNode(facet.nodeIds[0]);
        const auto *n1 = mesh.findNode(facet.nodeIds[1]);
        const auto *n2 = mesh.findNode(facet.nodeIds[2]);
        if (n0 == nullptr || n1 == nullptr || n2 == nullptr) {
            return false;
        }

        const double ax = n1->x.x - n0->x.x;
        const double ay = n1->x.y - n0->x.y;
        const double az = n1->x.z - n0->x.z;
        const double bx = n2->x.x - n0->x.x;
        const double by = n2->x.y - n0->x.y;
        const double bz = n2->x.z - n0->x.z;
        double nx = ay * bz - az * by;
        double ny = az * bx - ax * bz;
        double nz = ax * by - ay * bx;
        double norm = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (norm <= 1.0e-14) {
            return false;
        }

        double fx = 0.0;
        double fy = 0.0;
        double fz = 0.0;
        int facetNodeCount = 0;
        for (const auto nodeId : facet.nodeIds) {
            const auto *node = mesh.findNode(nodeId);
            if (node == nullptr) {
                continue;
            }
            fx += node->x.x;
            fy += node->x.y;
            fz += node->x.z;
            ++facetNodeCount;
        }
        const auto *owner = mesh.findElement(facet.ownerElementId);
        if (facetNodeCount == 0 || owner == nullptr) {
            return false;
        }
        fx /= facetNodeCount;
        fy /= facetNodeCount;
        fz /= facetNodeCount;

        double ex = 0.0;
        double ey = 0.0;
        double ez = 0.0;
        int elementNodeCount = 0;
        for (const auto nodeId : owner->nodeIds) {
            const auto *node = mesh.findNode(nodeId);
            if (node == nullptr) {
                continue;
            }
            ex += node->x.x;
            ey += node->x.y;
            ez += node->x.z;
            ++elementNodeCount;
        }
        if (elementNodeCount == 0) {
            return false;
        }
        ex /= elementNodeCount;
        ey /= elementNodeCount;
        ez /= elementNodeCount;

        const double inwardX = ex - fx;
        const double inwardY = ey - fy;
        const double inwardZ = ez - fz;
        if (nx * inwardX + ny * inwardY + nz * inwardZ < 0.0) {
            nx = -nx;
            ny = -ny;
            nz = -nz;
        }
        norm = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (norm <= 1.0e-14) {
            return false;
        }
        x = nx / norm;
        y = ny / norm;
        z = nz / norm;
        return true;
    }
    return false;
}

femcae::geometry::GeometryEntityId singleBoundaryHighlight(const BoundaryScopeResolution &resolution)
{
    if (!resolution.valid || resolution.geometryFaceIds.size() != 1) {
        return femcae::geometry::InvalidGeometryId;
    }
    return resolution.geometryFaceIds.front();
}

QString contextTitleFor(const ViewportContext context)
{
    switch (context) {
    case ViewportContext::Geometry:    return QStringLiteral("Geometry");
    case ViewportContext::Materials:   return QStringLiteral("Materials");
    case ViewportContext::Connections: return QStringLiteral("Connections");
    case ViewportContext::Mesh:        return QStringLiteral("Mesh");
    case ViewportContext::Loads:       return QStringLiteral("Loads / Boundary Conditions");
    case ViewportContext::Analysis:    return QStringLiteral("Analysis");
    case ViewportContext::Results:     return QStringLiteral("Results");
    case ViewportContext::Modal:       return QStringLiteral("Modal");
    case ViewportContext::Empty:       break;
    }
    return QStringLiteral("Model");
}

bool parseDocumentObjectId(const QJsonValue &value, ObjectId *result)
{
    if (result == nullptr) {
        return false;
    }
    if (value.isString()) {
        bool ok = false;
        const qulonglong parsed = value.toString().toULongLong(&ok, 10);
        if (!ok) {
            return false;
        }
        *result = static_cast<ObjectId>(parsed);
        return true;
    }
    // V1.1 eski dosyaları next_object_id alanını JSON number olarak yazıyordu.
    // Küçük legacy kimlikler için bu yol korunur; yeni dosyalar tam 64-bit
    // hassasiyet için decimal string kullanır.
    if (value.isDouble()) {
        const qint64 parsed = value.toInteger(-1);
        if (parsed < 0) {
            return false;
        }
        *result = static_cast<ObjectId>(parsed);
        return true;
    }
    return false;
}

} // namespace

Dynamics26MainWindow::Dynamics26MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setObjectName(QStringLiteral("Dynamics26MainWindow"));
    setWindowTitle(QStringLiteral("Dynamics26"));
    resize(1560, 960);

    buildServices();
    buildCommands();
    buildLayout();
    buildCommandSurface();
    buildMenus();
    wireSignals();

    // Başlangıç modeli: parametrik kutu gövdesi + varsayılan malzeme +
    // bir Static Structural analizi. Hepsi gerçek nesnedir; boş bir kabuk değil.
    rebuildGeometryNodes();
    materials_->resetToDefault();
    activeAnalysis_ = analysis_->createAnalysis(AnalysisType::StaticStructural);
    // Başlangıç kurulumu bir kullanıcı düzenlemesi değildir: doküman temiz başlar.
    documentCommands_->resetHistory();

    reportMessage(tr("Dynamics26 %1 — solver çekirdeği %2.%3.%4, C ABI %5")
                      .arg(QStringLiteral(DYNAMICS26_GUI_MILESTONE))
                      .arg(fem_version_major())
                      .arg(fem_version_minor())
                      .arg(fem_version_patch())
                      .arg(fem_api_version()),
                  Severity::Info);

    navigator_->expandAll();
    selectObject(project_->geometryNode());
    rebuildRecentMenu();
    updateWindowTitle();
    syncAll();
}

void Dynamics26MainWindow::buildServices()
{
    project_ = new ProjectModel(this);
    geometry_ = new GeometryService(this);
    mesh_ = new MeshService(geometry_, this);
    contacts_ = new ContactService(project_, geometry_, mesh_, this);
    materials_ = new MaterialService(project_, this);
    analysis_ = new AnalysisService(project_, mesh_, materials_, this);
    auto *namedSelections = new NamedSelectionService(project_, geometry_, mesh_, this);

    services_.project = project_;
    services_.geometry = geometry_;
    services_.mesh = mesh_;
    services_.namedSelections = namedSelections;
    services_.contacts = contacts_;
    services_.materials = materials_;
    services_.analysis = analysis_;

    // Doküman komut sistemi ve bağımlılık motoru servislerin ÜSTÜNDE durur:
    // servisler bunları tanımaz, kabuk ve komutlar kullanır. Persistent scope
    // servisleri bu noktada hazırdır; DetailsHost ve alt Details sayfaları dahil
    // ServiceContext kopyası alan tüm consumer'lar aynı örnekleri görür.
    documentCommands_ = new DocumentCommandManager(this);
    dependencies_ = new DependencyEngine(services_, this);
    services_.commands = documentCommands_;
    services_.dependencies = dependencies_;
}

void Dynamics26MainWindow::buildLayout()
{
    navigator_ = new ProjectNavigator(project_, this);
    graphics_ = new GraphicsWorkspace(this);
    details_ = new DetailsHost(services_, this);
    utility_ = new UtilityWorkspace(this);

    workspaceSplitter_ = new QSplitter(Qt::Horizontal, this);
    workspaceSplitter_->setObjectName(QStringLiteral("Dynamics26WorkspaceSplitter"));
    workspaceSplitter_->setChildrenCollapsible(false);
    workspaceSplitter_->setHandleWidth(1);
    workspaceSplitter_->addWidget(navigator_);
    workspaceSplitter_->addWidget(graphics_);
    workspaceSplitter_->addWidget(details_);
    // 3B grafik alanı görsel olarak baskın kalmalıdır: ~%60.
    workspaceSplitter_->setStretchFactor(0, 0);
    workspaceSplitter_->setStretchFactor(1, 1);
    workspaceSplitter_->setStretchFactor(2, 0);
    workspaceSplitter_->setSizes({300, 940, 320});

    verticalSplitter_ = new QSplitter(Qt::Vertical, this);
    verticalSplitter_->setObjectName(QStringLiteral("Dynamics26VerticalSplitter"));
    verticalSplitter_->setChildrenCollapsible(false);
    verticalSplitter_->setHandleWidth(1);
    verticalSplitter_->addWidget(workspaceSplitter_);
    verticalSplitter_->addWidget(utility_);
    verticalSplitter_->setStretchFactor(0, 1);
    verticalSplitter_->setStretchFactor(1, 0);
    // Alt yardımcı alan başlangıçta kapalıdır (§19).
    utility_->setVisible(false);
    setCentralWidget(verticalSplitter_);

    engineeringStatus_ = new EngineeringStatusBar(this);
    setStatusBar(engineeringStatus_);
}

void Dynamics26MainWindow::buildCommands()
{
    commands_ = new CommandRegistry(this);
    commands_->add(QStringLiteral("file.new"), tr("Yeni"), CommandGlyph::New, QStringLiteral("Ctrl+N"));
    commands_->add(QStringLiteral("file.open"), tr("Aç"), CommandGlyph::Open, QStringLiteral("Ctrl+O"));
    commands_->add(QStringLiteral("file.save"), tr("Kaydet"), CommandGlyph::Save, QStringLiteral("Ctrl+S"));

    commands_->add(QStringLiteral("geometry.import"), tr("Import Geometry"), CommandGlyph::ImportGeometry,
                   QString(), tr("STEP / STP CAD gövdesi içe aktar"));
    commands_->add(QStringLiteral("geometry.replace"), tr("Replace"), CommandGlyph::ReplaceGeometry,
                   QString(), tr("Mevcut geometriyi yeni bir STEP dosyasıyla değiştir"));
    commands_->add(QStringLiteral("geometry.importSection"), tr("Import Section"), CommandGlyph::NamedSelection,
                   QString(), tr("DXF kesit profili içe aktar"));

    commands_->add(QStringLiteral("mesh.generate"), tr("Generate Mesh"), CommandGlyph::GenerateMesh,
                   QStringLiteral("F7"), tr("Structured HEX8 mesh üret"));
    auto *nodes = commands_->add(QStringLiteral("mesh.showNodes"), tr("Nodes"), CommandGlyph::SelectVertex,
                                 QString(), tr("FEM düğümlerini göster"));
    nodes->setCheckable(true);

    commands_->add(QStringLiteral("analysis.insertSupport"), tr("Insert Support"), CommandGlyph::InsertSupport,
                   QString(), tr("Fixed Support ekle"));
    commands_->add(QStringLiteral("analysis.insertForce"), tr("Insert Force"), CommandGlyph::InsertForce,
                   QString(), tr("Force ekle"));
    commands_->add(QStringLiteral("analysis.solve"), tr("Solve"), CommandGlyph::Solve, QStringLiteral("F5"),
                   tr("Aktif analizi çöz"));

    commands_->add(QStringLiteral("results.exportCsv"), tr("Export CSV"), CommandGlyph::Export, QString(),
                   tr("Sonuçları CSV olarak dışa aktar"));
    commands_->add(QStringLiteral("results.exportVtk"), tr("Export VTK"), CommandGlyph::Export, QString(),
                   tr("Sonuçları legacy VTK olarak dışa aktar"));

    commands_->add(QStringLiteral("view.fit"), tr("Fit View"), CommandGlyph::FitView, QStringLiteral("Ctrl+0"));
    commands_->add(QStringLiteral("view.iso"), tr("Isometric"), CommandGlyph::Isometric, QStringLiteral("Ctrl+1"));

    auto *navigatorToggle = commands_->add(QStringLiteral("panel.navigator"), tr("Navigator"),
                                           CommandGlyph::ShowNavigator);
    navigatorToggle->setCheckable(true);
    navigatorToggle->setChecked(true);
    auto *detailsToggle = commands_->add(QStringLiteral("panel.details"), tr("Details"), CommandGlyph::ShowDetails);
    detailsToggle->setCheckable(true);
    detailsToggle->setChecked(true);
    auto *diagnosticsToggle = commands_->add(QStringLiteral("panel.diagnostics"), tr("Diagnostics"),
                                             CommandGlyph::ShowDiagnostics);
    diagnosticsToggle->setCheckable(true);
    diagnosticsToggle->setChecked(false);

    commands_->addPlain(QStringLiteral("edit.rename"), tr("Yeniden Adlandır"), QStringLiteral("F2"));
    commands_->addPlain(QStringLiteral("edit.duplicate"), tr("Çoğalt"), QStringLiteral("Shift+Ctrl+D"));
    commands_->addPlain(QStringLiteral("edit.delete"), tr("Sil"), QStringLiteral("Del"));
    commands_->addPlain(QStringLiteral("edit.cut"), tr("Kes"), QStringLiteral("Ctrl+X"));
    commands_->addPlain(QStringLiteral("edit.copy"), tr("Kopyala"), QStringLiteral("Ctrl+C"));
    commands_->addPlain(QStringLiteral("edit.paste"), tr("Yapıştır"), QStringLiteral("Ctrl+V"));
    commands_->addPlain(QStringLiteral("edit.selectAll"), tr("Tümünü Seç"), QStringLiteral("Ctrl+A"));
    commands_->addPlain(QStringLiteral("edit.suppress"), tr("Bastır"));
    commands_->addPlain(QStringLiteral("edit.unsuppress"), tr("Bastırmayı Kaldır"));

    commands_->addPlain(QStringLiteral("file.saveAs"), tr("Farklı Kaydet…"), QStringLiteral("Shift+Ctrl+S"));
    commands_->addPlain(QStringLiteral("file.revert"), tr("Kaydedilene Dön"));
    commands_->addPlain(QStringLiteral("file.close"), tr("Kapat"), QStringLiteral("Ctrl+W"));

    commands_->addPlain(QStringLiteral("tree.expandAll"), tr("Tümünü Genişlet"));
    commands_->addPlain(QStringLiteral("tree.collapseAll"), tr("Tümünü Daralt"));

    commands_->addPlain(QStringLiteral("mesh.clearGenerated"), tr("Clear Generated Mesh"));
    commands_->addPlain(QStringLiteral("analysis.preflight"), tr("Preflight"), QStringLiteral("Ctrl+R"));
    commands_->addPlain(QStringLiteral("analysis.clearSolution"), tr("Clear Solution"));
    commands_->addPlain(QStringLiteral("analysis.insertDeformation"), tr("Total Deformation"));
    commands_->addPlain(QStringLiteral("analysis.insertStress"), tr("Equivalent Stress"));
    commands_->addPlain(QStringLiteral("analysis.insertReaction"), tr("Reaction Force"));
    commands_->addPlain(QStringLiteral("material.create"), tr("Yeni Malzeme"));
    commands_->addPlain(QStringLiteral("material.assign"), tr("Gövdeye Ata"));
    commands_->addPlain(QStringLiteral("connections.insertContact"), tr("Yeni Contact Region"));

    commands_->addPlain(QStringLiteral("help.shortcuts"), tr("Klavye Kısayolları"));
    commands_->addPlain(QStringLiteral("help.systemInfo"), tr("Sistem Bilgisi"));
    commands_->addPlain(QStringLiteral("help.about"), tr("Dynamics26 Hakkında"));

    commands_->addPlain(QStringLiteral("analysis.insertStatic"), tr("Static Structural Ekle"));
    commands_->addPlain(QStringLiteral("analysis.insertModal"), tr("Modal Ekle"));
    commands_->addPlain(QStringLiteral("analysis.insertNonlinear"), tr("Nonlinear Static Ekle"));
    commands_->addPlain(QStringLiteral("verify.mixedUp"), tr("Mixed u-p HEX8/P0 Simple Shear"));
    commands_->addPlain(QStringLiteral("verify.contact"), tr("Rigid-Master Frictionless Contact"));
    commands_->addPlain(QStringLiteral("verify.nonlinear"), tr("Total-Lagrangian HEX8 Nonlinear Bar"));
    commands_->addPlain(QStringLiteral("verify.modal"), tr("Axial Modal (2 Eleman)"));
}

void Dynamics26MainWindow::buildCommandSurface()
{
    mainToolBar_ = addToolBar(tr("Komutlar"));
    mainToolBar_->setObjectName(QStringLiteral("Dynamics26CommandSurface"));
    mainToolBar_->setMovable(false);
    mainToolBar_->setFloatable(false);
    mainToolBar_->setIconSize(QSize(17, 17));
    mainToolBar_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    mainToolBar_->addAction(commands_->action(QStringLiteral("file.new")));
    mainToolBar_->addAction(commands_->action(QStringLiteral("file.open")));
    mainToolBar_->addAction(commands_->action(QStringLiteral("file.save")));
    mainToolBar_->addSeparator();
    mainToolBar_->addAction(commands_->action(QStringLiteral("geometry.import")));
    mainToolBar_->addAction(commands_->action(QStringLiteral("mesh.generate")));
    mainToolBar_->addSeparator();
    mainToolBar_->addAction(commands_->action(QStringLiteral("analysis.solve")));
    mainToolBar_->addSeparator();
    mainToolBar_->addAction(commands_->action(QStringLiteral("view.fit")));

    auto *spacer = new QWidget(mainToolBar_);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    mainToolBar_->addWidget(spacer);
    mainToolBar_->addAction(commands_->action(QStringLiteral("panel.navigator")));
    mainToolBar_->addAction(commands_->action(QStringLiteral("panel.details")));
    mainToolBar_->addAction(commands_->action(QStringLiteral("panel.diagnostics")));

    addToolBarBreak();
    contextToolBar_ = addToolBar(tr("Bağlam"));
    contextToolBar_->setObjectName(QStringLiteral("Dynamics26ContextSurface"));
    contextToolBar_->setMovable(false);
    contextToolBar_->setFloatable(false);
    contextToolBar_->setIconSize(QSize(15, 15));
    contextToolBar_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
}

void Dynamics26MainWindow::buildMenus()
{
    auto *fileMenu = menuBar()->addMenu(tr("Dosya"));
    fileMenu->addAction(commands_->action(QStringLiteral("file.new")));
    fileMenu->addAction(commands_->action(QStringLiteral("file.open")));
    recentMenu_ = fileMenu->addMenu(tr("Son Kullanılanlar"));
    fileMenu->addSeparator();
    fileMenu->addAction(commands_->action(QStringLiteral("file.save")));
    fileMenu->addAction(commands_->action(QStringLiteral("file.saveAs")));
    fileMenu->addAction(commands_->action(QStringLiteral("file.revert")));
    fileMenu->addSeparator();
    fileMenu->addAction(commands_->action(QStringLiteral("geometry.import")));
    fileMenu->addSeparator();
    fileMenu->addAction(commands_->action(QStringLiteral("file.close")));

    auto *editMenu = menuBar()->addMenu(tr("Düzenle"));
    editMenu_ = editMenu;
    undoAction_ = documentCommands_->createUndoAction(this);
    undoAction_->setShortcut(QKeySequence::Undo);
    redoAction_ = documentCommands_->createRedoAction(this);
    redoAction_->setShortcut(QKeySequence::Redo);
    editMenu->addAction(undoAction_);
    editMenu->addAction(redoAction_);
    editMenu->addSeparator();
    editMenu->addAction(commands_->action(QStringLiteral("edit.cut")));
    editMenu->addAction(commands_->action(QStringLiteral("edit.copy")));
    editMenu->addAction(commands_->action(QStringLiteral("edit.paste")));
    editMenu->addAction(commands_->action(QStringLiteral("edit.duplicate")));
    editMenu->addAction(commands_->action(QStringLiteral("edit.delete")));
    editMenu->addSeparator();
    editMenu->addAction(commands_->action(QStringLiteral("edit.rename")));
    editMenu->addAction(commands_->action(QStringLiteral("edit.suppress")));
    editMenu->addAction(commands_->action(QStringLiteral("edit.unsuppress")));
    editMenu->addSeparator();
    editMenu->addAction(commands_->action(QStringLiteral("edit.selectAll")));

    auto *geometryMenu = menuBar()->addMenu(tr("Geometri"));
    geometryMenu->addAction(commands_->action(QStringLiteral("geometry.import")));
    geometryMenu->addAction(commands_->action(QStringLiteral("geometry.replace")));
    geometryMenu->addSeparator();
    geometryMenu->addAction(commands_->action(QStringLiteral("geometry.importSection")));

    auto *materialMenu = menuBar()->addMenu(tr("Malzeme"));
    materialMenu->addAction(commands_->action(QStringLiteral("material.create")));
    materialMenu->addAction(commands_->action(QStringLiteral("material.assign")));

    auto *meshMenu = menuBar()->addMenu(tr("Mesh"));
    meshMenu->addAction(commands_->action(QStringLiteral("mesh.generate")));
    meshMenu->addAction(commands_->action(QStringLiteral("mesh.clearGenerated")));
    meshMenu->addSeparator();
    meshMenu->addAction(commands_->action(QStringLiteral("mesh.showNodes")));

    auto *analysisMenu = menuBar()->addMenu(tr("Analiz"));
    analysisMenu->addAction(commands_->action(QStringLiteral("analysis.insertStatic")));
    analysisMenu->addAction(commands_->action(QStringLiteral("analysis.insertModal")));
    analysisMenu->addAction(commands_->action(QStringLiteral("analysis.insertNonlinear")));
    analysisMenu->addSeparator();
    auto *insertMenu = analysisMenu->addMenu(tr("Ekle"));
    insertMenu->addAction(commands_->action(QStringLiteral("analysis.insertSupport")));
    insertMenu->addAction(commands_->action(QStringLiteral("analysis.insertForce")));
    analysisMenu->addSeparator();
    analysisMenu->addAction(commands_->action(QStringLiteral("analysis.preflight")));
    analysisMenu->addAction(commands_->action(QStringLiteral("analysis.solve")));
    analysisMenu->addAction(commands_->action(QStringLiteral("analysis.clearSolution")));
    analysisMenu->addSeparator();
    auto *verification = analysisMenu->addMenu(tr("Solver Doğrulama Preset'leri"));
    verification->setToolTip(tr("Model bağımsız çekirdek doğrulama çözümleri"));
    verification->addAction(commands_->action(QStringLiteral("verify.mixedUp")));
    verification->addAction(commands_->action(QStringLiteral("verify.contact")));
    verification->addAction(commands_->action(QStringLiteral("verify.nonlinear")));
    verification->addAction(commands_->action(QStringLiteral("verify.modal")));

    auto *resultsMenu = menuBar()->addMenu(tr("Sonuçlar"));
    auto *insertResultMenu = resultsMenu->addMenu(tr("Sonuç Ekle"));
    insertResultMenu->addAction(commands_->action(QStringLiteral("analysis.insertDeformation")));
    insertResultMenu->addAction(commands_->action(QStringLiteral("analysis.insertStress")));
    insertResultMenu->addAction(commands_->action(QStringLiteral("analysis.insertReaction")));
    resultsMenu->addSeparator();
    resultsMenu->addAction(commands_->action(QStringLiteral("results.exportCsv")));
    resultsMenu->addAction(commands_->action(QStringLiteral("results.exportVtk")));

    auto *viewMenu = menuBar()->addMenu(tr("Görünüm"));
    viewMenu->addAction(commands_->action(QStringLiteral("view.fit")));
    viewMenu->addAction(commands_->action(QStringLiteral("view.iso")));
    viewMenu->addSeparator();
    viewMenu->addAction(commands_->action(QStringLiteral("tree.expandAll")));
    viewMenu->addAction(commands_->action(QStringLiteral("tree.collapseAll")));
    viewMenu->addSeparator();
    viewMenu->addAction(commands_->action(QStringLiteral("panel.navigator")));
    viewMenu->addAction(commands_->action(QStringLiteral("panel.details")));
    viewMenu->addAction(commands_->action(QStringLiteral("panel.diagnostics")));

    auto *helpMenu = menuBar()->addMenu(tr("Yardım"));
    helpMenu->addAction(commands_->action(QStringLiteral("help.shortcuts")));
    helpMenu->addAction(commands_->action(QStringLiteral("help.systemInfo")));
    helpMenu->addSeparator();
    helpMenu->addAction(commands_->action(QStringLiteral("help.about")));
}

void Dynamics26MainWindow::wireSignals()
{
    connect(commands_, &CommandRegistry::commandTriggered, this, &Dynamics26MainWindow::handleCommand);
    connect(navigator_, &ProjectNavigator::objectSelected, this, &Dynamics26MainWindow::handleSelection);
    connect(navigator_, &ProjectNavigator::contextMenuRequested, this,
            &Dynamics26MainWindow::showObjectContextMenu);
    connect(navigator_, &ProjectNavigator::renameCommitted, this, &Dynamics26MainWindow::renameObject);

    connect(documentCommands_, &DocumentCommandManager::documentMutated, this, [this] {
        navigator_->expandAll();
        syncAll();
    });
    connect(documentCommands_, &DocumentCommandManager::dirtyChanged, this, [this](bool) {
        updateWindowTitle();
        syncCommandStates();
        syncDocumentState();
    });

    QSettings settings;
    recentFiles_ = settings.value(QStringLiteral("recentProjects")).toStringList();

    connect(graphics_, &GraphicsWorkspace::fitViewRequested, this, [this] { graphics_->viewport()->fitView(); });
    connect(graphics_, &GraphicsWorkspace::isometricViewRequested, this,
            [this] { graphics_->viewport()->setIsometricView(); });
    connect(graphics_->viewport(), &ViewportWidget::geometryPicked, this, &Dynamics26MainWindow::handleGeometryPick);

    connect(details_, &DetailsHost::modelEdited, this, [this] { syncAll(); });
    connect(details_, &DetailsHost::commandRequested, this, &Dynamics26MainWindow::handleCommand);
    connect(details_->geometryPage(), &GeometryDetails::representationChanged, this, [this](const int index) {
        const auto representation = index == 1 ? SurfaceRepresentation::Shaded
                                               : (index == 2 ? SurfaceRepresentation::Wireframe
                                                             : SurfaceRepresentation::ShadedWithEdges);
        graphics_->viewport()->setRepresentation(representation);
    });
    connect(details_->geometryPage(), &GeometryDetails::tessellationQualityChanged, this, [this](const double value) {
        tessellationDeflection_ = value;
        syncViewport();
    });
    connect(details_->boundaryConditionPage(), &BoundaryConditionDetails::scopeHighlightRequested, this,
            [this](const quint64 geometryId) {
                graphics_->viewport()->setHighlightedGeometry(geometryId);
            });

    connect(geometry_, &GeometryService::message, this, &Dynamics26MainWindow::reportMessage);
    connect(mesh_, &MeshService::message, this, &Dynamics26MainWindow::reportMessage);
    connect(materials_, &MaterialService::message, this, &Dynamics26MainWindow::reportMessage);
    connect(analysis_, &AnalysisService::message, this, &Dynamics26MainWindow::reportMessage);
    connect(analysis_, &AnalysisService::solverOutput, this,
            [this](const QString &text) { utility_->appendSolverOutput(text); });
    connect(analysis_, &AnalysisService::solveStateChanged, this,
            [this](const ObjectId analysisId, const SolveState state) {
                // AnalysisService signal i (ObjectId, SolveState) tasir. ObjectId yi
                // bool gibi yorumlamak gecerli her analiz kimliginde UI yi sonsuza
                // kadar solving durumunda birakir ve Solve komutunu kilitlerdi.
                // Yalniz aktif analizin lifecycle state i global shell durumunu etkiler.
                if (analysisId != activeAnalysis()) {
                    return;
                }
                solving_ = state == SolveState::Preflight || state == SolveState::Solving;
                syncCommandStates();
                syncStatusBar();
            });

    connect(engineeringStatus_, &EngineeringStatusBar::diagnosticsToggled, this, [this](const bool visible) {
        if (!visible) {
            utility_->noteUserDismissed();
        }
        utility_->setVisible(visible);
        commands_->action(QStringLiteral("panel.diagnostics"))->setChecked(visible);
        if (visible) {
            verticalSplitter_->setSizes({height() - 240, 200});
        }
    });
}

void Dynamics26MainWindow::handleCommand(const QString &id)
{
    if (id == QStringLiteral("file.new")) {
        newProject();
    } else if (id == QStringLiteral("file.open")) {
        openProject();
    } else if (id == QStringLiteral("file.save")) {
        saveProject();
    } else if (id == QStringLiteral("file.saveAs")) {
        saveProjectAs();
    } else if (id == QStringLiteral("file.revert")) {
        revertToSaved();
    } else if (id == QStringLiteral("file.close")) {
        close();
    } else if (id == QStringLiteral("edit.rename")) {
        navigator_->beginInlineRename(selected_);
    } else if (id == QStringLiteral("edit.duplicate")) {
        duplicateObject(selected_);
    } else if (id == QStringLiteral("edit.delete")) {
        deleteObject(selected_);
    } else if (id == QStringLiteral("edit.copy")) {
        copySelectedObject(false);
    } else if (id == QStringLiteral("edit.cut")) {
        copySelectedObject(true);
    } else if (id == QStringLiteral("edit.paste")) {
        pasteObject();
    } else if (id == QStringLiteral("edit.suppress")) {
        setObjectSuppressed(selected_, true);
    } else if (id == QStringLiteral("edit.unsuppress")) {
        setObjectSuppressed(selected_, false);
    } else if (id == QStringLiteral("tree.expandAll")) {
        navigator_->expandAll();
    } else if (id == QStringLiteral("tree.collapseAll")) {
        navigator_->collapseAll();
    } else if (id == QStringLiteral("mesh.clearGenerated")) {
        clearGeneratedMesh();
    } else if (id == QStringLiteral("analysis.preflight")) {
        runPreflight();
    } else if (id == QStringLiteral("analysis.clearSolution")) {
        clearSolution();
    } else if (id == QStringLiteral("analysis.insertDeformation")) {
        documentCommands_->push(new commands::CreateResultDefinitionCommand(
            services_, activeAnalysis(), ResultDefinitionKind::TotalDeformation));
        navigator_->expandAll();
        syncAll();
    } else if (id == QStringLiteral("analysis.insertStress")) {
        documentCommands_->push(new commands::CreateResultDefinitionCommand(
            services_, activeAnalysis(), ResultDefinitionKind::EquivalentStress));
        navigator_->expandAll();
        syncAll();
    } else if (id == QStringLiteral("analysis.insertReaction")) {
        documentCommands_->push(new commands::CreateResultDefinitionCommand(
            services_, activeAnalysis(), ResultDefinitionKind::ReactionForce));
        navigator_->expandAll();
        syncAll();
    } else if (id == QStringLiteral("material.create")) {
        MaterialDefinition definition;
        definition.name = tr("Material");
        auto *command = new commands::CreateMaterialCommand(services_, definition, -1, tr("Add Material"));
        documentCommands_->push(command);
        navigator_->expandAll();
        selectObject(command->createdId());
        syncAll();
    } else if (id == QStringLiteral("material.assign")) {
        if (project_->typeOf(selected_) == ObjectType::Material) {
            documentCommands_->push(new commands::AssignMaterialCommand(services_, selected_));
            syncAll();
        }
    } else if (id == QStringLiteral("connections.insertContact")) {
        ContactDefinition definition;
        auto *command = new commands::CreateContactCommand(services_, definition);
        documentCommands_->push(command);
        navigator_->expandAll();
        if (command->createdId() != InvalidObjectId) {
            selectObject(command->createdId());
        }
        syncAll();
    } else if (id == QStringLiteral("help.shortcuts")) {
        showKeyboardShortcuts();
    } else if (id == QStringLiteral("help.systemInfo")) {
        showSystemInformation();
    } else if (id == QStringLiteral("help.about")) {
        showAbout();
    } else if (id == QStringLiteral("geometry.import")) {
        importGeometry(false);
    } else if (id == QStringLiteral("geometry.replace")) {
        importGeometry(true);
    } else if (id == QStringLiteral("geometry.importSection")) {
        importSection();
    } else if (id == QStringLiteral("mesh.generate")) {
        generateMesh();
    } else if (id == QStringLiteral("mesh.showNodes")) {
        showMeshNodes_ = commands_->action(id)->isChecked();
        syncViewport();
    } else if (id == QStringLiteral("analysis.insertSupport")) {
        auto *command = new commands::CreateFixedSupportCommand(services_, activeAnalysis(), SupportDefinition{},
                                                                -1, tr("Add Fixed Support"));
        documentCommands_->push(command);
        navigator_->expandAll();
        if (command->createdId() != InvalidObjectId) {
            selectObject(command->createdId());
        }
        syncAll();
    } else if (id == QStringLiteral("analysis.insertForce")) {
        auto *command =
            new commands::CreateForceCommand(services_, activeAnalysis(), LoadDefinition{}, -1, tr("Add Force"));
        documentCommands_->push(command);
        navigator_->expandAll();
        if (command->createdId() != InvalidObjectId) {
            selectObject(command->createdId());
        }
        syncAll();
    } else if (id == QStringLiteral("analysis.solve")) {
        solveActiveAnalysis();
    } else if (id == QStringLiteral("analysis.insertStatic")) {
        insertAnalysis(AnalysisType::StaticStructural);
    } else if (id == QStringLiteral("analysis.insertModal")) {
        insertAnalysis(AnalysisType::Modal);
    } else if (id == QStringLiteral("analysis.insertNonlinear")) {
        insertAnalysis(AnalysisType::NonlinearStatic);
    } else if (id == QStringLiteral("results.exportCsv")) {
        exportResults(false);
    } else if (id == QStringLiteral("results.exportVtk")) {
        exportResults(true);
    } else if (id == QStringLiteral("view.fit")) {
        graphics_->viewport()->resetCamera();
    } else if (id == QStringLiteral("view.iso")) {
        graphics_->viewport()->setIsometricView();
    } else if (id == QStringLiteral("panel.navigator")) {
        navigator_->setVisible(commands_->action(id)->isChecked());
    } else if (id == QStringLiteral("panel.details")) {
        details_->setVisible(commands_->action(id)->isChecked());
    } else if (id == QStringLiteral("panel.diagnostics")) {
        const bool visible = commands_->action(id)->isChecked();
        if (!visible) {
            utility_->noteUserDismissed();
        }
        utility_->setVisible(visible);
        engineeringStatus_->setDiagnosticsChecked(visible);
        if (visible) {
            verticalSplitter_->setSizes({height() - 240, 200});
        }
    } else if (id.startsWith(QStringLiteral("verify."))) {
        runVerificationPreset(id);
    }
}

bool Dynamics26MainWindow::runCommand(const QString &commandId)
{
    QAction *action = commands_->action(commandId);
    if (action == nullptr || !action->isEnabled()) {
        return false;
    }
    action->trigger();
    return true;
}

void Dynamics26MainWindow::handleSelection(const ObjectId id)
{
    selected_ = id;
    const ObjectId owning = analysis_->owningAnalysis(id);
    if (owning != InvalidObjectId) {
        activeAnalysis_ = owning;
    }
    details_->showObject(id);
    syncViewport();
    syncCommandStates();
    syncContextualSurface();
    syncStatusBar();
}

void Dynamics26MainWindow::handleGeometryPick(const quint64 geometryId)
{
    graphics_->viewport()->setHighlightedGeometry(geometryId);
    if (geometryId == 0) {
        graphics_->setSelectionLabel(QString());
        syncStatusBar();
        return;
    }
    static const std::array<BoxFace, 6> faces{BoxFace::XMin, BoxFace::XMax, BoxFace::YMin,
                                              BoxFace::YMax, BoxFace::ZMin, BoxFace::ZMax};
    for (const auto face : faces) {
        if (static_cast<quint64>(mesh_->geometryIdFor(face)) != geometryId) {
            continue;
        }
        const QString text = tr("%1 seçildi · %2 facet")
                                 .arg(displayName(face))
                                 .arg(mesh_->facetCountFor(face));
        graphics_->setSelectionLabel(text);
        engineeringStatus_->setSelection(tr("%1  •  Global Coordinate System").arg(displayName(face)));
        return;
    }
    graphics_->setSelectionLabel(tr("1 Body seçildi"));
    engineeringStatus_->setSelection(tr("1 Body  •  Global Coordinate System"));
}

void Dynamics26MainWindow::selectObject(const ObjectId id)
{
    // Selection state'in tek UI kaynağı ProjectNavigator current index'idir.
    // Index gerçekten değişirse objectSelected; MainWindow ardından daha sonra
    // bağlanan SelectionCoordinator'ı senkron olarak çalıştırır. Aynı index için
    // no-op olmak özellikle persistent Named Selection overlay'inin statik
    // ObjectType viewport sync'i tarafından yeniden ezilmesini önler.
    navigator_->selectObject(id);
}

ObjectId Dynamics26MainWindow::firstObjectOfType(const ObjectType type) const
{
    QVector<ObjectId> stack;
    for (const ObjectId root : project_->roots()) {
        stack.push_back(root);
    }
    while (!stack.isEmpty()) {
        const ObjectId id = stack.takeFirst();
        if (project_->typeOf(id) == type) {
            return id;
        }
        for (const ObjectId child : project_->childrenOf(id)) {
            stack.push_back(child);
        }
    }
    return InvalidObjectId;
}

ObjectId Dynamics26MainWindow::activeAnalysis() const
{
    if (analysis_->analysis(activeAnalysis_) != nullptr) {
        return activeAnalysis_;
    }
    const QVector<ObjectId> all = project_->analyses();
    return all.isEmpty() ? InvalidObjectId : all.first();
}

void Dynamics26MainWindow::syncAll()
{
    if (suppressSync_) {
        return;
    }
    dependencies_->evaluate();
    details_->refresh();
    syncViewport();
    syncCommandStates();
    syncContextualSurface();
    syncStatusBar();
    syncDocumentState();
}

void Dynamics26MainWindow::syncViewport()
{
    ViewportWidget *viewport = graphics_->viewport();
    const ObjectType type = project_->typeOf(selected_);
    const ViewportContext context = selected_ == InvalidObjectId ? ViewportContext::Geometry
                                                                 : viewportContextFor(type);
    viewport->setContext(context);
    graphics_->setContextLabel(contextTitleFor(context));
    graphics_->setFaceSelectionAvailable(mesh_->hasMesh());

    const GeometrySummary geometrySummary = geometry_->summary();
    const auto showModelGeometry = [&] {
        if (geometrySummary.hasGeometry) {
            const auto bodies = geometry_->bodies();
            const auto tessellation = geometry_->displayTessellation(bodies.first(), tessellationDeflection_);
            if (tessellation.has_value()) {
                viewport->showGeometry(*tessellation);
                return;
            }
        }
        const MeshService::Definition &definition = mesh_->definition();
        viewport->showGeometry(makeBoxDisplayTessellation(definition.lengthMm, definition.widthMm,
                                                          definition.heightMm));
    };

    const auto showLoadsView = [&] {
        if (!mesh_->hasMesh()) {
            showModelGeometry();
            return;
        }
        QVector<BoundaryGlyph> glyphs;
        const ObjectId analysisId = activeAnalysis();
        const AnalysisRecord *record = analysis_->analysis(analysisId);
        const auto &currentMesh = mesh_->mesh();
        if (record != nullptr) {
            for (const ObjectId supportId : record->supports) {
                const SupportDefinition *definition = analysis_->support(supportId);
                if (definition == nullptr) {
                    continue;
                }
                const BoundaryScopeResolution resolution = analysis_->resolveBoundaryScope(*definition);
                if (!resolution.valid) {
                    continue;
                }
                for (const auto geometryId : resolution.geometryFaceIds) {
                    BoundaryGlyph glyph;
                    glyph.geometryId = geometryId;
                    glyph.isLoad = false;
                    if (!inwardBoundaryNormal(currentMesh, geometryId, glyph.dx, glyph.dy, glyph.dz)) {
                        if (definition->scopingMethod != BoundaryScopingMethod::GeometrySelection) {
                            continue;
                        }
                        outwardNormal(definition->scope, glyph.dx, glyph.dy, glyph.dz);
                        glyph.dx = -glyph.dx;
                        glyph.dy = -glyph.dy;
                        glyph.dz = -glyph.dz;
                    }
                    glyphs.push_back(glyph);
                }
            }
            for (const ObjectId loadId : record->loads) {
                const LoadDefinition *definition = analysis_->load(loadId);
                if (definition == nullptr) {
                    continue;
                }
                const BoundaryScopeResolution resolution = analysis_->resolveBoundaryScope(*definition);
                if (!resolution.valid) {
                    continue;
                }
                for (const auto geometryId : resolution.geometryFaceIds) {
                    BoundaryGlyph glyph;
                    glyph.geometryId = geometryId;
                    glyph.isLoad = true;
                    glyph.dx = definition->fxN;
                    glyph.dy = definition->fyN;
                    glyph.dz = definition->fzN;
                    if (definition->magnitudeN() < 1.0e-12) {
                        double inwardX = 0.0;
                        double inwardY = 0.0;
                        double inwardZ = 0.0;
                        if (inwardBoundaryNormal(currentMesh, geometryId, inwardX, inwardY, inwardZ)) {
                            glyph.dx = -inwardX;
                            glyph.dy = -inwardY;
                            glyph.dz = -inwardZ;
                        } else if (definition->scopingMethod == BoundaryScopingMethod::GeometrySelection) {
                            outwardNormal(definition->scope, glyph.dx, glyph.dy, glyph.dz);
                        } else {
                            continue;
                        }
                    }
                    glyphs.push_back(glyph);
                }
            }
        }
        viewport->showModelWithBoundaryConditions(currentMesh, glyphs);
    };

    switch (context) {
    case ViewportContext::Mesh:
        if (mesh_->hasMesh()) {
            viewport->showMesh(mesh_->mesh(), showMeshNodes_);
        } else {
            showModelGeometry();
        }
        break;
    case ViewportContext::Loads:
    case ViewportContext::Analysis:
        showLoadsView();
        break;
    case ViewportContext::Results: {
        const ObjectId analysisId = analysis_->owningAnalysis(selected_);
        const auto *database = analysis_->resultDatabase(analysisId);
        if (database != nullptr && mesh_->hasMesh()) {
            ResultField field = ResultField::TotalDeformation;
            if (type == ObjectType::EquivalentStress) {
                field = ResultField::EquivalentStress;
            } else if (type == ObjectType::ReactionForce) {
                field = ResultField::ReactionForce;
            }
            viewport->showResult(mesh_->mesh(), *database, field);
            graphics_->setContextLabel(QStringLiteral("%1 — %2")
                                           .arg(contextTitleFor(context), displayName(type)));
        } else {
            showLoadsView();
        }
        break;
    }
    case ViewportContext::Geometry:
    case ViewportContext::Materials:
    case ViewportContext::Connections:
    case ViewportContext::Modal:
    case ViewportContext::Empty:
    default:
        showModelGeometry();
        break;
    }

    femcae::geometry::GeometryEntityId highlighted = femcae::geometry::InvalidGeometryId;
    if (type == ObjectType::FixedSupport) {
        if (const SupportDefinition *definition = analysis_->support(selected_)) {
            highlighted = singleBoundaryHighlight(analysis_->resolveBoundaryScope(*definition));
        }
    } else if (type == ObjectType::Force) {
        if (const LoadDefinition *definition = analysis_->load(selected_)) {
            highlighted = singleBoundaryHighlight(analysis_->resolveBoundaryScope(*definition));
        }
    }
    viewport->setHighlightedGeometry(highlighted);
}

void Dynamics26MainWindow::syncCommandStates()
{
    const bool hasGeometry = geometry_->summary().hasGeometry;
    const bool occt = GeometryService::occtAvailable();
    commands_->setEnabled(QStringLiteral("geometry.import"), occt,
                          tr("OCCT STEP adapter bu derlemede bulunmuyor."));
    commands_->setEnabled(QStringLiteral("geometry.replace"), occt && hasGeometry,
                          hasGeometry ? tr("OCCT STEP adapter bu derlemede bulunmuyor.")
                                      : tr("Değiştirilecek geometri yok."));
    commands_->setEnabled(QStringLiteral("mesh.showNodes"), mesh_->hasMesh(), tr("Önce mesh üretin."));
    commands_->setEnabled(QStringLiteral("mesh.clearGenerated"), mesh_->hasMesh(),
                          tr("Temizlenecek üretilmiş mesh yok."));
    commands_->setEnabled(QStringLiteral("connections.insertContact"), services_.contacts != nullptr,
                          tr("ContactService kullanılamıyor."));

    const ObjectId analysisId = activeAnalysis();
    const PreflightReport report = analysis_->preflight(analysisId);
    const bool solving = solving_;
    commands_->setEnabled(QStringLiteral("analysis.solve"), report.passed() && !solving,
                          solving ? tr("Çözüm sürüyor.") : report.firstFailure());
    commands_->setEnabled(QStringLiteral("analysis.preflight"), analysisId != InvalidObjectId,
                          tr("Önce bir analiz ekleyin."));
    commands_->setEnabled(QStringLiteral("analysis.insertSupport"), analysisId != InvalidObjectId,
                          tr("Önce bir analiz ekleyin."));
    commands_->setEnabled(QStringLiteral("analysis.insertForce"), analysisId != InvalidObjectId,
                          tr("Önce bir analiz ekleyin."));
    for (const char *id : {"analysis.insertDeformation", "analysis.insertStress", "analysis.insertReaction"}) {
        commands_->setEnabled(QLatin1String(id), analysisId != InvalidObjectId, tr("Önce bir analiz ekleyin."));
    }

    const bool hasResults = analysis_->hasResults(analysisId);
    commands_->setEnabled(QStringLiteral("analysis.clearSolution"), hasResults, tr("Temizlenecek çözüm yok."));
    commands_->setEnabled(QStringLiteral("results.exportCsv"), hasResults, tr("Dışa aktarılacak sonuç yok."));
    commands_->setEnabled(QStringLiteral("results.exportVtk"), hasResults, tr("Dışa aktarılacak sonuç yok."));

    const ObjectType type = project_->typeOf(selected_);
    const bool valid = project_->object(selected_) != nullptr;
    commands_->setEnabled(QStringLiteral("edit.rename"), valid && supportsRename(type),
                          tr("Bu nesne yeniden adlandırılamaz."));
    commands_->setEnabled(QStringLiteral("edit.duplicate"), valid && supportsDuplicate(type),
                          tr("Bu nesne çoğaltılamaz."));
    commands_->setEnabled(QStringLiteral("edit.delete"), valid && supportsDelete(type),
                          tr("Bu nesne silinemez."));
    const bool clipboardCompatible = valid && supportsDuplicate(type) && type != ObjectType::NamedSelection;
    commands_->setEnabled(QStringLiteral("edit.cut"), clipboardCompatible && supportsDelete(type),
                          tr("Bu nesne kesilemez."));
    commands_->setEnabled(QStringLiteral("edit.copy"), clipboardCompatible,
                          type == ObjectType::NamedSelection
                              ? tr("Named Selection kopyala/yapıştır bu milestone kapsamında etkin değil.")
                              : tr("Bu nesne kopyalanamaz."));
    commands_->setEnabled(QStringLiteral("edit.paste"), clipboardHasObject(),
                          tr("Panoda yapıştırılabilir Dynamics26 nesnesi yok."));
    commands_->setEnabled(QStringLiteral("edit.selectAll"), false,
                          tr("Çoklu seçim bu sürümde etkin değil."));
    const bool suppressible = valid && supportsSuppression(type);
    const bool alreadySuppressed = suppressible && project_->isSuppressed(selected_);
    commands_->setEnabled(QStringLiteral("edit.suppress"), suppressible && !alreadySuppressed,
                          alreadySuppressed ? tr("Nesne zaten bastırılmış.") : tr("Bu nesne bastırılamaz."));
    commands_->setEnabled(QStringLiteral("edit.unsuppress"), suppressible && alreadySuppressed,
                          tr("Nesne bastırılmamış."));
    commands_->setEnabled(QStringLiteral("material.assign"),
                          type == ObjectType::Material && materials_->assignedMaterialId() != selected_,
                          tr("Bu malzeme zaten atanmış."));

    commands_->setEnabled(QStringLiteral("file.revert"),
                          !currentProjectPath_.isEmpty() && documentCommands_->isDirty(),
                          currentProjectPath_.isEmpty() ? tr("Proje henüz kaydedilmedi.")
                                                        : tr("Kaydedilmemiş değişiklik yok."));
}

void Dynamics26MainWindow::syncContextualSurface()
{
    contextToolBar_->clear();
    contextTitle_ = new ui::SecondaryLabel(QString(), 0.56, 0.76, contextToolBar_);
    QFont contextFont = contextTitle_->font();
    contextFont.setPointSizeF(qMax(9.0, contextFont.pointSizeF() - 2.0));
    contextFont.setBold(true);
    contextFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    contextTitle_->setFont(contextFont);
    contextTitle_->setContentsMargins(8, 0, 12, 0);
    contextToolBar_->addWidget(contextTitle_);

    const ObjectType type = project_->typeOf(selected_);
    switch (type) {
    case ObjectType::GeometryFolder:
    case ObjectType::Body:
        contextTitle_->setText(tr("GEOMETRY"));
        contextToolBar_->addAction(commands_->action(QStringLiteral("geometry.import")));
        contextToolBar_->addAction(commands_->action(QStringLiteral("geometry.replace")));
        contextToolBar_->addAction(commands_->action(QStringLiteral("geometry.importSection")));
        break;
    case ObjectType::Mesh:
        contextTitle_->setText(tr("MESH"));
        contextToolBar_->addAction(commands_->action(QStringLiteral("mesh.generate")));
        contextToolBar_->addAction(commands_->action(QStringLiteral("mesh.showNodes")));
        break;
    case ObjectType::Analysis:
    case ObjectType::AnalysisSettings:
    case ObjectType::FixedSupport:
    case ObjectType::Force:
        contextTitle_->setText(tr("ANALYSIS"));
        contextToolBar_->addAction(commands_->action(QStringLiteral("analysis.insertSupport")));
        contextToolBar_->addAction(commands_->action(QStringLiteral("analysis.insertForce")));
        contextToolBar_->addSeparator();
        contextToolBar_->addAction(commands_->action(QStringLiteral("analysis.solve")));
        break;
    case ObjectType::Solution:
    case ObjectType::TotalDeformation:
    case ObjectType::EquivalentStress:
    case ObjectType::ReactionForce:
        contextTitle_->setText(tr("SOLUTION"));
        contextToolBar_->addAction(commands_->action(QStringLiteral("results.exportCsv")));
        contextToolBar_->addAction(commands_->action(QStringLiteral("results.exportVtk")));
        break;
    case ObjectType::MaterialsFolder:
    case ObjectType::Material:
        contextTitle_->setText(tr("MATERIALS"));
        break;
    case ObjectType::SectionsFolder:
    case ObjectType::Section:
        contextTitle_->setText(tr("SECTIONS"));
        contextToolBar_->addAction(commands_->action(QStringLiteral("geometry.importSection")));
        break;
    case ObjectType::ConnectionsFolder:
    case ObjectType::ContactRegion:
        contextTitle_->setText(tr("CONNECTIONS"));
        contextToolBar_->addAction(commands_->action(QStringLiteral("connections.insertContact")));
        break;
    case ObjectType::NamedSelectionsFolder:
        contextTitle_->setText(tr("NAMED SELECTIONS"));
        break;
    case ObjectType::NamedSelection:
        contextTitle_->setText(tr("NAMED SELECTION"));
        break;
    default:
        contextTitle_->setText(tr("PROJECT"));
        break;
    }
}

void Dynamics26MainWindow::syncStatusBar()
{
    const GeometrySummary summary = geometry_->summary();
    const int bodyCount = summary.hasGeometry ? summary.bodyCount : 1;
    engineeringStatus_->setModelStatistics(bodyCount, mesh_->elementCount(), mesh_->dofCount(), mesh_->hasMesh());

    const ObjectId analysisId = activeAnalysis();
    const AnalysisRecord *record = analysis_->analysis(analysisId);
    const bool ready = analysis_->preflight(analysisId).passed();
    const bool stale = record != nullptr && record->solved && analysis_->solutionIsOutOfDate(analysisId);
    if (solving_) {
        engineeringStatus_->setSolverState(SolverState::Solving);
    } else if (!ready) {
        engineeringStatus_->setSolverState(SolverState::NotReady);
    } else if (record != nullptr && record->solved && !stale) {
        engineeringStatus_->setSolverState(SolverState::Solved,
                                           tr("max |u| %1 mm").arg(record->solveResults.maxDisplacementMm, 0, 'g', 4));
    } else {
        engineeringStatus_->setSolverState(SolverState::Ready);
    }

    const auto setBoundaryStatus = [this](const auto &definition) {
        const BoundaryScopeResolution resolution = analysis_->resolveBoundaryScope(definition);
        if (!resolution.valid) {
            engineeringStatus_->setSelection(
                tr("Scope  •  %1").arg(resolution.error.isEmpty() ? tr("Geçersiz kapsam") : resolution.error));
            return;
        }
        const qlonglong faceCount = static_cast<qlonglong>(resolution.geometryFaceIds.size());
        if (mesh_->hasMesh() && !mesh_->isOutOfDate()) {
            engineeringStatus_->setSelection(
                tr("%1  •  %2 face  •  %3 node  •  Global Coordinate System")
                    .arg(resolution.label)
                    .arg(faceCount)
                    .arg(analysis_->resolvedBoundaryNodeCount(definition)));
        } else {
            engineeringStatus_->setSelection(
                tr("%1  •  %2 face  •  Mesh güncel değil  •  Global Coordinate System")
                    .arg(resolution.label)
                    .arg(faceCount));
        }
    };

    const ObjectType type = project_->typeOf(selected_);
    if (type == ObjectType::FixedSupport) {
        if (const SupportDefinition *definition = analysis_->support(selected_)) {
            setBoundaryStatus(*definition);
            return;
        }
    } else if (type == ObjectType::Force) {
        if (const LoadDefinition *definition = analysis_->load(selected_)) {
            setBoundaryStatus(*definition);
            return;
        }
    }
    const ProjectObject *object = project_->object(selected_);
    engineeringStatus_->setSelection(object != nullptr ? object->name : QString());
}

void Dynamics26MainWindow::syncDocumentState()
{
    QString stale;
    if (mesh_->isOutOfDate()) {
        stale = tr("Mesh Out of Date");
    } else if (dependencies_->anySolutionOutOfDate()) {
        stale = tr("Solution Out of Date");
    }
    engineeringStatus_->setDocumentState(documentCommands_->isDirty(), stale);
}

void Dynamics26MainWindow::rebuildGeometryNodes()
{
    project_->removeChildren(project_->geometryNode());
    const GeometrySummary summary = geometry_->summary();
    if (summary.hasGeometry) {
        for (const auto bodyId : geometry_->bodies()) {
            const ObjectId node = project_->addObject(project_->geometryNode(), ObjectType::Body,
                                                      geometry_->bodyName(bodyId), static_cast<qint64>(bodyId));
            project_->setState(node, ObjectState::UpToDate, tr("CAD gövdesi"));
        }
        project_->setState(project_->geometryNode(), ObjectState::UpToDate,
                           tr("%1 — %2 body").arg(summary.sourceFileName).arg(summary.bodyCount));
    } else {
        const ObjectId node = project_->addObject(project_->geometryNode(), ObjectType::Body,
                                                  tr("Body 1"), 0);
        project_->setState(node, ObjectState::Ready, tr("Parametrik kutu"));
        project_->setState(project_->geometryNode(), ObjectState::Ready, tr("Parametrik kutu gövdesi"));
    }
    navigator_->expandAll();
}

void Dynamics26MainWindow::newProjectWithoutPrompt()
{
    suppressSync_ = true;
    // Persistent engineering scope servisleri ProjectModel kimliklerini kullanır;
    // model reset'ten ÖNCE temizlenmelidir. Aksi halde service storage içinde
    // artık ağaçta bulunmayan ObjectId'ler kalır ve sonraki projede çakışır.
    if (services_.namedSelections != nullptr) {
        services_.namedSelections->clear();
    }
    if (services_.contacts != nullptr) {
        services_.contacts->clear();
    }
    analysis_->clearAll();
    materials_->clear();
    geometry_->clear();
    mesh_->reset();
    project_->resetToEmptyProject();
    utility_->clearAll();
    rebuildGeometryNodes();
    materials_->resetToDefault();
    activeAnalysis_ = analysis_->createAnalysis(AnalysisType::StaticStructural);
    currentProjectPath_.clear();
    suppressSync_ = false;

    documentCommands_->resetHistory();
    navigator_->expandAll();
    selectObject(project_->geometryNode());
    updateWindowTitle();
    syncAll();
}

void Dynamics26MainWindow::newProject()
{
    if (!confirmDiscardChanges()) {
        return;
    }
    newProjectWithoutPrompt();
    reportMessage(tr("Yeni proje oluşturuldu."), Severity::Info);
}

bool Dynamics26MainWindow::confirmDiscardChanges()
{
    if (!documentCommands_->isDirty()) {
        return true;
    }
    QMessageBox box(this);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(tr("Dynamics26"));
    box.setText(tr("Bu projede kaydedilmemiş değişiklikler var."));
    box.setInformativeText(tr("Devam etmeden önce kaydetmek ister misiniz?"));
    box.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Save);
    const int answer = box.exec();
    if (answer == QMessageBox::Cancel) {
        return false;
    }
    if (answer == QMessageBox::Save) {
        saveProject();
        return !documentCommands_->isDirty();
    }
    return true;
}

void Dynamics26MainWindow::openProject()
{
    if (!confirmDiscardChanges()) {
        return;
    }
    const QString path = QFileDialog::getOpenFileName(this, tr("Dynamics26 Projesi Aç"), QString(),
                                                      tr("Dynamics26 / FEMCAE Project (*.femcae.json);;JSON (*.json)"));
    if (path.isEmpty()) {
        return;
    }
    (void)openProjectFromPath(path);
}

bool Dynamics26MainWindow::openProjectFromPath(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        reportMessage(tr("Proje açılamadı: %1").arg(path), Severity::Error);
        return false;
    }
    constexpr qint64 kMaximumProjectBytes = 16 * 1024 * 1024;
    if (file.size() < 2 || file.size() > kMaximumProjectBytes) {
        reportMessage(tr("Proje dosyası boş veya boyut sınırını aşıyor: %1").arg(path), Severity::Error);
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        reportMessage(tr("Proje JSON bozuk: %1 (offset %2)").arg(parseError.errorString()).arg(parseError.offset),
                      Severity::Error);
        return false;
    }
    const auto migration = femcae::gui::ProjectFileMigrator::migrate(document.object(), fem_project_schema_version());
    if (!migration.ok) {
        reportMessage(tr("Proje şema doğrulama/migration hatası: %1").arg(migration.message), Severity::Error);
        return false;
    }
    if (migration.migrated) {
        reportMessage(tr("Proje migration uygulandı: %1").arg(migration.message), Severity::Warning);
    }

    const QJsonObject root = migration.project;
    newProjectWithoutPrompt();

    suppressSync_ = true;
    const QJsonObject documentObject = root.value(QStringLiteral("dynamics26_document")).toObject();
    const bool hasFullDocument = !documentObject.isEmpty();

    geometry_->loadProjectJson(root.value(QStringLiteral("geometry")).toObject());
    mesh_->loadProjectJson(root.value(QStringLiteral("prepost")).toObject());

    if (hasFullDocument) {
        analysis_->clearAll();
        materials_->clear();

        ObjectId reservedId = InvalidObjectId;
        const QJsonValue nextIdValue = documentObject.value(QStringLiteral("next_object_id"));
        if (!nextIdValue.isUndefined() && !parseDocumentObjectId(nextIdValue, &reservedId)) {
            suppressSync_ = false;
            newProjectWithoutPrompt();
            reportMessage(tr("Proje açılamadı: next_object_id geçerli bir 64-bit kimlik değil."), Severity::Error);
            return false;
        }
        if (reservedId != InvalidObjectId) {
            project_->reserveIdsUpTo(reservedId);
        }

        materials_->fromJson(documentObject.value(QStringLiteral("materials")).toObject());
        analysis_->fromJson(documentObject.value(QStringLiteral("analyses")).toObject());

        const QJsonObject namedSelectionsObject =
            documentObject.value(QStringLiteral("named_selections")).toObject();
        if (!namedSelectionsObject.isEmpty()) {
            if (services_.namedSelections == nullptr) {
                suppressSync_ = false;
                newProjectWithoutPrompt();
                reportMessage(tr("Proje Named Selection içeriyor ancak persistent scope servisi hazır değil."),
                              Severity::Error);
                return false;
            }
            QString namedSelectionError;
            if (!services_.namedSelections->fromJson(namedSelectionsObject, &namedSelectionError)) {
                suppressSync_ = false;
                newProjectWithoutPrompt();
                reportMessage(tr("Named Selection verisi yüklenemedi: %1").arg(namedSelectionError),
                              Severity::Error);
                return false;
            }
        }

        const QJsonObject contactsObject = documentObject.value(QStringLiteral("contacts")).toObject();
        if (!contactsObject.isEmpty()) {
            if (services_.contacts == nullptr) {
                suppressSync_ = false;
                newProjectWithoutPrompt();
                reportMessage(tr("Proje Contact Region içeriyor ancak ContactService hazır değil."),
                              Severity::Error);
                return false;
            }
            QString contactError;
            if (!services_.contacts->fromJson(contactsObject, &contactError)) {
                suppressSync_ = false;
                newProjectWithoutPrompt();
                reportMessage(tr("Contact verisi yüklenemedi: %1").arg(contactError), Severity::Error);
                return false;
            }
        }
    } else {
        materials_->fromLegacyJson(root.value(QStringLiteral("material")).toObject());
        analysis_->applyLegacyLoadJson(root.value(QStringLiteral("load")).toObject());
        reportMessage(tr("Eski proje şeması yüklendi; nesne grafiği varsayılanlardan tamamlandı."),
                      Severity::Warning);
    }

    rebuildGeometryNodes();
    const QVector<ObjectId> analyses = project_->analyses();
    activeAnalysis_ = analyses.isEmpty() ? InvalidObjectId : analyses.first();
    currentProjectPath_ = path;
    suppressSync_ = false;

    documentCommands_->resetHistory();
    rememberRecentFile(path);
    reportMessage(tr("Proje açıldı: %1").arg(QFileInfo(path).fileName()), Severity::Success);
    navigator_->expandAll();
    selectObject(project_->geometryNode());
    updateWindowTitle();
    syncAll();
    return true;
}

void Dynamics26MainWindow::saveProject()
{
    if (currentProjectPath_.isEmpty()) {
        saveProjectAs();
        return;
    }
    (void)saveProjectToPath(currentProjectPath_);
}

void Dynamics26MainWindow::saveProjectAs()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Dynamics26 Projesini Kaydet"),
                                                currentProjectPath_.isEmpty()
                                                    ? QStringLiteral("dynamics26-model.femcae.json")
                                                    : currentProjectPath_,
                                                tr("Dynamics26 / FEMCAE Project (*.femcae.json);;JSON (*.json)"));
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)) {
        path += QStringLiteral(".femcae.json");
    }
    (void)saveProjectToPath(path);
}

bool Dynamics26MainWindow::saveProjectToPath(const QString &path)
{
    QJsonObject root;
    root[QStringLiteral("application_version")] = QStringLiteral("%1.%2.%3")
                                                      .arg(fem_version_major())
                                                      .arg(fem_version_minor())
                                                      .arg(fem_version_patch());
    root[QStringLiteral("project_schema")] = fem_project_schema_version();
    root[QStringLiteral("analysis")] = QStringLiteral("static_structural_hex8_mesh");
    root[QStringLiteral("material")] = materials_->toLegacyJson();
    QJsonObject section;
    section[QStringLiteral("type")] = QStringLiteral("truss");
    section[QStringLiteral("area_mm2")] = 100.0;
    section[QStringLiteral("length_mm")] = mesh_->definition().lengthMm;
    root[QStringLiteral("section")] = section;
    root[QStringLiteral("load")] = analysis_->toLegacyLoadJson();
    root[QStringLiteral("geometry")] = geometry_->projectJson();
    root[QStringLiteral("prepost")] = mesh_->projectJson();

    QJsonObject documentObject;
    documentObject[QStringLiteral("version")] = 1;
    documentObject[QStringLiteral("materials")] = materials_->toJson();
    documentObject[QStringLiteral("analyses")] = analysis_->toJson();
    // ObjectId aynı engineering identity contract'inin parçasıdır; JSON number
    // (>2^53) precision riski alınmaz. Loader eski numeric alanı da kabul eder.
    documentObject[QStringLiteral("next_object_id")] = QString::number(project_->peekNextId());
    if (services_.namedSelections != nullptr) {
        documentObject[QStringLiteral("named_selections")] = services_.namedSelections->toJson();
    }
    if (services_.contacts != nullptr) {
        documentObject[QStringLiteral("contacts")] = services_.contacts->toJson();
    }
    documentObject[QStringLiteral("gui_milestone")] = QStringLiteral(DYNAMICS26_GUI_MILESTONE);
    root[QStringLiteral("dynamics26_document")] = documentObject;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        reportMessage(tr("Proje kaydedilemedi: %1").arg(path), Severity::Error);
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    currentProjectPath_ = path;
    documentCommands_->markSaved();
    rememberRecentFile(path);
    updateWindowTitle();
    reportMessage(tr("Proje kaydedildi: %1").arg(QFileInfo(path).fileName()), Severity::Success);
    syncCommandStates();
    return true;
}

void Dynamics26MainWindow::revertToSaved()
{
    if (currentProjectPath_.isEmpty()) {
        return;
    }
    const QString path = currentProjectPath_;
    QMessageBox box(this);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(tr("Dynamics26"));
    box.setText(tr("Kaydedilene dönülsün mü?"));
    box.setInformativeText(tr("Son kaydetmeden sonraki tüm değişiklikler kaybolur."));
    box.setStandardButtons(QMessageBox::Discard | QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Cancel);
    if (box.exec() != QMessageBox::Discard) {
        return;
    }
    (void)openProjectFromPath(path);
}

void Dynamics26MainWindow::rememberRecentFile(const QString &path)
{
    recentFiles_.removeAll(path);
    recentFiles_.prepend(path);
    while (recentFiles_.size() > 8) {
        recentFiles_.removeLast();
    }
    QSettings settings;
    settings.setValue(QStringLiteral("recentProjects"), recentFiles_);
    rebuildRecentMenu();
}

void Dynamics26MainWindow::rebuildRecentMenu()
{
    if (recentMenu_ == nullptr) {
        return;
    }
    recentMenu_->clear();
    if (recentFiles_.isEmpty()) {
        QAction *empty = recentMenu_->addAction(tr("Son proje yok"));
        empty->setEnabled(false);
        return;
    }
    for (const QString &path : recentFiles_) {
        QAction *action = recentMenu_->addAction(QFileInfo(path).fileName());
        action->setToolTip(path);
        connect(action, &QAction::triggered, this, [this, path] {
            if (confirmDiscardChanges()) {
                (void)openProjectFromPath(path);
            }
        });
    }
}

void Dynamics26MainWindow::updateWindowTitle()
{
    const QString name = currentProjectPath_.isEmpty() ? tr("Adsız Proje")
                                                       : QFileInfo(currentProjectPath_).fileName();
    QString title = QStringLiteral("Dynamics26 — %1").arg(name);
    if (documentCommands_->isDirty()) {
        title += tr(" — Düzenlendi");
    }
    setWindowTitle(title);
    setWindowModified(false);
    setWindowFilePath(currentProjectPath_);
}

bool Dynamics26MainWindow::importGeometryFromPath(const QString &path)
{
    if (!geometry_->importStep(path)) {
        return false;
    }
    rebuildGeometryNodes();
    selectObject(project_->geometryNode());
    syncAll();
    return true;
}

void Dynamics26MainWindow::importGeometry(const bool replace)
{
    const QString path = QFileDialog::getOpenFileName(this, replace ? tr("Geometriyi Değiştir") : tr("Geometri İçe Aktar"),
                                                      QString(), tr("STEP Geometry (*.step *.stp *.STEP *.STP)"));
    if (path.isEmpty()) {
        return;
    }
    if (replace) {
        geometry_->clear();
        mesh_->clearGenerated();
    }
    (void)importGeometryFromPath(path);
}

void Dynamics26MainWindow::importSection()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("DXF Kesit İçe Aktar"), QString(),
                                                      tr("DXF (*.dxf *.DXF)"));
    if (path.isEmpty()) {
        return;
    }
    if (geometry_->importDxfSection(path)) {
        selectObject(project_->sectionsNode());
    }
    syncAll();
}

void Dynamics26MainWindow::generateMesh()
{
    QElapsedTimer timer;
    timer.start();
    const bool ok = mesh_->generate();
    const double seconds = static_cast<double>(timer.nsecsElapsed()) * 1.0e-9;
    if (ok) {
        utility_->appendTiming(tr("Mesh üretimi"), seconds);
        selectObject(project_->meshNode());
    } else {
        showUtility(UtilityWorkspace::Tab::Messages, true);
    }
    syncAll();
}

void Dynamics26MainWindow::clearGeneratedMesh()
{
    mesh_->clearGenerated();
    selectObject(project_->meshNode());
    syncAll();
}

void Dynamics26MainWindow::clearSolution()
{
    const ObjectId analysisId = activeAnalysis();
    if (analysisId == InvalidObjectId) {
        return;
    }
    analysis_->clearSolution(analysisId);
    syncAll();
}

void Dynamics26MainWindow::runPreflight()
{
    const ObjectId analysisId = activeAnalysis();
    const PreflightReport report = analysis_->preflight(analysisId);
    showUtility(UtilityWorkspace::Tab::Messages, true);
    reportMessage(QStringLiteral("── PRE-FLIGHT ──"), Severity::Info);
    for (const auto &check : report.checks) {
        const Severity severity = check.status == PreflightCheck::Status::Passed
            ? Severity::Success
            : (check.status == PreflightCheck::Status::Warning ? Severity::Warning : Severity::Error);
        const QString mark = check.status == PreflightCheck::Status::Passed
            ? QStringLiteral("✓")
            : (check.status == PreflightCheck::Status::Warning ? QStringLiteral("!") : QStringLiteral("✕"));
        utility_->appendMessage(QStringLiteral("%1 %2%3")
                                    .arg(mark, check.label,
                                         check.detail.isEmpty() ? QString() : QStringLiteral(" — ") + check.detail),
                                severity);
    }
    reportMessage(report.passed() ? tr("Ready to Solve") : tr("Preflight başarısız — çözüm başlatılamaz."),
                  report.passed() ? Severity::Success : Severity::Warning);
    syncAll();
}

void Dynamics26MainWindow::solveActiveAnalysis()
{
    const ObjectId analysisId = activeAnalysis();
    if (analysisId == InvalidObjectId) {
        return;
    }

    utility_->clearSolverOutput();
    solving_ = true;
    dependencies_->setSolvingAnalysis(analysisId);
    engineeringStatus_->setSolverState(SolverState::Solving);
    QApplication::setOverrideCursor(Qt::BusyCursor);
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    const bool ok = analysis_->solve(analysisId);
    QApplication::restoreOverrideCursor();
    solving_ = false;
    dependencies_->setSolvingAnalysis(InvalidObjectId);

    const AnalysisRecord *record = analysis_->analysis(analysisId);
    if (ok && record != nullptr) {
        const SolveResults &results = record->solveResults;
        utility_->appendTiming(tr("Static Structural çözümü"), results.wallClockSeconds);
        utility_->setResultRows({
            {tr("Maximum Total Deformation"), QStringLiteral("%1 mm").arg(results.maxDisplacementMm, 0, 'g', 8)},
            {tr("Maximum Equivalent Stress"), QStringLiteral("%1 MPa").arg(results.maxVonMisesMPa, 0, 'g', 8)},
            {tr("Minimum Equivalent Stress"), QStringLiteral("%1 MPa").arg(results.minVonMisesMPa, 0, 'g', 8)},
            {QStringLiteral("ΣRx"), QStringLiteral("%1 N").arg(results.reactionXN, 0, 'g', 8)},
            {QStringLiteral("ΣRy"), QStringLiteral("%1 N").arg(results.reactionYN, 0, 'g', 8)},
            {QStringLiteral("ΣRz"), QStringLiteral("%1 N").arg(results.reactionZN, 0, 'g', 8)},
            {tr("Nodes"), QString::number(results.nodeCount)},
            {tr("Elements"), QString::number(results.elementCount)},
            {tr("Degrees of Freedom"), QString::number(results.dofCount)},
        });
        navigator_->expandAll();
        const ObjectId deformation = firstObjectOfType(ObjectType::TotalDeformation);
        if (deformation != InvalidObjectId) {
            selectObject(deformation);
        }
    } else {
        showUtility(UtilityWorkspace::Tab::Messages, true);
    }
    syncAll();
}

void Dynamics26MainWindow::exportResults(const bool vtk)
{
    const ObjectId analysisId = activeAnalysis();
    const auto *database = analysis_->resultDatabase(analysisId);
    if (database == nullptr || !mesh_->hasMesh()) {
        reportMessage(tr("Dışa aktarılacak sonuç yok."), Severity::Warning);
        return;
    }
    const QString path = vtk
        ? QFileDialog::getSaveFileName(this, tr("Legacy VTK Kaydet"), QStringLiteral("dynamics26-results.vtk"),
                                       tr("VTK (*.vtk)"))
        : QFileDialog::getSaveFileName(this, tr("Sonuç CSV Kaydet"), QStringLiteral("dynamics26-results.csv"),
                                       tr("CSV (*.csv)"));
    if (path.isEmpty()) {
        return;
    }
    try {
        if (vtk) {
            database->exportLegacyVtk(mesh_->mesh(), path.toStdString(), 1.0);
        } else {
            database->exportCsv(mesh_->mesh(), path.toStdString());
        }
        reportMessage(tr("Dışa aktarıldı: %1").arg(QFileInfo(path).fileName()), Severity::Success);
    } catch (const std::exception &ex) {
        reportMessage(tr("Dışa aktarma başarısız: %1").arg(QString::fromUtf8(ex.what())), Severity::Error);
        showUtility(UtilityWorkspace::Tab::Messages, true);
    }
}

void Dynamics26MainWindow::insertAnalysis(const AnalysisType type)
{
    auto *command = new commands::CreateAnalysisCommand(services_, type);
    documentCommands_->push(command);
    activeAnalysis_ = command->createdId();
    navigator_->expandAll();
    selectObject(activeAnalysis_);
    syncAll();
}

void Dynamics26MainWindow::runVerificationPreset(const QString &id)
{
    showUtility(UtilityWorkspace::Tab::SolverOutput, true);
    utility_->appendSolverOutput(QStringLiteral("──────────────────────────────────────────────"));

    const MaterialDefinition *material = materials_->assigned();
    const double young = (material != nullptr ? material->youngGPa : 210.0) * 1.0e9;
    const double poisson = material != nullptr ? material->poisson : 0.30;

    if (id == QStringLiteral("verify.mixedUp")) {
        utility_->appendSolverOutput(tr("Doğrulama preset'i: Mixed u-p HEX8/P0 manufactured simple shear"));
        const double c10 = (material != nullptr ? material->c10MPa : 1.0) * 1.0e6;
        const double bulk = (material != nullptr ? material->bulkMPa : 2000.0) * 1.0e6;
        double recoveredGamma = 0.0;
        double pressure = 0.0;
        double loadFactor = 0.0;
        double pressureResidual = 0.0;
        int iterations = 0;
        const int status = fem_demo_mixed_up_hex8_shear(c10, bulk, 0.12, &recoveredGamma, &pressure, &loadFactor,
                                                        &pressureResidual, &iterations);
        if (status != 0) {
            utility_->appendSolverOutput(tr("  BAŞARISIZ — engine status %1").arg(status));
            reportMessage(tr("Mixed u-p doğrulaması başarısız (status %1).").arg(status), Severity::Error);
            return;
        }
        utility_->appendSolverOutput(tr("  İstenen γ           = %1").arg(0.12));
        utility_->appendSolverOutput(tr("  Çözülen γ           = %1").arg(recoveredGamma, 0, 'g', 8));
        utility_->appendSolverOutput(tr("  Element P0 basıncı  = %1 MPa").arg(pressure / 1.0e6, 0, 'g', 8));
        utility_->appendSolverOutput(tr("  Basınç rezidüeli    = %1").arg(pressureResidual, 0, 'g', 8));
        utility_->appendSolverOutput(tr("  Newton düzeltmesi   = %1").arg(iterations));
        reportMessage(tr("Mixed u-p HEX8/P0 doğrulaması tamamlandı."), Severity::Success);
        return;
    }

    if (id == QStringLiteral("verify.contact")) {
        utility_->appendSolverOutput(tr("Doğrulama preset'i: Rigid-master sürtünmesiz temas"));
        double penetration = 0.0;
        double normalForce = 0.0;
        int activeContacts = 0;
        int iterations = 0;
        const int status = fem_demo_contact_hex8(young, poisson, young * 100.0, 1000.0, 1, &penetration,
                                                 &normalForce, &activeContacts, &iterations);
        if (status != 0) {
            utility_->appendSolverOutput(tr("  BAŞARISIZ — engine status %1").arg(status));
            reportMessage(tr("Temas doğrulaması başarısız (status %1).").arg(status), Severity::Error);
            return;
        }
        utility_->appendSolverOutput(tr("  Aktif temas noktası = %1").arg(activeContacts));
        utility_->appendSolverOutput(tr("  Maks. penetrasyon   = %1 mm").arg(penetration * 1.0e3, 0, 'g', 8));
        utility_->appendSolverOutput(tr("  Normal temas kuvv.  = %1 N").arg(normalForce, 0, 'g', 8));
        utility_->appendSolverOutput(tr("  Newton düzeltmesi   = %1").arg(iterations));
        reportMessage(tr("Temas doğrulaması tamamlandı."), Severity::Success);
        return;
    }

    if (id == QStringLiteral("verify.nonlinear")) {
        utility_->appendSolverOutput(tr("Doğrulama preset'i: Total-Lagrangian HEX8 nonlineer çubuk"));
        constexpr int kCapacity = 512;
        std::vector<int> attempts(kCapacity);
        std::vector<int> iterations(kCapacity);
        std::vector<int> converged(kCapacity);
        std::vector<double> loadFactors(kCapacity);
        std::vector<double> relativeResiduals(kCapacity);
        std::vector<double> relativeDisplacements(kCapacity);
        std::vector<double> alphas(kCapacity);
        double displacement = 0.0;
        double completedLoadFactor = 0.0;
        double finalResidual = 0.0;
        int acceptedSteps = 0;
        int totalIterations = 0;
        int cutbacks = 0;
        int historyCount = 0;
        const int status = fem_demo_nonlinear_hex8(young, poisson, 1.0e-4, 1.0, 1000.0, 0.25, 0.01, 0.5, 1, 1, 25, 1,
                                                   &displacement, &completedLoadFactor, &finalResidual, &acceptedSteps,
                                                   &totalIterations, &cutbacks, kCapacity, &historyCount,
                                                   attempts.data(), iterations.data(), loadFactors.data(),
                                                   relativeResiduals.data(), relativeDisplacements.data(),
                                                   alphas.data(), converged.data());
        if (status != 0) {
            utility_->appendSolverOutput(tr("  BAŞARISIZ — engine status %1").arg(status));
            reportMessage(tr("Nonlineer doğrulama başarısız (status %1).").arg(status), Severity::Error);
            return;
        }
        utility_->appendSolverOutput(tr("  Uç deplasman        = %1 mm").arg(displacement * 1.0e3, 0, 'g', 8));
        utility_->appendSolverOutput(tr("  Tamamlanan λ        = %1").arg(completedLoadFactor, 0, 'g', 8));
        utility_->appendSolverOutput(tr("  Kabul edilen adım   = %1").arg(acceptedSteps));
        utility_->appendSolverOutput(tr("  Newton düzeltmesi   = %1").arg(totalIterations));
        utility_->appendSolverOutput(tr("  Cutback             = %1").arg(cutbacks));

        SolverConvergenceSnapshot snapshot;
        snapshot.summary.state = SolverConvergenceState::Converged;
        snapshot.summary.completedLoadFactor = completedLoadFactor;
        snapshot.summary.finalResidualNorm = finalResidual;
        snapshot.summary.acceptedSteps = acceptedSteps;
        snapshot.summary.totalIterations = totalIterations;
        snapshot.summary.cutbackCount = cutbacks;
        snapshot.entries.reserve(historyCount);
        for (int i = 0; i < historyCount; ++i) {
            const std::size_t index = static_cast<std::size_t>(i);
            snapshot.entries.push_back(SolverConvergenceEntry{
                attempts[index],
                iterations[index],
                loadFactors[index],
                relativeResiduals[index],
                relativeDisplacements[index],
                alphas[index],
                converged[index] != 0
            });
        }
        utility_->setConvergenceData(snapshot);
        showUtility(UtilityWorkspace::Tab::Convergence, true);
        reportMessage(tr("Nonlineer doğrulama tamamlandı: %1 yakınsama kaydı.").arg(historyCount), Severity::Success);
        return;
    }

    if (id == QStringLiteral("verify.modal")) {
        utility_->appendSolverOutput(tr("Doğrulama preset'i: İki elemanlı eksenel modal"));
        const double density = material != nullptr ? material->densityKgM3 : 7850.0;
        double f1 = 0.0;
        double f2 = 0.0;
        double mid1 = 0.0;
        double tip1 = 0.0;
        double mid2 = 0.0;
        double tip2 = 0.0;
        const int status = fem_demo_axial_modal(young, density, 1.0e-4, 1.0, &f1, &f2, &mid1, &tip1, &mid2, &tip2);
        if (status != 0) {
            utility_->appendSolverOutput(tr("  BAŞARISIZ — engine status %1").arg(status));
            reportMessage(tr("Modal doğrulama başarısız (status %1).").arg(status), Severity::Error);
            return;
        }
        utility_->appendSolverOutput(tr("  Mode 1 = %1 Hz").arg(f1, 0, 'g', 8));
        utility_->appendSolverOutput(tr("  Mode 2 = %1 Hz").arg(f2, 0, 'g', 8));
        reportMessage(tr("Modal doğrulama tamamlandı."), Severity::Success);
        return;
    }
}

void Dynamics26MainWindow::showUtility(const UtilityWorkspace::Tab tab, const bool force)
{
    if (utility_->userDismissed() && !force) {
        return;
    }
    utility_->showTab(tab);
    if (!utility_->isVisible()) {
        utility_->setVisible(true);
        verticalSplitter_->setSizes({height() - 240, 200});
    }
    commands_->action(QStringLiteral("panel.diagnostics"))->setChecked(true);
    engineeringStatus_->setDiagnosticsChecked(true);
}

void Dynamics26MainWindow::reportMessage(const QString &text, const Severity severity)
{
    utility_->appendMessage(text, severity);
    if (severity == Severity::Error) {
        showUtility(UtilityWorkspace::Tab::Messages, true);
    }
}

QVector<ObjectId> Dynamics26MainWindow::objectsOfType(const ObjectType type) const
{
    QVector<ObjectId> found;
    QVector<ObjectId> stack;
    for (const ObjectId root : project_->roots()) {
        stack.push_back(root);
    }
    while (!stack.isEmpty()) {
        const ObjectId id = stack.takeFirst();
        if (project_->typeOf(id) == type) {
            found.push_back(id);
        }
        for (const ObjectId child : project_->childrenOf(id)) {
            stack.push_back(child);
        }
    }
    return found;
}

void Dynamics26MainWindow::renameObject(const ObjectId id, const QString &newName)
{
    const ProjectObject *object = project_->object(id);
    if (object == nullptr || newName.trimmed().isEmpty() || newName.trimmed() == object->name) {
        return;
    }
    if (object->type == ObjectType::NamedSelection) {
        documentCommands_->push(new commands::RenameNamedSelectionCommand(services_, id, newName));
    } else if (object->type == ObjectType::ContactRegion && services_.contacts != nullptr) {
        documentCommands_->push(new commands::RenameContactCommand(services_, id, newName));
    } else {
        documentCommands_->push(new commands::RenameObjectCommand(services_, id, newName));
    }
    syncAll();
}

void Dynamics26MainWindow::duplicateObject(const ObjectId id)
{
    const ObjectType type = project_->typeOf(id);
    if (!supportsDuplicate(type)) {
        return;
    }
    ObjectId created = InvalidObjectId;
    if (type == ObjectType::Material) {
        const MaterialDefinition *definition = materials_->byId(id);
        if (definition == nullptr) {
            return;
        }
        MaterialDefinition copy = *definition;
        copy.name = tr("%1 Copy").arg(definition->name);
        auto *command = new commands::CreateMaterialCommand(services_, copy, materials_->rowOf(id) + 1,
                                                            tr("Duplicate %1").arg(definition->name));
        documentCommands_->push(command);
        created = command->createdId();
    } else if (type == ObjectType::FixedSupport) {
        const SupportDefinition *definition = analysis_->support(id);
        if (definition == nullptr) {
            return;
        }
        SupportDefinition copy = *definition;
        copy.name = tr("%1 Copy").arg(definition->name);
        auto *command = new commands::CreateFixedSupportCommand(services_, analysis_->owningAnalysis(id), copy,
                                                                project_->rowOf(id) + 1,
                                                                tr("Duplicate %1").arg(definition->name));
        documentCommands_->push(command);
        created = command->createdId();
    } else if (type == ObjectType::Force) {
        const LoadDefinition *definition = analysis_->load(id);
        if (definition == nullptr) {
            return;
        }
        LoadDefinition copy = *definition;
        copy.name = tr("%1 Copy").arg(definition->name);
        auto *command = new commands::CreateForceCommand(services_, analysis_->owningAnalysis(id), copy,
                                                         project_->rowOf(id) + 1,
                                                         tr("Duplicate %1").arg(definition->name));
        documentCommands_->push(command);
        created = command->createdId();
    } else if (type == ObjectType::NamedSelection && services_.namedSelections != nullptr) {
        const NamedSelectionDefinition *definition = services_.namedSelections->byId(id);
        if (definition == nullptr) {
            return;
        }
        NamedSelectionDefinition copy = *definition;
        copy.name = tr("%1 Copy").arg(definition->name);
        auto *command = new commands::CreateNamedSelectionCommand(
            services_, copy, services_.namedSelections->rowOf(id) + 1,
            tr("Duplicate %1").arg(definition->name));
        documentCommands_->push(command);
        created = command->createdId();
    }
    if (created != InvalidObjectId) {
        navigator_->expandAll();
        selectObject(created);
    }
    syncAll();
}

void Dynamics26MainWindow::deleteObject(const ObjectId id)
{
    const ObjectType type = project_->typeOf(id);
    if (!supportsDelete(type)) {
        return;
    }
    if (type == ObjectType::Material) {
        if (materials_->count() <= 1) {
            reportMessage(tr("Modelde en az bir malzeme bulunmalıdır."), Severity::Warning);
            return;
        }
        documentCommands_->push(new commands::DeleteMaterialCommand(services_, id));
    } else if (type == ObjectType::Analysis) {
        documentCommands_->push(new commands::DeleteAnalysisCommand(services_, id));
        activeAnalysis_ = InvalidObjectId;
    } else if (type == ObjectType::ContactRegion && services_.contacts != nullptr) {
        documentCommands_->push(new commands::DeleteContactCommand(services_, id));
    } else if (type == ObjectType::FixedSupport || type == ObjectType::Force) {
        documentCommands_->push(new commands::DeleteBoundaryConditionCommand(services_, id));
    } else if (isResultDefinition(type)) {
        documentCommands_->push(new commands::DeleteResultDefinitionCommand(services_, id));
    } else if (type == ObjectType::NamedSelection && services_.namedSelections != nullptr) {
        documentCommands_->push(new commands::DeleteNamedSelectionCommand(services_, id));
    }
    navigator_->expandAll();
    selectObject(project_->projectRoot());
    syncAll();
}

void Dynamics26MainWindow::setObjectSuppressed(const ObjectId id, const bool suppressed)
{
    const ObjectType type = project_->typeOf(id);
    if (!supportsSuppression(type) || project_->isSuppressed(id) == suppressed) {
        return;
    }
    if (type == ObjectType::ContactRegion && services_.contacts != nullptr) {
        documentCommands_->push(new commands::SetContactSuppressedCommand(services_, id, suppressed));
    } else {
        documentCommands_->push(new commands::SuppressObjectCommand(services_, id, suppressed));
    }
    syncAll();
}

namespace {
const char *kClipboardMime = "application/x-dynamics26-object+json";
}

bool Dynamics26MainWindow::clipboardHasObject() const
{
    const QMimeData *data = QGuiApplication::clipboard()->mimeData();
    return data != nullptr && data->hasFormat(QLatin1String(kClipboardMime));
}

void Dynamics26MainWindow::copySelectedObject(const bool cut)
{
    const ObjectType type = project_->typeOf(selected_);
    if (!supportsDuplicate(type) || type == ObjectType::NamedSelection) {
        return;
    }
    QJsonObject payload;
    payload[QStringLiteral("object_type")] = static_cast<int>(type);
    if (type == ObjectType::Material) {
        const MaterialDefinition *definition = materials_->byId(selected_);
        if (definition == nullptr) {
            return;
        }
        payload[QStringLiteral("data")] = definition->toJson();
    } else if (type == ObjectType::FixedSupport) {
        const SupportDefinition *definition = analysis_->support(selected_);
        if (definition == nullptr) {
            return;
        }
        payload[QStringLiteral("data")] = definition->toJson();
    } else if (type == ObjectType::Force) {
        const LoadDefinition *definition = analysis_->load(selected_);
        if (definition == nullptr) {
            return;
        }
        payload[QStringLiteral("data")] = definition->toJson();
    } else {
        return;
    }

    auto *data = new QMimeData;
    data->setData(QLatin1String(kClipboardMime), QJsonDocument(payload).toJson(QJsonDocument::Compact));
    data->setText(project_->object(selected_)->name);
    QGuiApplication::clipboard()->setMimeData(data);

    if (cut) {
        deleteObject(selected_);
    } else {
        syncCommandStates();
    }
}

void Dynamics26MainWindow::pasteObject()
{
    const QMimeData *data = QGuiApplication::clipboard()->mimeData();
    if (data == nullptr || !data->hasFormat(QLatin1String(kClipboardMime))) {
        return;
    }
    const QJsonObject payload =
        QJsonDocument::fromJson(data->data(QLatin1String(kClipboardMime))).object();
    const auto type = static_cast<ObjectType>(payload.value(QStringLiteral("object_type")).toInt(-1));
    const QJsonObject definitionObject = payload.value(QStringLiteral("data")).toObject();

    ObjectId created = InvalidObjectId;
    if (type == ObjectType::Material) {
        MaterialDefinition definition = MaterialDefinition::fromJson(definitionObject);
        auto *command = new commands::CreateMaterialCommand(services_, definition, -1,
                                                            tr("Paste %1").arg(definition.name));
        documentCommands_->push(command);
        created = command->createdId();
    } else if (type == ObjectType::FixedSupport || type == ObjectType::Force) {
        const ObjectId analysisId = activeAnalysis();
        if (analysisId == InvalidObjectId) {
            reportMessage(tr("Yapıştırmak için önce bir analiz seçin."), Severity::Warning);
            return;
        }
        if (type == ObjectType::FixedSupport) {
            SupportDefinition definition = SupportDefinition::fromJson(definitionObject);
            auto *command = new commands::CreateFixedSupportCommand(services_, analysisId, definition, -1,
                                                                    tr("Paste %1").arg(definition.name));
            documentCommands_->push(command);
            created = command->createdId();
        } else {
            LoadDefinition definition = LoadDefinition::fromJson(definitionObject);
            auto *command = new commands::CreateForceCommand(services_, analysisId, definition, -1,
                                                             tr("Paste %1").arg(definition.name));
            documentCommands_->push(command);
            created = command->createdId();
        }
    }
    if (created != InvalidObjectId) {
        navigator_->expandAll();
        selectObject(created);
    }
    syncAll();
}

void Dynamics26MainWindow::showObjectContextMenu(const ObjectId id, const QPoint &globalPosition)
{
    QMenu *menu = buildContextMenu(id, this);
    if (menu == nullptr) {
        return;
    }
    menu->exec(globalPosition);
    menu->deleteLater();
}

QMenu *Dynamics26MainWindow::buildContextMenu(const ObjectId id, QWidget *parent)
{
    if (project_->object(id) == nullptr) {
        return nullptr;
    }
    if (selected_ != id) {
        handleSelection(id);
    }
    syncCommandStates();

    auto *menuPtr = new QMenu(parent);
    QMenu &menu = *menuPtr;
    const ObjectType type = project_->typeOf(id);
    const auto add = [&menu, this](const char *commandId) {
        if (QAction *action = commands_->action(QLatin1String(commandId))) {
            menu.addAction(action);
        }
    };

    switch (type) {
    case ObjectType::FixedSupport:
    case ObjectType::Force:
        add("edit.rename");
        add("edit.duplicate");
        menu.addSeparator();
        add(project_->isSuppressed(id) ? "edit.unsuppress" : "edit.suppress");
        menu.addSeparator();
        add("edit.delete");
        break;
    case ObjectType::Material:
        add("material.assign");
        menu.addSeparator();
        add("edit.rename");
        add("edit.duplicate");
        add("edit.delete");
        break;
    case ObjectType::MaterialsFolder:
        add("material.create");
        break;
    case ObjectType::Mesh:
        add("mesh.generate");
        add("mesh.clearGenerated");
        menu.addSeparator();
        add("mesh.showNodes");
        break;
    case ObjectType::GeometryFolder:
        add("geometry.import");
        add("geometry.replace");
        add("geometry.importSection");
        break;
    case ObjectType::Body:
        add("edit.rename");
        menu.addSeparator();
        add(project_->isSuppressed(id) ? "edit.unsuppress" : "edit.suppress");
        break;
    case ObjectType::ConnectionsFolder:
        add("connections.insertContact");
        break;
    case ObjectType::ContactRegion:
        add("connections.insertContact");
        menu.addSeparator();
        add("edit.rename");
        menu.addSeparator();
        add(project_->isSuppressed(id) ? "edit.unsuppress" : "edit.suppress");
        menu.addSeparator();
        add("edit.delete");
        break;
    case ObjectType::NamedSelection:
        add("edit.rename");
        add("edit.duplicate");
        menu.addSeparator();
        add("edit.delete");
        break;
    case ObjectType::NamedSelectionsFolder:
        add("tree.expandAll");
        add("tree.collapseAll");
        break;
    case ObjectType::Analysis:
    case ObjectType::AnalysisSettings: {
        add("analysis.preflight");
        add("analysis.solve");
        add("analysis.clearSolution");
        menu.addSeparator();
        QMenu *insert = menu.addMenu(tr("Ekle"));
        insert->addAction(commands_->action(QStringLiteral("analysis.insertSupport")));
        insert->addAction(commands_->action(QStringLiteral("analysis.insertForce")));
        menu.addSeparator();
        add("edit.rename");
        add("edit.delete");
        break;
    }
    case ObjectType::Solution: {
        add("analysis.clearSolution");
        menu.addSeparator();
        QMenu *insert = menu.addMenu(tr("Sonuç Ekle"));
        insert->addAction(commands_->action(QStringLiteral("analysis.insertDeformation")));
        insert->addAction(commands_->action(QStringLiteral("analysis.insertStress")));
        insert->addAction(commands_->action(QStringLiteral("analysis.insertReaction")));
        break;
    }
    case ObjectType::TotalDeformation:
    case ObjectType::EquivalentStress:
    case ObjectType::ReactionForce:
        add("edit.rename");
        menu.addSeparator();
        add(project_->isSuppressed(id) ? "edit.unsuppress" : "edit.suppress");
        menu.addSeparator();
        add("edit.delete");
        menu.addSeparator();
        add("results.exportCsv");
        add("results.exportVtk");
        break;
    default:
        add("tree.expandAll");
        add("tree.collapseAll");
        break;
    }

    if (menu.isEmpty()) {
        delete menuPtr;
        return nullptr;
    }
    return menuPtr;
}

void Dynamics26MainWindow::showKeyboardShortcuts()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Klavye Kısayolları"));
    auto *layout = new QVBoxLayout(&dialog);
    auto *text = new QPlainTextEdit(&dialog);
    text->setReadOnly(true);
    text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    QStringList lines;
    lines << tr("DOSYA")
          << QStringLiteral("  ⌘N        %1").arg(tr("Yeni"))
          << QStringLiteral("  ⌘O        %1").arg(tr("Aç"))
          << QStringLiteral("  ⌘S        %1").arg(tr("Kaydet"))
          << QStringLiteral("  ⇧⌘S       %1").arg(tr("Farklı Kaydet"))
          << QStringLiteral("  ⌘W        %1").arg(tr("Kapat"))
          << QString()
          << tr("DÜZENLE")
          << QStringLiteral("  ⌘Z        %1").arg(tr("Geri Al"))
          << QStringLiteral("  ⇧⌘Z       %1").arg(tr("Yinele"))
          << QStringLiteral("  ⌘X        %1").arg(tr("Kes"))
          << QStringLiteral("  ⌘C        %1").arg(tr("Kopyala"))
          << QStringLiteral("  ⌘V        %1").arg(tr("Yapıştır"))
          << QStringLiteral("  ⇧⌘D       %1").arg(tr("Çoğalt"))
          << QStringLiteral("  F2        %1").arg(tr("Yeniden Adlandır"))
          << QStringLiteral("  Delete    %1").arg(tr("Sil"))
          << QString()
          << tr("MODEL")
          << QStringLiteral("  F7        %1").arg(tr("Generate Mesh"))
          << QStringLiteral("  ⌘R        %1").arg(tr("Preflight"))
          << QStringLiteral("  F5        %1").arg(tr("Solve"))
          << QString()
          << tr("GÖRÜNÜM")
          << QStringLiteral("  ⌘0        %1").arg(tr("Fit View"))
          << QStringLiteral("  ⌘1        %1").arg(tr("İzometrik"));
    text->setPlainText(lines.join(QLatin1Char('\n')));
    layout->addWidget(text);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addWidget(buttons);
    dialog.resize(460, 520);
    dialog.exec();
}

void Dynamics26MainWindow::showSystemInformation()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Sistem Bilgisi"));
    auto *layout = new QVBoxLayout(&dialog);
    auto *text = new QPlainTextEdit(&dialog);
    text->setReadOnly(true);
    text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    QStringList lines;
    lines << QStringLiteral("Dynamics26 GUI      : %1").arg(QStringLiteral(DYNAMICS26_GUI_MILESTONE))
          << QStringLiteral("Application Version : %1").arg(QStringLiteral(FEMCAE_APP_VERSION))
          << QStringLiteral("Solver Engine       : %1.%2.%3")
                 .arg(fem_version_major()).arg(fem_version_minor()).arg(fem_version_patch())
          << QStringLiteral("Solver C ABI        : %1").arg(fem_api_version())
          << QStringLiteral("Project Schema      : %1").arg(fem_project_schema_version())
          << QStringLiteral("Result Schema       : %1").arg(fem_result_schema_version())
          << QString()
          << QStringLiteral("Qt                  : %1 (build %2)").arg(qVersion(), QStringLiteral(QT_VERSION_STR))
          << QStringLiteral("VTK viewport        : %1")
                 .arg(ViewportWidget::vtkAvailable() ? QStringLiteral("enabled") : QStringLiteral("disabled"))
          << QStringLiteral("OCCT geometry       : %1")
                 .arg(GeometryService::occtAvailable() ? QStringLiteral("enabled") : QStringLiteral("disabled"))
          << QString()
          << QStringLiteral("Operating System    : %1").arg(QSysInfo::prettyProductName())
          << QStringLiteral("Kernel              : %1 %2").arg(QSysInfo::kernelType(), QSysInfo::kernelVersion())
          << QStringLiteral("Architecture        : %1").arg(QSysInfo::currentCpuArchitecture());
    text->setPlainText(lines.join(QLatin1Char('\n')));
    layout->addWidget(text);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addWidget(buttons);
    dialog.resize(520, 400);
    dialog.exec();
}

void Dynamics26MainWindow::showAbout()
{
    QMessageBox::about(this, tr("Dynamics26 Hakkında"),
                       tr("<b>Dynamics26</b> %1<br><br>"
                          "Modern Fortran FEM çekirdeği · C++20 geometry/meshing · Qt 6 + VTK + OCCT<br>"
                          "Solver engine %2.%3.%4 · C ABI %5")
                           .arg(QStringLiteral(DYNAMICS26_GUI_MILESTONE))
                           .arg(fem_version_major())
                           .arg(fem_version_minor())
                           .arg(fem_version_patch())
                           .arg(fem_api_version()));
}

void Dynamics26MainWindow::closeEvent(QCloseEvent *event)
{
    if (confirmDiscardChanges()) {
        event->accept();
    } else {
        event->ignore();
    }
}

bool Dynamics26MainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::ApplicationPaletteChange || event->type() == QEvent::PaletteChange) {
        commands_->refreshIcons();
        graphics_->refreshIcons();
        navigator_->refreshDecorations();
        graphics_->viewport()->refreshAppearance();
    }
    return QMainWindow::event(event);
}

} // namespace d26