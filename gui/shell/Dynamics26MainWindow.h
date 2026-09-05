#pragma once

// Dynamics26 ana penceresi — görünür kompozisyonun TEK sahibi.
//
// Bu sınıf dışında hiçbir katman pencere yerleşimini değiştirmez, widget
// taşımaz veya sonradan "düzeltme" uygulamaz. Eski MainWindow → Shell →
// WorkbenchController → UxController → ProductPolish → AppearanceController
// zinciri kaldırılmıştır.
//
// Yerleşim:
//   ┌ komut yüzeyi (global + bağlamsal) ─────────────────────────┐
//   │ Model Ağacı │      3B GRAFİK (baskın)      │   Details     │
//   ├───────────── mühendislik durum çubuğu ─────────────────────┤
//   └ alt yardımcı alan (başlangıçta kapalı) ────────────────────┘

#include "../core/ProjectTypes.h"
#include "../core/ServiceContext.h"
#include "UtilityWorkspace.h"

#include <QJsonObject>
#include <QMainWindow>
#include <QStringList>

class QAction;
class QCloseEvent;
class QLabel;
class QMenu;
class QSplitter;
class QToolBar;
class QToolButton;

namespace d26 {

class AnalysisService;
class CommandRegistry;
class ContactService;
class DependencyEngine;
class DetailsHost;
class DocumentCommandManager;
class EngineeringStatusBar;
class GeometryService;
class GraphicsWorkspace;
class MaterialService;
class MeshService;
class ProjectModel;
class ProjectNavigator;
class SelectionCoordinator;

class Dynamics26MainWindow final : public QMainWindow
{
    Q_OBJECT
public:
    explicit Dynamics26MainWindow(QWidget *parent = nullptr);

    // Otomatik ekran görüntüsü sürücüsünün kullandığı erişimciler.
    [[nodiscard]] ServiceContext services() const noexcept { return services_; }
    [[nodiscard]] ProjectNavigator *navigator() const noexcept { return navigator_; }
    [[nodiscard]] GraphicsWorkspace *graphics() const noexcept { return graphics_; }
    [[nodiscard]] DetailsHost *detailsHost() const noexcept { return details_; }
    [[nodiscard]] UtilityWorkspace *utility() const noexcept { return utility_; }
    [[nodiscard]] SelectionCoordinator *selectionCoordinator() const noexcept;
    void selectObject(ObjectId id);
    [[nodiscard]] ObjectId firstObjectOfType(ObjectType type) const;
    [[nodiscard]] QVector<ObjectId> objectsOfType(ObjectType type) const;
    bool runCommand(const QString &commandId);
    // Dosya diyaloğu açmadan çalışan yollar. Otomasyon/doğrulama içindir;
    // normal kullanıcı akışı ilgili komutları kullanır.
    bool importGeometryFromPath(const QString &path);
    bool saveProjectToPath(const QString &path);
    bool openProjectFromPath(const QString &path);
    void newProjectWithoutPrompt();

    // Öz-test ve otomasyonun ihtiyaç duyduğu erişimciler.
    [[nodiscard]] DocumentCommandManager *documentCommands() const noexcept { return documentCommands_; }
    [[nodiscard]] DependencyEngine *dependencyEngine() const noexcept { return dependencies_; }
    [[nodiscard]] ObjectId currentAnalysis() const { return activeAnalysis(); }
    [[nodiscard]] CommandRegistry *commandRegistry() const noexcept { return commands_; }
    // Bağlam menüsünü kurar (ekran görüntüsü sürücüsü de aynı menüyü kullanır).
    [[nodiscard]] QMenu *buildContextMenu(ObjectId id, QWidget *parent);
    [[nodiscard]] QMenu *editMenu() const noexcept { return editMenu_; }
    void renameObject(ObjectId id, const QString &newName);
    void duplicateObject(ObjectId id);
    void deleteObject(ObjectId id);
    void setObjectSuppressed(ObjectId id, bool suppressed);

protected:
    bool event(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    // SelectionCoordinator görünür ikinci bir shell değildir. Yalnız viewport
    // kaynaklı transient selection'ın mevcut Navigator/Details bağlamını kamera
    // sahnesini yeniden kurmadan senkronlayabilmesi için kabuğun kontrollü
    // composition-helper erişimine sahiptir.
    friend class SelectionCoordinator;
    // Beta.2 B2.5 production verification migration. Helper ayrı engineering
    // state yaratmaz; mevcut private Utility/reporting yüzeyini canonical
    // verification QAction'ları için kullanır.
    friend void runAdvancedNonlinearVerification(Dynamics26MainWindow &window);
    friend void runAdvancedMixedUpVerification(Dynamics26MainWindow &window);
    friend void runAdvancedContactVerification(Dynamics26MainWindow &window);

    void buildServices();
    void buildLayout();
    void buildCommands();
    void buildCommandSurface();
    void buildMenus();
    void wireSignals();

    void handleCommand(const QString &id);
    void handleSelection(ObjectId id);
    void handleGeometryPick(quint64 geometryId);
    void handleResultPick(double worldX, double worldY, double worldZ, qint64 boundaryFacetId);
    void showObjectContextMenu(ObjectId id, const QPoint &globalPosition);
    void updateWindowTitle();

    void syncAll();
    void syncViewport();
    void syncCommandStates();
    void syncContextualSurface();
    void syncStatusBar();
    void syncDocumentState();

    void rebuildGeometryNodes();

    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void revertToSaved();
    [[nodiscard]] bool confirmDiscardChanges();
    void rememberRecentFile(const QString &path);
    void rebuildRecentMenu();
    void importGeometry(bool replace);
    void importSection();
    void generateMesh();
    void solveActiveAnalysis();
    void exportResults(bool vtk);
    void insertAnalysis(AnalysisType type);
    void runVerificationPreset(const QString &id);
    void runPreflight();
    void clearGeneratedMesh();
    void clearSolution();
    void copySelectedObject(bool cut);
    void pasteObject();
    [[nodiscard]] bool clipboardHasObject() const;
    void showKeyboardShortcuts();
    void showSystemInformation();
    void showAbout();

    void showUtility(UtilityWorkspace::Tab tab, bool force);
    void setUtilityVisible(bool visible);
    void reportMessage(const QString &text, Severity severity);
    [[nodiscard]] ObjectId activeAnalysis() const;

    ServiceContext services_{};
    ProjectModel *project_{nullptr};
    GeometryService *geometry_{nullptr};
    MeshService *mesh_{nullptr};
    ContactService *contacts_{nullptr};
    MaterialService *materials_{nullptr};
    AnalysisService *analysis_{nullptr};

    CommandRegistry *commands_{nullptr};
    DocumentCommandManager *documentCommands_{nullptr};
    DependencyEngine *dependencies_{nullptr};
    QAction *undoAction_{nullptr};
    QAction *redoAction_{nullptr};
    QMenu *recentMenu_{nullptr};
    QMenu *editMenu_{nullptr};
    ProjectNavigator *navigator_{nullptr};
    GraphicsWorkspace *graphics_{nullptr};
    DetailsHost *details_{nullptr};
    UtilityWorkspace *utility_{nullptr};
    EngineeringStatusBar *engineeringStatus_{nullptr};

    QToolBar *mainToolBar_{nullptr};
    QToolBar *contextToolBar_{nullptr};
    QToolButton *ribbonGeometry_{nullptr};
    QToolButton *ribbonMaterial_{nullptr};
    QToolButton *ribbonMesh_{nullptr};
    QToolButton *ribbonAnalysis_{nullptr};
    QToolButton *ribbonResults_{nullptr};
    QLabel *contextTitle_{nullptr};
    QSplitter *workspaceSplitter_{nullptr};
    QSplitter *verticalSplitter_{nullptr};

    ObjectId selected_{InvalidObjectId};
    ObjectId activeAnalysis_{InvalidObjectId};
    QString currentProjectPath_;
    QStringList recentFiles_;
    bool suppressSync_{false};
    double tessellationDeflection_{0.15};
    bool showMeshNodes_{false};
    bool solving_{false};
};

} // namespace d26
