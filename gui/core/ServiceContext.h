#pragma once

// Details sayfalarının ve kabuk bileşenlerinin mühendislik servislerine
// EXPLICIT erişimi. Widget ağacında arama veya metin eşleştirme ile nesne
// bulma yapılmaz; bağımlılıklar bu yapı üzerinden açıkça verilir.

namespace d26 {

class ProjectModel;
class GeometryService;
class MeshService;
class NamedSelectionService;
class MaterialService;
class AnalysisService;
class DocumentCommandManager;
class DependencyEngine;

struct ServiceContext {
    ProjectModel *project{nullptr};
    GeometryService *geometry{nullptr};
    MeshService *mesh{nullptr};
    // Persistent engineering scope servisidir. Transient viewport selection'ın
    // sahibi değildir; Named Selection yaşam döngüsü ve kalıcılığı application
    // composition üzerinden açıkça erişilebilir olur.
    NamedSelectionService *namedSelections{nullptr};
    MaterialService *materials{nullptr};
    AnalysisService *analysis{nullptr};
    // Model mutasyonları servisleri DOĞRUDAN çağırmaz; domain command olarak
    // buraya itilir. Böylece her değişiklik undoable ve dirty-state farkındadır.
    DocumentCommandManager *commands{nullptr};
    DependencyEngine *dependencies{nullptr};
};

} // namespace d26
