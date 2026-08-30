#include "SelfTest.h"

#include "../commands/DomainCommands.h"
#include "../core/DependencyEngine.h"
#include "../core/DocumentCommandManager.h"
#include "../core/ProjectModel.h"
#include "../core/UiTheme.h"
#include "../services/AnalysisService.h"
#include "../services/GeometryService.h"
#include "../services/MaterialService.h"
#include "../services/MeshService.h"
#include "../shell/Dynamics26MainWindow.h"
#include "../shell/GraphicsWorkspace.h"

#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QStyleHints>
#include <QTemporaryDir>
#include <QTimer>
#include <QUndoStack>
#include <QtGlobal>

#include <cmath>
#include <iostream>

namespace d26 {
namespace {

int failures = 0;
int checksRun = 0;

void settle(const int milliseconds)
{
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
    QApplication::processEvents(QEventLoop::AllEvents, 60);
}

void check(const bool condition, const std::string &what)
{
    ++checksRun;
    std::cout << (condition ? "  PASS  " : "  FAIL  ") << what << '\n';
    if (!condition) {
        ++failures;
    }
}

void section(const char *title)
{
    std::cout << "\n[" << title << "]\n";
}

bool nearlyEqual(const double a, const double b, const double relativeTolerance = 1.0e-9)
{
    const double scale = std::max({1.0, std::abs(a), std::abs(b)});
    return std::abs(a - b) <= relativeTolerance * scale;
}

} // namespace

int runSelfTest(QApplication &app, Dynamics26MainWindow &window)
{
    failures = 0;
    checksRun = 0;
    const ServiceContext services = window.services();
    DocumentCommandManager *document = window.documentCommands();
    QUndoStack *stack = document->stack();
    std::cout << "Dynamics26 GUI self-test\n";
    settle(400);

    // =======================================================================
    section("1 — başlangıç nesne grafiği");
    check(services.project->projectRoot() != InvalidObjectId, "proje kökü kuruldu");
    check(window.firstObjectOfType(ObjectType::Body) != InvalidObjectId, "başlangıç gövdesi var");
    check(window.firstObjectOfType(ObjectType::Material) != InvalidObjectId, "malzeme nesnesi ağaçta");
    check(window.firstObjectOfType(ObjectType::Analysis) != InvalidObjectId, "Static Structural analizi var");
    check(window.firstObjectOfType(ObjectType::FixedSupport) != InvalidObjectId, "varsayılan sınır şartı var");
    check(window.firstObjectOfType(ObjectType::Force) != InvalidObjectId, "varsayılan yük var");
    check(window.objectsOfType(ObjectType::TotalDeformation).size() == 1, "Total Deformation TANIMI çözümden önce var");
    check(window.objectsOfType(ObjectType::EquivalentStress).size() == 1, "Equivalent Stress tanımı var");
    check(window.objectsOfType(ObjectType::ReactionForce).size() == 1, "Reaction Force tanımı var");
    check(!services.mesh->hasMesh(), "başlangıçta mesh yok");
    check(!window.utility()->isVisible(), "alt yardımcı alan başlangıçta kapalı");
    check(!document->isDirty(), "başlangıçta doküman temiz");
    check(!ViewportWidget::vtkAvailable() || window.graphics()->viewport()->hasAxisTriad(),
          "viewport: non-interactive axis triad kuruldu");
    check(!ViewportWidget::orientationCubeAvailable() || window.graphics()->viewport()->hasOrientationCube(),
          "viewport: orientation cube camera'ya bağlı");
    {
        ViewportWidget *viewport = window.graphics()->viewport();
        const int undoIndex = stack->index();
        bool representationsOk = true;
        for (const SurfaceRepresentation representation : {SurfaceRepresentation::Shaded,
                                                            SurfaceRepresentation::ShadedWithEdges,
                                                            SurfaceRepresentation::Wireframe}) {
            viewport->setRepresentation(representation);
            representationsOk = representationsOk && viewport->representationMatchesScene();
        }
        viewport->setRepresentation(SurfaceRepresentation::ShadedWithEdges);
        check(representationsOk, "viewport: Shaded / Shaded+Edges / Wireframe aktör durumu doğru");
        check(stack->index() == undoIndex, "representation değişimi document Undo stack'e girmiyor");
    }

    // =======================================================================
    section("2 — mesh üretimi ve bağımlılık");
    const ObjectId meshNode = services.project->meshNode();
    const ObjectId analysisId = window.firstObjectOfType(ObjectType::Analysis);
    const AnalysisRecord *record = services.analysis->analysis(analysisId);

    // Mesh bölmeleri MODEL STATE'tir → undoable komut olarak uygulanır.
    MeshService::Definition meshBefore = services.mesh->definition();
    MeshService::Definition meshAfter = meshBefore;
    meshAfter.nx = 10;
    meshAfter.ny = 2;
    meshAfter.nz = 2;
    document->push(new commands::SetMeshDefinitionCommand(services, meshBefore, meshAfter,
                                                          QStringLiteral("Change Mesh Divisions")));
    settle(60);
    check(services.mesh->definition().nx == 10, "mesh bölmesi komutla değişti");
    check(document->isDirty(), "model düzenlemesi dokümanı kirletti");

    check(window.runCommand(QStringLiteral("mesh.generate")), "Generate Mesh komutu çalıştı");
    settle(300);
    check(services.mesh->hasMesh(), "mesh üretildi");
    check(services.mesh->nodeCount() == 11 * 3 * 3, "düğüm sayısı beklenen (11x3x3)");
    check(services.mesh->elementCount() == 10 * 2 * 2, "eleman sayısı beklenen (10x2x2)");
    check(services.mesh->quality().invertedElementCount == 0, "ters eleman yok");
    check(!services.mesh->isOutOfDate(), "üretimden hemen sonra mesh güncel");
    check(services.project->object(meshNode)->state == ObjectState::UpToDate, "Mesh düğümü UpToDate");

    // =======================================================================
    section("3 — çözüm ve sonuç nesneleri");
    check(services.analysis->preflight(analysisId).passed(), "preflight geçti");
    check(window.runCommand(QStringLiteral("analysis.solve")), "Solve komutu çalıştı");
    settle(400);
    record = services.analysis->analysis(analysisId);
    check(record != nullptr && record->solved, "çözüm tamamlandı");
    check(record != nullptr && record->solveResults.maxDisplacementMm > 0.0, "deplasman pozitif");
    check(record != nullptr && record->solveResults.maxVonMisesMPa > 0.0, "von Mises pozitif");
    check(!services.analysis->solutionIsOutOfDate(analysisId), "çözüm güncel");
    {
        const ObjectId deformation = window.firstObjectOfType(ObjectType::TotalDeformation);
        check(services.project->object(deformation)->state == ObjectState::UpToDate,
              "sonuç nesnesi UpToDate oldu");
    }
    // Denge kontrolü: toplam reaksiyon uygulanan yükü dengelemelidir.
    {
        const ObjectId forceId = window.firstObjectOfType(ObjectType::Force);
        const LoadDefinition *load = services.analysis->load(forceId);
        check(record != nullptr && load != nullptr
                  && std::abs(record->solveResults.reactionXN + load->fxN) < 1.0e-6 * std::max(1.0, std::abs(load->fxN)),
              "ΣRx uygulanan yükü dengeliyor");
    }

    // =======================================================================
    section("4 — bağımlılık / out-of-date (§43)");
    {
        const ObjectId forceId = window.firstObjectOfType(ObjectType::Force);
        const LoadDefinition *before = services.analysis->load(forceId);
        LoadDefinition after = *before;
        after.fxN = before->fxN * 1.5;
        document->push(new commands::SetForceCommand(services, forceId, *before, after));
        settle(80);
        check(!services.mesh->isOutOfDate(), "yük değişimi mesh'i bayatlatmadı");
        check(services.analysis->solutionIsOutOfDate(analysisId), "yük değişimi çözümü OutOfDate yaptı");
        check(services.project->object(analysisId)->state == ObjectState::OutOfDate,
              "analiz düğümü OutOfDate gösteriyor");
        stack->undo();
        settle(80);
    }
    {
        // Mesh ayarı değişince hem mesh hem çözüm bayatlar.
        MeshService::Definition before = services.mesh->definition();
        MeshService::Definition after = before;
        after.nx = 20;
        document->push(new commands::SetMeshDefinitionCommand(services, before, after,
                                                              QStringLiteral("Change Mesh Divisions")));
        settle(80);
        check(services.mesh->isOutOfDate(), "Nx değişimi mesh'i OutOfDate yaptı");
        check(services.analysis->solutionIsOutOfDate(analysisId), "Nx değişimi çözümü OutOfDate yaptı");
        check(!services.analysis->preflight(analysisId).passed(), "bayat mesh ile preflight başarısız");
        stack->undo();
        settle(80);
        check(!services.mesh->isOutOfDate(), "undo sonrası mesh yeniden güncel");
    }
    {
        // Malzeme değişimi mesh'i etkilemez, çözümü bayatlatır.
        const ObjectId materialId = services.materials->assignedMaterialId();
        const MaterialDefinition *before = services.materials->byId(materialId);
        MaterialDefinition after = *before;
        after.youngGPa = before->youngGPa * 0.5;
        document->push(new commands::SetMaterialPropertiesCommand(services, materialId, *before, after));
        settle(80);
        check(!services.mesh->isOutOfDate(), "malzeme değişimi mesh'i bayatlatmadı");
        check(services.analysis->solutionIsOutOfDate(analysisId), "malzeme değişimi çözümü OutOfDate yaptı");
        stack->undo();
        settle(80);
        check(!services.analysis->solutionIsOutOfDate(analysisId),
              "Undo ile girdi eski haline dönünce çözüm yeniden geçerli");
    }
    {
        // Ad değişikliği solver girdisi değildir: sonuçları bayatlatmamalıdır.
        const ObjectId forceId = window.firstObjectOfType(ObjectType::Force);
        const QString original = services.project->object(forceId)->name;
        window.renameObject(forceId, QStringLiteral("Tip Load"));
        settle(80);
        check(!services.analysis->solutionIsOutOfDate(analysisId),
              "yeniden adlandırma çözümü bayatlatmadı (solver girdisi değil)");
        window.renameObject(forceId, original);
        settle(80);
    }

    // =======================================================================
    section("5 — Undo / Redo domain komutları (§41)");
    {
        const int before = window.objectsOfType(ObjectType::Force).size();
        check(window.runCommand(QStringLiteral("analysis.insertForce")), "Insert Force komutu çalıştı");
        settle(80);
        check(window.objectsOfType(ObjectType::Force).size() == before + 1, "Force sayısı arttı");
        stack->undo();
        settle(80);
        check(window.objectsOfType(ObjectType::Force).size() == before, "Undo: Force sayısı eski değere döndü");
        stack->redo();
        settle(80);
        check(window.objectsOfType(ObjectType::Force).size() == before + 1, "Redo: Force geri geldi");
        stack->undo();
        settle(80);
    }
    {
        // Özellik değişikliği: Fx 100 → (duraklama) → 250 → undo → redo.
        // Duraklama birleştirme penceresini aştığı için iki ayrı Undo adımı olur.
        const ObjectId forceId = window.firstObjectOfType(ObjectType::Force);
        LoadDefinition base = *services.analysis->load(forceId);
        base.fxN = 100.0;
        document->push(new commands::SetForceCommand(services, forceId, *services.analysis->load(forceId), base));
        settle(commands::DomainCommand::mergeWindowMs() + 150);
        LoadDefinition next = base;
        next.fxN = 250.0;
        document->push(new commands::SetForceCommand(services, forceId, base, next));
        settle(60);
        check(nearlyEqual(services.analysis->load(forceId)->fxN, 250.0), "Fx = 250 uygulandı");
        stack->undo();
        settle(60);
        check(nearlyEqual(services.analysis->load(forceId)->fxN, 100.0), "Undo: Fx = 100");
        stack->redo();
        settle(60);
        check(nearlyEqual(services.analysis->load(forceId)->fxN, 250.0), "Redo: Fx = 250");
    }
    {
        // Hızlı ardışık düzenlemeler (spinbox sürükleme) TEK Undo adımına birleşir.
        // Önce bir duraklama: bu blok önceki komutun birleşme zincirine takılmasın.
        settle(commands::DomainCommand::mergeWindowMs() + 150);
        const ObjectId forceId = window.firstObjectOfType(ObjectType::Force);
        const LoadDefinition original = *services.analysis->load(forceId);
        const int indexBefore = stack->index();
        LoadDefinition step = original;
        for (int i = 1; i <= 4; ++i) {
            LoadDefinition next = step;
            next.fzN = 10.0 * i;
            document->push(new commands::SetForceCommand(services, forceId, step, next));
            step = next;
        }
        settle(60);
        check(stack->index() == indexBefore + 1, "hızlı ardışık düzenlemeler tek Undo adımında birleşti");
        stack->undo();
        settle(60);
        check(nearlyEqual(services.analysis->load(forceId)->fzN, original.fzN),
              "Undo: birleşmiş adım tek seferde geri alındı");
    }
    {
        // Yeniden adlandırma
        const ObjectId forceId = window.firstObjectOfType(ObjectType::Force);
        const QString original = services.project->object(forceId)->name;
        window.renameObject(forceId, QStringLiteral("Load A"));
        settle(60);
        check(services.project->object(forceId)->name == QStringLiteral("Load A"), "Rename uygulandı");
        check(services.analysis->load(forceId)->name == QStringLiteral("Load A"),
              "Rename domain DisplayName'i değiştirdi (yalnız ağaç metni değil)");
        stack->undo();
        settle(60);
        check(services.project->object(forceId)->name == original, "Undo: eski ad geri geldi");
        stack->redo();
        settle(60);
        check(services.project->object(forceId)->name == QStringLiteral("Load A"), "Redo: yeni ad");
        stack->undo();
        settle(60);
    }
    {
        // Silme ve geri alma — nesne kimliği ve durumu korunmalı
        const ObjectId supportId = window.firstObjectOfType(ObjectType::FixedSupport);
        const SupportDefinition definition = *services.analysis->support(supportId);
        const int rowBefore = services.project->rowOf(supportId);
        window.deleteObject(supportId);
        settle(80);
        check(services.analysis->support(supportId) == nullptr, "Fixed Support silindi");
        stack->undo();
        settle(80);
        const SupportDefinition *restored = services.analysis->support(supportId);
        check(restored != nullptr, "Undo: Fixed Support AYNI ObjectId ile geri geldi");
        check(restored != nullptr && restored->name == definition.name, "Undo: ad korundu");
        check(restored != nullptr && restored->scope == definition.scope, "Undo: kapsam korundu");
        check(services.project->rowOf(supportId) == rowBefore, "Undo: ağaçtaki konum korundu");
    }
    {
        // Malzeme oluştur / çoğalt / sil
        const int before = window.objectsOfType(ObjectType::Material).size();
        check(window.runCommand(QStringLiteral("material.create")), "New Material komutu çalıştı");
        settle(80);
        check(window.objectsOfType(ObjectType::Material).size() == before + 1, "malzeme eklendi");
        stack->undo();
        settle(80);
        check(window.objectsOfType(ObjectType::Material).size() == before, "Undo: malzeme kaldırıldı");
    }

    // =======================================================================
    section("6 — Suppress / Unsuppress (§18)");
    {
        const ObjectId forceId = window.firstObjectOfType(ObjectType::Force);
        window.setObjectSuppressed(forceId, true);
        settle(80);
        check(services.project->isSuppressed(forceId), "yük bastırıldı");
        check(services.project->object(forceId) != nullptr, "bastırılan nesne modelde kaldı (silinmedi)");
        check(services.project->object(forceId)->state == ObjectState::Suppressed, "durum Suppressed");
        const PreflightReport report = services.analysis->preflight(analysisId);
        check(!report.passed(), "tüm yükler bastırılınca preflight başarısız");
        stack->undo();
        settle(80);
        check(!services.project->isSuppressed(forceId), "Undo: bastırma kaldırıldı");
        check(services.analysis->preflight(analysisId).passed(), "preflight yeniden geçiyor");
    }

    // =======================================================================
    section("7 — Preflight (§46)");
    {
        // Malzeme hyperelastic yapılınca Static Structural yolu kapanır.
        const ObjectId materialId = services.materials->assignedMaterialId();
        const MaterialDefinition *before = services.materials->byId(materialId);
        MaterialDefinition after = *before;
        after.model = MaterialModel::MooneyRivlin;
        document->push(new commands::SetMaterialPropertiesCommand(services, materialId, *before, after));
        settle(80);
        check(!services.analysis->preflight(analysisId).passed(), "hyperelastic malzeme ile preflight başarısız");
        stack->undo();
        settle(80);
        check(services.analysis->preflight(analysisId).passed(), "lineer malzemeye dönünce preflight geçiyor");
    }
    {
        // Sıkışmazlık niyeti mixed u-p'ye çözülünce Solve kapanır.
        const AnalysisRecord *current = services.analysis->analysis(analysisId);
        document->push(new commands::SetIncompressibilityCommand(services, analysisId, current->incompressibility,
                                                                 IncompressibilityIntent::NearlyIncompressible));
        settle(80);
        check(services.analysis->resolvedFormulation(analysisId) == ResolvedFormulation::MixedUP,
              "Nearly Incompressible -> mixed u-p");
        check(!services.analysis->preflight(analysisId).passed(), "mixed u-p ile preflight başarısız (dürüst kısıt)");
        stack->undo();
        settle(80);
        check(services.analysis->resolvedFormulation(analysisId) == ResolvedFormulation::DisplacementBased,
              "Automatic + nu=0.30 -> displacement-based");
    }
    {
        const PreflightReport report = services.analysis->preflight(analysisId);
        check(report.passed(), "geçerli model: Ready to Solve");
        check(report.checks.size() >= 8, "preflight en az 8 kontrol raporluyor");
    }

    // =======================================================================
    section("8 — Clear Generated Mesh / Clear Solution (§45)");
    {
        check(window.runCommand(QStringLiteral("analysis.solve")), "yeniden çözüldü");
        settle(400);
        check(services.analysis->hasResults(analysisId), "sonuç mevcut");
        const int resultDefinitions = window.objectsOfType(ObjectType::TotalDeformation).size()
            + window.objectsOfType(ObjectType::EquivalentStress).size()
            + window.objectsOfType(ObjectType::ReactionForce).size();

        check(window.runCommand(QStringLiteral("analysis.clearSolution")), "Clear Solution çalıştı");
        settle(120);
        check(!services.analysis->hasResults(analysisId), "hesaplanmış sonuç temizlendi");
        check(window.firstObjectOfType(ObjectType::Analysis) != InvalidObjectId, "analiz korundu");
        check(window.firstObjectOfType(ObjectType::FixedSupport) != InvalidObjectId, "sınır şartı korundu");
        check(window.firstObjectOfType(ObjectType::Force) != InvalidObjectId, "yük korundu");
        const int afterDefinitions = window.objectsOfType(ObjectType::TotalDeformation).size()
            + window.objectsOfType(ObjectType::EquivalentStress).size()
            + window.objectsOfType(ObjectType::ReactionForce).size();
        check(afterDefinitions == resultDefinitions, "sonuç TANIMLARI korundu");
    }
    {
        const MeshService::Definition before = services.mesh->definition();
        check(window.runCommand(QStringLiteral("mesh.clearGenerated")), "Clear Generated Mesh çalıştı");
        settle(120);
        check(!services.mesh->hasMesh(), "üretilmiş düğüm/eleman silindi");
        check(services.mesh->definition().nx == before.nx && services.mesh->definition().ny == before.ny
                  && services.mesh->definition().nz == before.nz,
              "mesh bölme tanımı korundu");
        check(nearlyEqual(services.mesh->definition().lengthMm, before.lengthMm), "mesh ölçü tanımı korundu");
        check(window.runCommand(QStringLiteral("mesh.generate")), "mesh yeniden üretildi");
        settle(300);
    }

    // =======================================================================
    section("9 — Dirty / Clean doküman durumu (§42)");
    QTemporaryDir temporary;
    check(temporary.isValid(), "geçici dizin oluşturuldu");
    const QString projectPath = temporary.filePath(QStringLiteral("selftest.femcae.json"));
    {
        check(document->isDirty(), "düzenlemelerden sonra doküman kirli");
        check(window.saveProjectToPath(projectPath), "proje kaydedildi");
        check(!document->isDirty(), "kaydetme sonrası doküman temiz");

        const ObjectId forceId = window.firstObjectOfType(ObjectType::Force);
        const LoadDefinition *before = services.analysis->load(forceId);
        LoadDefinition after = *before;
        after.fyN = 42.0;
        document->push(new commands::SetForceCommand(services, forceId, *before, after));
        settle(60);
        check(document->isDirty(), "düzenleme sonrası tekrar kirli");
        stack->undo();
        settle(60);
        check(!document->isDirty(), "kaydedilen noktaya Undo ile dönünce temiz");
    }

    // =======================================================================
    section("10 — Tam nesne kalıcılığı round-trip (§44)");
    {
        // Zengin bir test projesi kur: 2 malzeme, 2 mesnet, 2 yük (biri
        // bastırılmış), analiz ayarları ve sonuç tanımları.
        window.runCommand(QStringLiteral("material.create"));
        settle(60);
        window.runCommand(QStringLiteral("analysis.insertSupport"));
        settle(60);
        window.runCommand(QStringLiteral("analysis.insertForce"));
        settle(60);

        const QVector<ObjectId> forces = window.objectsOfType(ObjectType::Force);
        check(forces.size() >= 2, "iki yük tanımlandı");
        LoadDefinition first = *services.analysis->load(forces.at(0));
        first.fxN = 100.0;
        first.fyN = 0.0;
        first.fzN = 0.0;
        document->push(new commands::SetForceCommand(services, forces.at(0),
                                                     *services.analysis->load(forces.at(0)), first));
        LoadDefinition second = *services.analysis->load(forces.at(1));
        second.fxN = 0.0;
        second.fyN = 200.0;
        second.fzN = 50.0;
        second.scope = BoxFace::YMax;
        document->push(new commands::SetForceCommand(services, forces.at(1),
                                                     *services.analysis->load(forces.at(1)), second));
        settle(60);

        const QVector<ObjectId> supports = window.objectsOfType(ObjectType::FixedSupport);
        check(supports.size() >= 2, "iki sınır şartı tanımlandı");
        window.setObjectSuppressed(supports.at(1), true);
        settle(60);
        document->push(new commands::SetIncompressibilityCommand(
            services, analysisId, services.analysis->analysis(analysisId)->incompressibility,
            IncompressibilityIntent::Compressible));
        settle(60);

        // Beklenen durumu topla
        struct Expected {
            ObjectId id;
            QString name;
            int row;
        };
        QVector<Expected> expectedForces;
        for (const ObjectId id : window.objectsOfType(ObjectType::Force)) {
            expectedForces.push_back({id, services.project->object(id)->name, services.project->rowOf(id)});
        }
        QVector<Expected> expectedMaterials;
        for (const ObjectId id : window.objectsOfType(ObjectType::Material)) {
            expectedMaterials.push_back({id, services.project->object(id)->name, services.project->rowOf(id)});
        }
        QVector<ObjectId> expectedResults;
        for (const ObjectType type : {ObjectType::TotalDeformation, ObjectType::EquivalentStress,
                                      ObjectType::ReactionForce}) {
            for (const ObjectId id : window.objectsOfType(type)) {
                expectedResults.push_back(id);
            }
        }
        const ObjectId suppressedSupport = supports.at(1);
        const double expectedFx = services.analysis->load(forces.at(0))->fxN;
        const double expectedFy = services.analysis->load(forces.at(1))->fyN;
        const double expectedFz = services.analysis->load(forces.at(1))->fzN;
        const BoxFace expectedScope = services.analysis->load(forces.at(1))->scope;
        const IncompressibilityIntent expectedIntent =
            services.analysis->analysis(analysisId)->incompressibility;

        const QString richPath = temporary.filePath(QStringLiteral("selftest-rich.femcae.json"));
        check(window.saveProjectToPath(richPath), "zengin proje kaydedildi");

        window.newProjectWithoutPrompt();
        settle(120);
        check(window.objectsOfType(ObjectType::Force).size() == 1, "RESET: yeni proje varsayılana döndü");

        check(window.openProjectFromPath(richPath), "zengin proje yeniden açıldı");
        settle(150);

        // ObjectId, DisplayName, ordering
        const QVector<ObjectId> reopenedForces = window.objectsOfType(ObjectType::Force);
        check(reopenedForces.size() == expectedForces.size(), "yük sayısı round-trip");
        bool identityOk = reopenedForces.size() == expectedForces.size();
        bool orderingOk = identityOk;
        for (int i = 0; identityOk && i < expectedForces.size(); ++i) {
            identityOk = identityOk && reopenedForces.at(i) == expectedForces.at(i).id
                && services.project->object(reopenedForces.at(i))->name == expectedForces.at(i).name;
            orderingOk = orderingOk && services.project->rowOf(reopenedForces.at(i)) == expectedForces.at(i).row;
        }
        check(identityOk, "yük ObjectId ve DisplayName round-trip");
        check(orderingOk, "yük ordering round-trip");

        const QVector<ObjectId> reopenedMaterials = window.objectsOfType(ObjectType::Material);
        bool materialsOk = reopenedMaterials.size() == expectedMaterials.size();
        for (int i = 0; materialsOk && i < expectedMaterials.size(); ++i) {
            materialsOk = reopenedMaterials.at(i) == expectedMaterials.at(i).id
                && services.project->object(reopenedMaterials.at(i))->name == expectedMaterials.at(i).name;
        }
        check(materialsOk, "malzeme kimliği ve adı round-trip");

        const ObjectId reopenedAnalysis = window.firstObjectOfType(ObjectType::Analysis);
        check(reopenedAnalysis == analysisId, "analiz ObjectId round-trip");
        check(services.analysis->analysis(reopenedAnalysis) != nullptr
                  && services.analysis->analysis(reopenedAnalysis)->incompressibility == expectedIntent,
              "analiz ayarı (incompressibility) round-trip");

        check(services.analysis->load(reopenedForces.at(0)) != nullptr
                  && nearlyEqual(services.analysis->load(reopenedForces.at(0))->fxN, expectedFx),
              "Force 1 Fx round-trip");
        check(services.analysis->load(reopenedForces.at(1)) != nullptr
                  && nearlyEqual(services.analysis->load(reopenedForces.at(1))->fyN, expectedFy)
                  && nearlyEqual(services.analysis->load(reopenedForces.at(1))->fzN, expectedFz),
              "Force 2 Fy/Fz round-trip");
        check(services.analysis->load(reopenedForces.at(1)) != nullptr
                  && services.analysis->load(reopenedForces.at(1))->scope == expectedScope,
              "Force 2 scope round-trip");
        check(services.project->isSuppressed(suppressedSupport), "bastırılmış sınır şartı round-trip");

        QVector<ObjectId> reopenedResults;
        for (const ObjectType type : {ObjectType::TotalDeformation, ObjectType::EquivalentStress,
                                      ObjectType::ReactionForce}) {
            for (const ObjectId id : window.objectsOfType(type)) {
                reopenedResults.push_back(id);
            }
        }
        check(reopenedResults == expectedResults, "sonuç tanımları kimlik ve sıra ile round-trip");
        check(services.project->parentOf(reopenedForces.at(0)) == reopenedAnalysis,
              "ağaç hiyerarşisi round-trip");
        check(!document->isDirty(), "açma sonrası doküman temiz");
    }

    // =======================================================================
    section("11 — görünüm regresyonu (§47)");
    {
        const auto readable = [](const QColor &foreground, const QColor &background) {
            // Basit okunabilirlik ölçütü: lightness farkı yeterli mi?
            return std::abs(foreground.lightnessF() - background.lightnessF()) > 0.25;
        };
        const QColor lightSecondary = ui::secondaryText();
        const QColor lightWindow = QApplication::palette().color(QPalette::Window);
        check(readable(lightSecondary, lightWindow), "Light: ikincil metin okunur");

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        app.styleHints()->setColorScheme(Qt::ColorScheme::Dark);
        settle(220);
        check(ui::isDarkAppearance(), "koyu görünüme geçildi");
        const QColor darkSecondary = ui::secondaryText();
        const QColor darkWindow = QApplication::palette().color(QPalette::Window);
        check(readable(darkSecondary, darkWindow), "Dark: ikincil metin okunur (stale foreground yok)");
        check(darkSecondary.lightnessF() > lightSecondary.lightnessF(),
              "Dark ikincil metin Light'takinden daha açık");
        check(ui::statusColor(ui::StatusTone::UpToDate).lightnessF() > 0.3, "Dark: durum rengi görünür");

        app.styleHints()->setColorScheme(Qt::ColorScheme::Light);
        settle(220);
        check(!ui::isDarkAppearance(), "açık görünüme geri dönüldü");
        const QColor backToLight = ui::secondaryText();
        check(std::abs(backToLight.lightnessF() - lightSecondary.lightnessF()) < 0.02,
              "Light → Dark → Light: ikincil metin ilk değerine döndü (renk kalıntısı yok)");
        app.styleHints()->unsetColorScheme();
        settle(120);
#else
        std::cout << "  SKIP  görünüm zorlama testi Qt 6.8+ gerektirir (mevcut: " << QT_VERSION_STR << ")\n";
#endif
    }

    std::cout << '\n' << (failures == 0 ? "SELFTEST PASS\n" : "SELFTEST FAIL\n");
    std::cout << "checks=" << checksRun << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}

} // namespace d26
