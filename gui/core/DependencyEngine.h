#pragma once

// Bağımlılık / güncellik motoru.
//
// Model ağacındaki nesne DURUMLARININ (UpToDate / OutOfDate / Ready /
// NotReady / Warning / Error / Suppressed / Solving) TEK yazarıdır.
// Servisler yalnız kendi verilerini ve nesne adlarını yönetir; durum burada
// merkezî olarak hesaplanır. Böylece iki katman aynı rozeti farklı kurallarla
// yazamaz.
//
// Bağımlılık kuralları (§21):
//
//   Geometri değişti      → Mesh OutOfDate, Solution OutOfDate
//   Mesh ayarı değişti    → Mesh OutOfDate, Solution OutOfDate
//   Malzeme değişti       → Mesh geçerli kalır, Solution OutOfDate
//   Sınır şartı değişti   → Mesh geçerli kalır, Solution OutOfDate
//   Yük değişti           → Mesh geçerli kalır, Solution OutOfDate
//
// Kurallar damga (revision) karşılaştırmasıyla uygulanır: her servis kendi
// revizyonunu artırır, çözüm üretildiği damgaları saklar.

#include "ProjectTypes.h"
#include "ServiceContext.h"

#include <QObject>

namespace d26 {

class DependencyEngine final : public QObject
{
    Q_OBJECT
public:
    explicit DependencyEngine(const ServiceContext &services, QObject *parent = nullptr);

    // Tüm ağacın durumunu yeniden hesaplar ve ProjectModel'e yazar.
    void evaluate();

    // Çözüm sürerken analiz düğümü Solving olarak gösterilir.
    void setSolvingAnalysis(ObjectId analysisId);

    [[nodiscard]] bool meshIsOutOfDate() const;
    [[nodiscard]] bool anySolutionOutOfDate() const;

private:
    void evaluateModel();
    void evaluateAnalysis(ObjectId analysisId);

    ServiceContext services_;
    ObjectId solvingAnalysis_{InvalidObjectId};
};

} // namespace d26
