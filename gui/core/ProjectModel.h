#pragma once

// Dynamics26 — proje nesne grafiği.
//
// Bu sınıf model ağacının TEK sahibidir. Mühendislik verisi (CAD belgesi,
// FEM mesh, sonuç veritabanı, Named Selection scope collection) servislerde
// yaşar; burada yalnız nesne kimliği, türü, adı, ebeveyni ve durumu tutulur.
// Böylece ağaç görünümü ile mühendislik durumu birbirine karışmaz.

#include "ProjectTypes.h"

#include <QHash>
#include <QObject>
#include <QVector>

namespace d26 {

struct ProjectObject {
    ObjectId id{InvalidObjectId};
    ObjectId parent{InvalidObjectId};
    ObjectType type{ObjectType::Model};
    QString name;
    ObjectState state{ObjectState::None};
    QString statusText;
    // Bastırılmış nesne modelde kalır fakat preflight/solve tarafından dikkate
    // alınmaz. Delete ile aynı şey DEĞİLDİR ve undoable'dır.
    bool suppressed{false};
    // Alan bağımlı yük: geometri gövde kimliği, analiz indeksi, mod numarası vb.
    qint64 tag{0};
    QVector<ObjectId> children;
};

class ProjectModel final : public QObject
{
    Q_OBJECT
public:
    explicit ProjectModel(QObject *parent = nullptr);

    // Ağaç bir ormandır: "Project" ve her analiz ayrı bir üst düzey köktür
    // (ANSYS Mechanical'daki Model / Static Structural ayrımı).
    [[nodiscard]] const QVector<ObjectId> &roots() const noexcept { return roots_; }
    [[nodiscard]] const ProjectObject *object(ObjectId id) const noexcept;
    [[nodiscard]] ObjectType typeOf(ObjectId id) const noexcept;
    [[nodiscard]] ObjectId parentOf(ObjectId id) const noexcept;
    [[nodiscard]] const QVector<ObjectId> &childrenOf(ObjectId id) const;
    [[nodiscard]] int rowOf(ObjectId id) const;

    // İlk kez tanımlı boş model iskeletini kurar (Project/Model/Geometry/...).
    void resetToEmptyProject();

    ObjectId addObject(ObjectId parent, ObjectType type, const QString &name, qint64 tag = 0);
    ObjectId addRoot(ObjectType type, const QString &name, qint64 tag = 0);
    // Undo (silmeyi geri alma) ve proje yükleme için: nesneyi TAM olarak eski
    // kimliğiyle ve eski konumuna geri koyar. requestedId = 0 ise yeni kimlik
    // üretilir; row < 0 ise sona eklenir.
    ObjectId addObjectAt(ObjectId parent, int row, ObjectType type, const QString &name, qint64 tag,
                         ObjectId requestedId);
    ObjectId addRootAt(int row, ObjectType type, const QString &name, qint64 tag, ObjectId requestedId);
    void removeObject(ObjectId id);
    void removeChildren(ObjectId id);

    void setName(ObjectId id, const QString &name);
    void setState(ObjectId id, ObjectState state, const QString &statusText = QString());
    void setSuppressed(ObjectId id, bool suppressed);
    [[nodiscard]] bool isSuppressed(ObjectId id) const;
    // Nesnenin kendisi veya bir atası bastırılmışsa etkin olarak bastırılmıştır.
    [[nodiscard]] bool isEffectivelySuppressed(ObjectId id) const;

    // Proje yüklerken kimlik sayacını geri yükler; kaydedilmiş ObjectId'ler
    // yeniden üretilen kimliklerle çakışmaz.
    void reserveIdsUpTo(ObjectId highestUsedId);
    [[nodiscard]] ObjectId peekNextId() const noexcept { return nextId_; }

    // İskeletin sabit düğümlerine hızlı erişim.
    [[nodiscard]] ObjectId projectRoot() const noexcept { return project_; }
    [[nodiscard]] ObjectId modelNode() const noexcept { return model_; }
    [[nodiscard]] ObjectId geometryNode() const noexcept { return geometry_; }
    [[nodiscard]] ObjectId materialsNode() const noexcept { return materials_; }
    [[nodiscard]] ObjectId sectionsNode() const noexcept { return sections_; }
    [[nodiscard]] ObjectId connectionsNode() const noexcept { return connections_; }
    [[nodiscard]] ObjectId meshNode() const noexcept { return mesh_; }
    [[nodiscard]] ObjectId namedSelectionsNode() const noexcept { return namedSelections_; }

    // Verilen düğümden yukarı doğru ilk eşleşen türü bulur (ör. bir sonuç
    // nesnesinden sahibi olan analize çıkmak).
    [[nodiscard]] ObjectId ancestorOfType(ObjectId id, ObjectType type) const;
    [[nodiscard]] QVector<ObjectId> childrenOfType(ObjectId parent, ObjectType type) const;
    [[nodiscard]] QVector<ObjectId> analyses() const;

signals:
    // Yapı değişiklikleri tam yenileme ile yayınlanır: ağaç küçüktür ve
    // kısmi index güncellemelerinin ince hatalarına gerek yoktur.
    void aboutToRestructure();
    void restructured();
    void objectChanged(ObjectId id);

private:
    ObjectId nextId_{1};
    QHash<ObjectId, ProjectObject> objects_;
    QVector<ObjectId> roots_;
    QVector<ObjectId> empty_;

    ObjectId project_{InvalidObjectId};
    ObjectId model_{InvalidObjectId};
    ObjectId geometry_{InvalidObjectId};
    ObjectId materials_{InvalidObjectId};
    ObjectId sections_{InvalidObjectId};
    ObjectId connections_{InvalidObjectId};
    ObjectId mesh_{InvalidObjectId};
    ObjectId namedSelections_{InvalidObjectId};

    void detach(ObjectId id);
    void collectSubtree(ObjectId id, QVector<ObjectId> &out) const;
};

} // namespace d26
