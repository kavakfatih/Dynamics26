#pragma once

// Semantik CAE ikon seti.
//
// Model ağacı ve komut yüzeyi generic dosya/klasör ikonu kullanmaz. Her ikon
// mühendislik anlamını taşıyan basit bir vektör çizimdir ve mevcut palet
// rengiyle üretildiği için Light/Dark geçişinde doğru kontrastta kalır.
// Ek bir kaynak/dependency gerektirmez; QPainter ile çizilir.

#include "ProjectTypes.h"

#include <QColor>
#include <QIcon>

namespace d26 {

enum class CommandGlyph {
    New, Open, Save,
    ImportGeometry, ReplaceGeometry,
    GenerateMesh,
    InsertSupport, InsertForce,
    Solve, Stop,
    FitView, Isometric,
    SelectBody, SelectFace, SelectEdge, SelectVertex,
    ShowNavigator, ShowDetails, ShowDiagnostics,
    Probe, SectionCut, Export, Measure, NamedSelection, Evaluate
};

namespace CaeIcons {

// Model ağacı nesne ikonu.
[[nodiscard]] QIcon forType(ObjectType type, const QColor &tint);
// Komut yüzeyi ikonu.
[[nodiscard]] QIcon forCommand(CommandGlyph glyph, const QColor &tint);
// Durum rozeti (nesne adının yanında küçük gösterge).
[[nodiscard]] QIcon forState(ObjectState state);

// Palet değiştiğinde (Light <-> Dark) üretilmiş ikonlar geçersizleşir.
void invalidateCache();

} // namespace CaeIcons
} // namespace d26
