#pragma once

// Dynamics26 V1.1.0-beta.1 / B1.2 — Contact/Connections Light-Dark screenshot audit.
//
// Ana ScreenshotDriver'ın mevcut 16 regression sahnesine dokunmadan yalnız
// Contact authoring yüzeylerini ekler. Bu helper normal uygulama akışında
// çağrılmaz; --capture geliştirici modunda gerçek MainWindow/ContactService/
// SelectionCoordinator kompozisyonunu kullanır.

#include "../services/ContactService.h"
#include "../shell/Dynamics26MainWindow.h"
#include "../shell/GraphicsWorkspace.h"
#include "../viewport/ViewportWidget.h"

#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>

#include <iostream>

namespace d26 {
namespace contact_screenshot_detail {

inline void settle(const int milliseconds)
{
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
    QApplication::processEvents(QEventLoop::AllEvents, 60);
}

inline bool capture(Dynamics26MainWindow &window, const QString &path)
{
    QPixmap shot = window.grab();
    ViewportWidget *viewport = window.graphics()->viewport();
    const QImage rendered = viewport->grabRenderedImage();
    if (!rendered.isNull()) {
        QPainter painter(&shot);
        const QPoint topLeft = viewport->mapTo(&window, QPoint(0, 0));
        painter.drawImage(QRect(topLeft, viewport->size()), rendered);
    }
    const bool ok = shot.save(path, "PNG");
    std::cout << (ok ? "captured " : "FAILED   ") << path.toStdString() << '\n';
    return ok;
}

} // namespace contact_screenshot_detail

inline int runContactScreenshotDriver(QApplication &app,
                                      Dynamics26MainWindow &window,
                                      const QString &outputDirectory)
{
    QDir().mkpath(outputDirectory);
    const bool dark = app.palette().color(QPalette::Window).lightnessF() < 0.5;
    const QString suffix = dark ? QStringLiteral("dark") : QStringLiteral("light");
    int failures = 0;

    const auto shot = [&](const QString &name) {
        if (!contact_screenshot_detail::capture(
                window, QStringLiteral("%1/%2-%3.png").arg(outputDirectory, name, suffix))) {
            ++failures;
        }
    };

    ContactService *contacts = window.services().contacts;
    if (contacts == nullptr || window.services().project == nullptr) {
        std::cerr << "FAILED   Contact screenshot fixture missing ContactService/ProjectModel\n";
        return 1;
    }

    // 17) Connections Inspector + canonical contextual create action.
    contacts->clear();
    window.documentCommands()->resetHistory();
    window.selectObject(window.services().project->connectionsNode());
    contact_screenshot_detail::settle(320);
    shot(QStringLiteral("17-connections"));

    // 18) Canonical shell command creates an incomplete Contact and auto-selects
    // the dedicated Contact Inspector. No fake scope is inserted for the visual.
    if (!window.runCommand(QStringLiteral("connections.insertContact"))) {
        std::cerr << "FAILED   Contact screenshot fixture could not execute canonical insert command\n";
        ++failures;
        return failures;
    }
    contact_screenshot_detail::settle(360);
    const ObjectId contactId = contacts->order().isEmpty()
        ? InvalidObjectId : contacts->order().last();
    if (contactId == InvalidObjectId || contacts->byId(contactId) == nullptr) {
        std::cerr << "FAILED   Contact screenshot fixture has no created Contact object\n";
        ++failures;
        return failures;
    }
    shot(QStringLiteral("18-contact-inspector"));

    // 19) Gerçek Contact Source edit session. Parametrik modelde current FEM mesh
    // varsa Mesh/Facet fallback kullanılır; transient selection boş başlar. Amaç
    // Apply/Cancel kontrol hiyerarşisini ve edit durumunu Light/Dark'ta görmek.
    auto *editSource = window.findChild<QPushButton *>(
        QStringLiteral("Dynamics26ContactEditSource"));
    if (editSource == nullptr || !editSource->isEnabled()) {
        std::cerr << "FAILED   Contact Source Edit control unavailable in screenshot fixture\n";
        ++failures;
    } else {
        editSource->click();
        contact_screenshot_detail::settle(360);
        shot(QStringLiteral("19-contact-edit-source"));
        if (auto *cancel = window.findChild<QPushButton *>(
                QStringLiteral("Dynamics26ContactCancelSource"))) {
            if (cancel->isVisible()) {
                cancel->click();
                contact_screenshot_detail::settle(180);
            }
        }
    }

    return failures == 0 ? 0 : 1;
}

} // namespace d26
