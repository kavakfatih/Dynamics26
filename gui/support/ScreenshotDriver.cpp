#include "ScreenshotDriver.h"

#include "../shell/Dynamics26MainWindow.h"
#include "../shell/GraphicsWorkspace.h"
#include "../viewport/ViewportWidget.h"
#include "../core/ProjectModel.h"
#include "../core/DocumentCommandManager.h"
#include "../services/AnalysisService.h"
#include "../services/MeshService.h"
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
        shot(QStringLiteral("11-preflight"));
        window.runCommand(QStringLiteral("panel.diagnostics"));
        settle(160);
        window.runCommand(QStringLiteral("mesh.generate"));
        settle(300);
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

    return failures == 0 ? 0 : 1;
}

} // namespace d26
