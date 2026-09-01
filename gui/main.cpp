// Dynamics26 GUI giriş noktası — TEK kompozisyon kökü.
//
// Dynamics26MainWindow görünür pencere yerleşiminin tek sahibidir.
// SelectionCoordinator ikinci bir shell/pencere katmanı değildir; yalnız
// transient CAD selection state'ini mevcut servis/navigator/viewport arasında
// açıkça bağlar.
//
// Geliştirici bayrakları (normal kullanımda gerekmez):
//   --bundle-smoke                       macOS bundle audit protokolü
//   --selftest                           genel GUI öz-testi
//   --selection-selftest                 Selection + scope consumers + integrated application acceptance
//   --capture <dizin>                    belgeleme ekran görüntüleri
//   --capture-appearance light|dark      çekim için görünümü sabitler
//   --import-step <dosya>                dosya diyaloğu olmadan STEP yükler

#include "core/ProjectModel.h"
#include "services/AnalysisService.h"
#include "shell/CommandRegistry.h"
#include "shell/Dynamics26MainWindow.h"
#include "shell/SelectionCoordinator.h"
#include "support/BoundaryConsumerAcceptance.h"
#include "support/ContactPersistenceAcceptance.h"
#include "support/ContactPreflightAcceptance.h"
#include "support/ContactShellAcceptance.h"
#include "support/IntegratedWorkflowAcceptance.h"
#include "support/MaterialInspectorAcceptance.h"
#include "support/PreflightAcceptance.h"
#include "support/ScreenshotDriver.h"
#include "support/SelectionAcceptanceTest.h"
#include "support/SelfTest.h"

#include <QApplication>
#include <QSurfaceFormat>

#include <cstring>
#include <iostream>
#include <string>

#ifdef FEMCAE_GUI_HAS_VTK
#include <QVTKOpenGLNativeWidget.h>
#endif

namespace {

bool hasArgument(const int argc, char *argv[], const char *expected)
{
    for (int i = 1; i < argc; ++i) {
        if (argv[i] != nullptr && std::strcmp(argv[i], expected) == 0) {
            return true;
        }
    }
    return false;
}

std::string argumentValue(const int argc, char *argv[], const char *expected)
{
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] != nullptr && std::strcmp(argv[i], expected) == 0) {
            return argv[i + 1];
        }
    }
    return {};
}

const char *buildVersion()
{
#ifdef FEMCAE_APP_VERSION
    return FEMCAE_APP_VERSION;
#else
    return "unknown";
#endif
}

} // namespace

int main(int argc, char *argv[])
{
    // Bu yol QApplication/QPA başlatılmadan önce çalışır. macOS bundle audit
    // protokolü geçmiş release kanıtlarıyla uyumlu kalmalıdır; değiştirilmez.
    if (hasArgument(argc, argv, "--bundle-smoke")) {
        std::cout << "FEMCAE bundle smoke PASS version=" << buildVersion() << '\n';
        return 0;
    }

#ifdef FEMCAE_GUI_HAS_VTK
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
#endif

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Dynamics26"));
    app.setApplicationDisplayName(QStringLiteral("Dynamics26"));
    app.setApplicationVersion(QString::fromLatin1(buildVersion()));
    app.setOrganizationName(QStringLiteral("Dynamics26"));

    // Uygulama macOS System Appearance'ı kullanır. Global QPalette veya
    // uygulama çapında QSS ayarlanmaz; Light/Dark tek kaynaktan gelir.
    d26::Dynamics26MainWindow window;

    // Persistent engineering servisleri MainWindow::buildServices() içinde,
    // DetailsHost gibi ServiceContext kopyalayan consumer'lardan ÖNCE kurulur.
    // Entry point ikinci bir servis üretmez; tek document-lifetime örneklerini
    // yalnızca composition sanity check ile doğrular.
    if (window.services().namedSelections == nullptr || window.services().contacts == nullptr
        || window.services().analysis == nullptr) {
        std::cerr << "FEMCAE composition FAIL: persistent engineering consumer services unavailable\n";
        return 2;
    }

    // AnalysisService boundary-condition ve Contact preflight consumer'ları
    // entity listesi kopyalamaz. Aynı document-lifetime persistent servisleri
    // yalnız pointer dependency olarak bağlanır; ownership MainWindow composition
    // ağacında kalır.
    window.services().analysis->setNamedSelectionService(window.services().namedSelections);
    window.services().analysis->setContactService(window.services().contacts);

    auto *selectionCoordinator = new d26::SelectionCoordinator(&window, &window);
    Q_UNUSED(selectionCoordinator);

    // Alpha.4 structured Utility Preflight presentation. MainWindow kendi
    // command handler'ını constructor sırasında önce bağlamıştır; bu observer
    // command tamamlandıktan sonra AYNI AnalysisService::preflight() raporunu
    // tablo satırlarına dönüştürür. İkinci bir validator veya model state yoktur.
    QObject::connect(window.commandRegistry(), &d26::CommandRegistry::commandTriggered, &window,
                     [&window](const QString &commandId) {
        if (commandId != QStringLiteral("analysis.preflight")) {
            return;
        }
        const d26::ObjectId analysisId = window.currentAnalysis();
        if (analysisId == d26::InvalidObjectId) {
            return;
        }
        const d26::PreflightReport report = window.services().analysis->preflight(analysisId);
        QVector<d26::PreflightUtilityRow> rows;
        rows.reserve(report.checks.size());
        for (const auto &check : report.checks) {
            d26::PreflightUtilityRow row;
            row.status = check.status == d26::PreflightCheck::Status::Passed
                ? QStringLiteral("✓")
                : (check.status == d26::PreflightCheck::Status::Warning
                       ? QStringLiteral("!") : QStringLiteral("✕"));
            row.label = check.label;
            row.detail = check.detail;
            row.subject = check.subject;
            if (row.subject != d26::InvalidObjectId && window.services().project != nullptr) {
                if (const d26::ProjectObject *object = window.services().project->object(row.subject)) {
                    row.subjectLabel = object->name;
                } else {
                    row.subject = d26::InvalidObjectId;
                }
            }
            rows.push_back(row);
        }
        window.utility()->setPreflightRows(rows);
        window.utility()->showTab(d26::UtilityWorkspace::Tab::Preflight);
    });
    QObject::connect(window.utility(), &d26::UtilityWorkspace::preflightSubjectActivated, &window,
                     [&window](const d26::ObjectId subject) {
        if (subject == d26::InvalidObjectId || window.services().project == nullptr
            || window.services().project->object(subject) == nullptr) {
            return;
        }
        window.selectObject(subject);
    });

    // Hosted shell acceptance pencereyi göstermek zorunda değildir; QObject,
    // model ve widget-state signal zinciri görünürlükten bağımsız çalışır. Bu
    // ayrım QVTK/OpenGL'i headless CI'da gereksiz yere ekrana bağlamaz.
    const bool selectionSelfTest = hasArgument(argc, argv, "--selection-selftest");
    if (!selectionSelfTest) {
        window.show();
    }

    // Otomasyon kolaylığı: dosya diyaloğu olmadan STEP yükler.
    const std::string stepPath = argumentValue(argc, argv, "--import-step");
    if (!stepPath.empty()) {
        window.importGeometryFromPath(QString::fromStdString(stepPath));
    }

    if (selectionSelfTest) {
        // Aynı gerçek application composition üzerinde sırasıyla:
        //   1) transient/persistent selection authoring,
        //   2) Beta.1 Material Inspector binding + Undo/dependency contract,
        //   3) Fixed Support / Force persistent scope consumer,
        //   4) Alpha.4 Preflight interaction/structured diagnostics,
        //   5) tam Geometry->...->Solve->Results->Out-of-Date->Undo recovery,
        //   6) Beta.1 Contact Preflight safety gate,
        //   7) Beta.1 Contact project persistence + stale-scope load safety,
        //   8) Beta.1 Contact shell Rename/Delete/Suppress domain routing
        // zinciri yürütülür. Fiziksel pointer/mouse/trackpad kabulünün yerine
        // geçmez.
        const int selectionStatus = d26::runSelectionAcceptanceTest(app, window);
        if (selectionStatus != 0) {
            return selectionStatus;
        }
        const int materialInspectorStatus = d26::runMaterialInspectorAcceptanceTest(app, window);
        if (materialInspectorStatus != 0) {
            return materialInspectorStatus;
        }
        const int boundaryStatus = d26::runBoundaryConsumerAcceptanceTest(app, window);
        if (boundaryStatus != 0) {
            return boundaryStatus;
        }
        const int preflightStatus = d26::runPreflightAcceptanceTest(app, window);
        if (preflightStatus != 0) {
            return preflightStatus;
        }
        const int integratedStatus = d26::runIntegratedWorkflowAcceptanceTest(app, window);
        if (integratedStatus != 0) {
            return integratedStatus;
        }
        const int contactPreflightStatus = d26::runContactPreflightAcceptanceTest(app, window);
        if (contactPreflightStatus != 0) {
            return contactPreflightStatus;
        }
        const int contactPersistenceStatus = d26::runContactPersistenceAcceptanceTest(app, window);
        if (contactPersistenceStatus != 0) {
            return contactPersistenceStatus;
        }
        return d26::runContactShellAcceptanceTest(app, window);
    }

    if (hasArgument(argc, argv, "--selftest")) {
        // Geliştirici öz-testi: gerçek akışı çalıştırır ve sonucu doğrular.
        return d26::runSelfTest(app, window);
    }

    const std::string captureDirectory = argumentValue(argc, argv, "--capture");
    if (!captureDirectory.empty()) {
        // Geliştirici modu: belgelenmiş ekran görüntülerini üretir ve çıkar.
        const std::string appearance = argumentValue(argc, argv, "--capture-appearance");
        return d26::runScreenshotDriver(app, window, QString::fromStdString(captureDirectory),
                                        QString::fromStdString(appearance));
    }

    return app.exec();
}
