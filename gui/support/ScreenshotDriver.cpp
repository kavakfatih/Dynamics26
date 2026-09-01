#include "ScreenshotDriver.h"

#include "../shell/Dynamics26MainWindow.h"
#include "../shell/GraphicsWorkspace.h"
#include "../viewport/ViewportWidget.h"
#include "../core/ProjectModel.h"
#include "../core/DocumentCommandManager.h"
#include "../core/SelectionTypes.h"
#include "../services/AnalysisService.h"
#include "../services/MeshService.h"
#include "../services/NamedSelectionService.h"
#include "../shell/ProjectNavigator.h"
#include "../commands/DomainCommands.h"

#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QMenu>
#include <QPixmap>
#include <QPushButton>
#include <QToolButton>
#include <QUndoStack>
#include <QStyleHints>
#include <QtGlobal>
#include <QTimer>

#include <iostream>

namespace d26 {
namespace {

void settle(const int milliseconds)
{
    // VTK render + Qt yerleşiminin oturması için kısa bir olay döngüsü.
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
    QApplication::processEvents(QEventLoop::AllEvents, 60);
}

// Menüler ayrı üst-düzey pencerelerdir; pencere yakalamasına dahil olmazlar.
// Gerçek menü widget'ı ayrıca yakalanıp doğru konuma bindirilir.
bool captureWithMenu(Dynamics26MainWindow &window, const QString &path, QMenu *menu, const QPoint &windowPosition);

bool capture(Dynamics26MainWindow &window, const QString &path)
{
    return captureWithMenu(window, path, nullptr, {});
}

bool captureWithMenu(Dynamics26MainWindow &window, const QString &path, QMenu *menu, const QPoint &windowPosition)
{
    QPixmap shot = window.grab();
    // QOpenGLWidget içeriği pencere yakalamasında eksik kalabilir; VTK render
    // penceresi ayrıca okunup viewport dikdörtgenine bindirilir.
    ViewportWidget *viewport = window.graphics()->viewport();
    const QImage rendered = viewport->grabRenderedImage();
    if (!rendered.isNull()) {
        QPainter painter(&shot);
        const QPoint topLeft = viewport->mapTo(&window, QPoint(0, 0));
        painter.drawImage(QRect(topLeft, viewport->size()), rendered);
    }
    if (menu != nullptr) {
        const QPixmap menuPixmap = menu->grab();
        QPainter painter(&shot);
        painter.drawPixmap(windowPosition, menuPixmap);
    }
    const bool ok = shot.save(path, "PNG");
    std::cout << (ok ? "captured " : "FAILED   ") << path.toStdString() << '\n';
    return ok;
}

} // namespace

int runScreenshotDriver(QApplication &app, Dynamics26MainWindow &window, const QString &outputDirectory,
                        const QString &forcedAppearance)
{
    QDir().mkpath(outputDirectory);
    if (!forcedAppearance.isEmpty()) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        // QStyleHints::setColorScheme Qt 6.8+ API'sidir. Proje minimum Qt
        // sürümü 6.5 olduğu için yalnız belgeleme çekiminde ve sürüm
        // korumasıyla kullanılır; normal çalıştırmada hiç çağrılmaz.
        if (forcedAppearance.compare(QStringLiteral("dark"), Qt::CaseInsensitive) == 0) {
            app.styleHints()->setColorScheme(Qt::ColorScheme::Dark);
        } else if (forcedAppearance.compare(QStringLiteral("light"), Qt::CaseInsensitive) == 0) {
            app.styleHints()->setColorScheme(Qt::ColorScheme::Light);
        }
#else
        std::cout << "note: --capture-appearance requires Qt 6.8+; using system appearance\n";
#endif
    }
    settle(260);
    const bool dark = app.palette().color(QPalette::Window).lightnessF() < 0.5;
    const QString suffix = dark ? QStringLiteral("dark") : QStringLiteral("light");

    window.resize(1560, 960);
    settle(700);

    int failures = 0;
    const auto shot = [&](const QString &name) {
        if (!capture(window, QStringLiteral("%1/%2-%3.png").arg(outputDirectory, name, suffix))) {
            ++failures;
        }
    };

    // 1) Açılış ekranı — Geometry seçili.
    window.selectObject(window.services().project->geometryNode());
    settle(320);
    shot(QStringLiteral("01-initial"));

    // 2) Geometry / Body.
    const ObjectId body = window.firstObjectOfType(ObjectType::Body);
    if (body != InvalidObjectId) {
        window.selectObject(body);
        settle(320);
    }
    shot(QStringLiteral("02-geometry"));

    // 3) Mesh — gerçek structured HEX8 üretimi.
    window.selectObject(window.services().project->meshNode());
    settle(200);
    window.runCommand(QStringLiteral("mesh.generate"));
    settle(420);
    window.selectObject(window.services().project->meshNode());
    settle(320);
    shot(QStringLiteral("03-mesh"));

    // 4) Static Structural analizi.
    const ObjectId analysis = window.firstObjectOfType(ObjectType::Analysis);
    if (analysis != InvalidObjectId) {
        window.selectObject(analysis);
        settle(320);
    }
    shot(QStringLiteral("04-static-structural"));

    // 5) Sınır şartı.
    const ObjectId support = window.firstObjectOfType(ObjectType::FixedSupport);
    if (support != InvalidObjectId) {
        window.selectObject(support);
        settle(320);
    }
    shot(QStringLiteral("05-boundary-condition"));

    // 6) Çözüm ve sonuç — gerçek Fortran çözümü çalışır.
    window.runCommand(QStringLiteral("analysis.solve"));
    settle(600);
    const ObjectId stress = window.firstObjectOfType(ObjectType::EquivalentStress);
    if (stress != InvalidObjectId) {
        window.selectObject(stress);
        settle(500);
    }
    shot(QStringLiteral("06-solution-result"));

    // 7) Toplam deformasyon sonucu.
    const ObjectId deformation = window.firstObjectOfType(ObjectType::TotalDeformation);
    if (deformation != InvalidObjectId) {
        window.selectObject(deformation);
        settle(420);
    }
    shot(QStringLiteral("07-total-deformation"));

    // 8) Alt yardımcı alan açık (solver çıktısı).
    window.runCommand(QStringLiteral("panel.diagnostics"));
    settle(360);
    shot(QStringLiteral("08-utility-workspace"));
    window.runCommand(QStringLiteral("panel.diagnostics"));
    settle(200);

    // 9) Nesne türüne duyarlı bağlam menüsü (Force).
    {
        const ObjectId force = window.firstObjectOfType(ObjectType::Force);
        if (force != InvalidObjectId) {
            window.selectObject(force);
            settle(220);
            QMenu *menu = window.buildContextMenu(force, &window);
            if (menu != nullptr) {
                const QPoint position(300, 470);
                menu->popup(window.mapToGlobal(position));
                settle(260);
                if (!captureWithMenu(window, QStringLiteral("%1/09-context-menu-%2.png").arg(outputDirectory, suffix),
                                     menu, position)) {
                    ++failures;
                }
                menu->hide();
                menu->deleteLater();
                settle(120);
            }
        }
    }

    // 10) Undo/Redo — Edit menüsü dinamik komut metniyle.
    {
        window.runCommand(QStringLiteral("analysis.insertForce"));
        settle(240);
        if (QMenu *edit = window.editMenu()) {
            const QPoint position(70, 34);
            edit->popup(window.mapToGlobal(position));
            settle(260);
            if (!captureWithMenu(window, QStringLiteral("%1/10-undo-redo-%2.png").arg(outputDirectory, suffix), edit,
                                 position)) {
                ++failures;
            }
            edit->hide();
            settle(120);
        }
        window.documentCommands()->stack()->undo();
        settle(200);
    }

    // 11) Preflight — kasıtlı olarak eksik model üzerinde doğrulama raporu.
    // Alpha.4 foundation acceptance:
    //   1) "Göster" engineering state'i değiştirmeden Mesh object'ine gider.
    //   2) Mesh subject'e ait "Mesh Üret" quick-fix'i canonical mesh.generate
    //      komutunu kullanır ve modeli yeniden Ready to Solve durumuna getirir.
    //   3) Eksik aktif mesnet/yük quick-fix'leri undoable Insert komutlarıdır.
    //   4) BC Named Selection referansı tek adımda persistent scope nesnesine gider.
    {
        window.runCommand(QStringLiteral("mesh.clearGenerated"));
        settle(240);
        const ObjectId analysis = window.firstObjectOfType(ObjectType::Analysis);
        if (analysis != InvalidObjectId) {
            window.selectObject(analysis);
            settle(200);
        }
        window.runCommand(QStringLiteral("analysis.preflight"));
        settle(320);

        const ObjectId meshObject = window.services().project->meshNode();
        const int undoBeforeNavigation = window.documentCommands()->stack()->index();
        auto *showMesh = window.findChild<QToolButton *>(
            QStringLiteral("Dynamics26PreflightSubject_%1").arg(static_cast<qulonglong>(meshObject)));
        if (showMesh == nullptr) {
            std::cerr << "FAILED   actionable Preflight Mesh control missing\n";
            ++failures;
        } else {
            showMesh->click();
            settle(220);
            if (window.navigator()->selectedObject() != meshObject) {
                std::cerr << "FAILED   actionable Preflight did not focus Mesh project object\n";
                ++failures;
            }
            if (window.documentCommands()->stack()->index() != undoBeforeNavigation) {
                std::cerr << "FAILED   actionable Preflight navigation mutated document Undo history\n";
                ++failures;
            }
            if (analysis != InvalidObjectId) {
                window.selectObject(analysis);
                settle(180);
            }
        }

        // Quick-fix görseli hata durumunda yakalanır; ardından aynı native control
        // gerçekten çalıştırılır. Derived mesh generation document Undo işlemi
        // değildir, dolayısıyla Undo index'i sabit kalmalıdır.
        shot(QStringLiteral("11-preflight"));
        const int undoBeforeFix = window.documentCommands()->stack()->index();
        auto *fixMesh = window.findChild<QToolButton *>(QStringLiteral("Dynamics26PreflightFixMesh"));
        if (fixMesh == nullptr) {
            std::cerr << "FAILED   Preflight Mesh quick-fix control missing\n";
            ++failures;
        } else {
            fixMesh->click();
            settle(420);
            if (!window.services().mesh->hasMesh() || window.services().mesh->isOutOfDate()) {
                std::cerr << "FAILED   Preflight Mesh quick-fix did not generate a current FEM mesh\n";
                ++failures;
            }
            if (analysis != InvalidObjectId && !window.services().analysis->preflight(analysis).passed()) {
                std::cerr << "FAILED   Preflight Mesh quick-fix did not restore Ready to Solve\n";
                ++failures;
            }
            if (window.documentCommands()->stack()->index() != undoBeforeFix) {
                std::cerr << "FAILED   Preflight Mesh quick-fix mutated document Undo history\n";
                ++failures;
            }
        }

        // Aktif mesnet yok: quick-fix yeni Fixed Support oluşturmalı ve işlem
        // document Undo history'sinde tek engineering transaction olmalıdır.
        const ObjectId originalSupport = window.firstObjectOfType(ObjectType::FixedSupport);
        if (originalSupport == InvalidObjectId || analysis == InvalidObjectId) {
            std::cerr << "FAILED   Preflight support quick-fix fixture missing support/analysis\n";
            ++failures;
        } else {
            window.setObjectSuppressed(originalSupport, true);
            settle(220);
            window.selectObject(analysis);
            settle(220);
            const int supportCountBefore = window.objectsOfType(ObjectType::FixedSupport).size();
            const int undoBeforeSupportFix = window.documentCommands()->stack()->index();
            auto *fixSupport = window.findChild<QToolButton *>(QStringLiteral("Dynamics26PreflightFixSupport"));
            if (fixSupport == nullptr) {
                std::cerr << "FAILED   Preflight Fixed Support quick-fix control missing\n";
                ++failures;
            } else {
                fixSupport->click();
                settle(280);
                if (window.objectsOfType(ObjectType::FixedSupport).size() != supportCountBefore + 1) {
                    std::cerr << "FAILED   Preflight Fixed Support quick-fix did not insert support\n";
                    ++failures;
                }
                if (window.documentCommands()->stack()->index() != undoBeforeSupportFix + 1) {
                    std::cerr << "FAILED   Preflight Fixed Support quick-fix is not one Undo transaction\n";
                    ++failures;
                }
                if (!window.services().analysis->preflight(analysis).passed()) {
                    std::cerr << "FAILED   Preflight Fixed Support quick-fix did not restore readiness\n";
                    ++failures;
                }
                window.documentCommands()->stack()->undo();
                settle(180);
            }
            window.documentCommands()->stack()->undo();
            settle(220);
            if (!window.services().analysis->preflight(analysis).passed()) {
                std::cerr << "FAILED   Undo after support quick-fix did not restore original valid model\n";
                ++failures;
            }
        }

        // Aktif yük yok: quick-fix yeni Force oluşturmalı; Undo ile insertion ve
        // suppress işlemleri geri alındığında başlangıçtaki valid model korunmalı.
        const ObjectId originalForce = window.firstObjectOfType(ObjectType::Force);
        if (originalForce == InvalidObjectId || analysis == InvalidObjectId) {
            std::cerr << "FAILED   Preflight force quick-fix fixture missing force/analysis\n";
            ++failures;
        } else {
            window.setObjectSuppressed(originalForce, true);
            settle(220);
            window.selectObject(analysis);
            settle(220);
            const int forceCountBefore = window.objectsOfType(ObjectType::Force).size();
            const int undoBeforeForceFix = window.documentCommands()->stack()->index();
            auto *fixForce = window.findChild<QToolButton *>(QStringLiteral("Dynamics26PreflightFixForce"));
            if (fixForce == nullptr) {
                std::cerr << "FAILED   Preflight Force quick-fix control missing\n";
                ++failures;
            } else {
                fixForce->click();
                settle(280);
                if (window.objectsOfType(ObjectType::Force).size() != forceCountBefore + 1) {
                    std::cerr << "FAILED   Preflight Force quick-fix did not insert load\n";
                    ++failures;
                }
                if (window.documentCommands()->stack()->index() != undoBeforeForceFix + 1) {
                    std::cerr << "FAILED   Preflight Force quick-fix is not one Undo transaction\n";
                    ++failures;
                }
                if (!window.services().analysis->preflight(analysis).passed()) {
                    std::cerr << "FAILED   Preflight Force quick-fix did not restore readiness\n";
                    ++failures;
                }
                window.documentCommands()->stack()->undo();
                settle(180);
            }
            window.documentCommands()->stack()->undo();
            settle(220);
            if (!window.services().analysis->preflight(analysis).passed()) {
                std::cerr << "FAILED   Undo after force quick-fix did not restore original valid model\n";
                ++failures;
            }
        }

        // BC/Force persistent reference navigation. Fixture deliberately uses a
        // Mesh Node Named Selection; consumer için wrong-domain olsa da referans
        // nesnesi gerçektir ve Details "Göster" ile tam ObjectId'ye gitmelidir.
        // Navigation document state değildir; dangling referans ise tıklanamaz.
        if (originalSupport != InvalidObjectId && window.services().mesh->hasMesh()) {
            const auto &mesh = window.services().mesh->mesh();
            if (mesh.nodes.empty()) {
                std::cerr << "FAILED   boundary Named Selection navigation fixture has no FEM node\n";
                ++failures;
            } else {
                SelectionItem node;
                node.domain = SelectionDomain::Mesh;
                node.kind = SelectionKind::Node;
                node.meshEntityId = mesh.nodes.front().id;
                node.sourceRevision = window.services().mesh->generation();
                const NamedSelectionCreateResult referenced = window.services().namedSelections->createFromSelection(
                    QVector<SelectionItem>{node}, QStringLiteral("BC Navigation Fixture"));
                if (!referenced.success()) {
                    std::cerr << "FAILED   boundary Named Selection navigation fixture could not create scope\n";
                    ++failures;
                } else {
                    const SupportDefinition originalDefinition = *window.services().analysis->support(originalSupport);
                    SupportDefinition referencedDefinition = originalDefinition;
                    referencedDefinition.scopingMethod = BoundaryScopingMethod::NamedSelection;
                    referencedDefinition.namedSelectionId = referenced.id;
                    window.services().analysis->updateSupport(originalSupport, referencedDefinition);
                    window.selectObject(originalSupport);
                    settle(260);

                    auto *showReferenced = window.findChild<QToolButton *>(
                        QStringLiteral("Dynamics26BoundaryShowNamedSelection"));
                    const int undoBeforeReferenceNavigation = window.documentCommands()->stack()->index();
                    if (showReferenced == nullptr || !showReferenced->isEnabled()) {
                        std::cerr << "FAILED   boundary Named Selection navigation control unavailable\n";
                        ++failures;
                    } else {
                        showReferenced->click();
                        settle(220);
                        if (window.navigator()->selectedObject() != referenced.id) {
                            std::cerr << "FAILED   boundary Named Selection navigation did not focus referenced scope\n";
                            ++failures;
                        }
                        if (window.documentCommands()->stack()->index() != undoBeforeReferenceNavigation) {
                            std::cerr << "FAILED   boundary Named Selection navigation mutated Undo history\n";
                            ++failures;
                        }
                    }

                    SupportDefinition danglingDefinition = originalDefinition;
                    danglingDefinition.scopingMethod = BoundaryScopingMethod::NamedSelection;
                    danglingDefinition.namedSelectionId = static_cast<ObjectId>((quint64{1} << 53) + 991ULL);
                    window.services().analysis->updateSupport(originalSupport, danglingDefinition);
                    window.selectObject(originalSupport);
                    settle(220);
                    auto *showDangling = window.findChild<QToolButton *>(
                        QStringLiteral("Dynamics26BoundaryShowNamedSelection"));
                    if (showDangling == nullptr || showDangling->isEnabled()) {
                        std::cerr << "FAILED   dangling boundary Named Selection navigation must be disabled\n";
                        ++failures;
                    }

                    window.services().analysis->updateSupport(originalSupport, originalDefinition);
                    (void)window.services().namedSelections->remove(referenced.id);
                    settle(180);
                }
            }
        }

        window.runCommand(QStringLiteral("panel.diagnostics"));
        settle(160);
    }

    // 12) Out-of-date bağımlılık durumu: çözümden sonra mesh bölmesi değişir.
    {
        window.runCommand(QStringLiteral("analysis.solve"));
        settle(500);
        MeshService::Definition before = window.services().mesh->definition();
        MeshService::Definition after = before;
        after.nx = before.nx + 6;
        window.documentCommands()->push(new commands::SetMeshDefinitionCommand(
            window.services(), before, after, QStringLiteral("Change Mesh Divisions")));
        settle(320);
        const ObjectId analysis = window.firstObjectOfType(ObjectType::Analysis);
        if (analysis != InvalidObjectId) {
            window.selectObject(analysis);
            settle(240);
        }
        shot(QStringLiteral("12-out-of-date-state"));
    }

    // 13–16) Alpha.3.6 Named Selection native UI kanıtı.
    // Bu fixture yalnız screenshot/audit sürecinde oluşturulur. Persistent scope
    // gerçek MeshEntityId + generation taşır; normal görünümde transient
    // SelectionManager state'i yaratılmaz. Stale çekiminde mesh generation bilerek
    // ilerletilir ve eski numeric ID'lerin seçili gösterilmediği UI gözlenir.
    {
        NamedSelectionService *namedSelections = window.services().namedSelections;
        MeshService *meshService = window.services().mesh;
        if (namedSelections == nullptr || meshService == nullptr || !meshService->generate()) {
            std::cerr << "FAILED   Alpha.3.6 screenshot fixture could not generate mesh\n";
            ++failures;
        } else {
            settle(300);
            const auto &mesh = meshService->mesh();
            if (mesh.nodes.empty()) {
                std::cerr << "FAILED   Alpha.3.6 screenshot fixture has no FEM nodes\n";
                ++failures;
            } else {
                SelectionItem node;
                node.domain = SelectionDomain::Mesh;
                node.kind = SelectionKind::Node;
                node.meshEntityId = mesh.nodes.front().id;
                node.sourceRevision = meshService->generation();

                const NamedSelectionCreateResult created = namedSelections->createFromSelection(
                    QVector<SelectionItem>{node}, QStringLiteral("Audit Node Scope"));
                if (!created.success()) {
                    std::cerr << "FAILED   Alpha.3.6 screenshot fixture could not create Named Selection\n";
                    ++failures;
                } else {
                    window.selectObject(created.id);
                    settle(360);
                    shot(QStringLiteral("13-named-selection"));

                    auto *edit = window.findChild<QPushButton *>(QStringLiteral("Dynamics26NamedSelectionEdit"));
                    if (edit == nullptr) {
                        std::cerr << "FAILED   Named Selection Edit control missing in screenshot fixture\n";
                        ++failures;
                    } else {
                        edit->click();
                        settle(360);
                    }
                    shot(QStringLiteral("14-named-selection-edit"));

                    if (auto *cancel = window.findChild<QPushButton *>(
                            QStringLiteral("Dynamics26NamedSelectionCancel"))) {
                        cancel->click();
                        settle(260);
                    } else {
                        std::cerr << "FAILED   Named Selection Cancel control missing in screenshot fixture\n";
                        ++failures;
                    }

                    if (!meshService->generate()) {
                        std::cerr << "FAILED   Alpha.3.6 screenshot fixture could not advance mesh generation\n";
                        ++failures;
                    }
                    settle(340);
                    // Farklı current object üzerinden geri dönmek gerçek Navigator
                    // kullanıcı akışını taklit eder ve stale persistent context'i
                    // normal view olarak yeniden çözdürür.
                    window.selectObject(window.services().project->meshNode());
                    settle(180);
                    window.selectObject(created.id);
                    settle(360);
                    shot(QStringLiteral("15-named-selection-stale"));

                    edit = window.findChild<QPushButton *>(QStringLiteral("Dynamics26NamedSelectionEdit"));
                    if (edit == nullptr) {
                        std::cerr << "FAILED   stale Named Selection Edit control missing in screenshot fixture\n";
                        ++failures;
                    } else {
                        edit->click();
                        settle(360);
                    }
                    shot(QStringLiteral("16-named-selection-stale-edit"));

                    if (auto *cancel = window.findChild<QPushButton *>(
                            QStringLiteral("Dynamics26NamedSelectionCancel"))) {
                        cancel->click();
                        settle(180);
                    }
                }
            }
        }
    }

    return failures == 0 ? 0 : 1;
}

} // namespace d26
