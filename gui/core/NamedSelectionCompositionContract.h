#pragma once

// Dynamics26 Alpha.3.6 — application composition contract for persistent scopes.
//
// Bu helper görünür bir UI katmanı değildir. GUI entry point, MainWindow'ın
// kurduğu Project/Geometry/Mesh servislerinden tek NamedSelectionService
// örneğini üretir ve QObject ownership'i MainWindow'a verir. Böylece transient
// SelectionCoordinator'dan bağımsız, document-lifetime persistent scope servisi
// uygulama kompozisyonunda açıkça yaşar.

#include "ServiceContext.h"
#include "../services/NamedSelectionService.h"

#include <QObject>

namespace d26 {

[[nodiscard]] inline NamedSelectionService *createNamedSelectionComposition(
    const ServiceContext &services, QObject *owner)
{
    if (services.project == nullptr || services.geometry == nullptr
        || services.mesh == nullptr || owner == nullptr) {
        return nullptr;
    }
    return new NamedSelectionService(services.project, services.geometry, services.mesh, owner);
}

} // namespace d26
