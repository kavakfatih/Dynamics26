#include "CaeIcons.h"

#include "UiTheme.h"

#include <QHash>
#include <cmath>
#include <functional>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QPolygonF>

namespace d26 {
namespace {

constexpr int kLogicalSize = 16;

QHash<QString, QIcon> &cache()
{
    static QHash<QString, QIcon> instance;
    return instance;
}

// Tüm çizimler 16x16 logical grid üzerinde tanımlıdır; QPixmap devicePixelRatio
// ile ölçeklenir, böylece Retina ekranda keskin kalır.
QPixmap makeCanvas(qreal dpr)
{
    QPixmap pixmap(static_cast<int>(kLogicalSize * dpr), static_cast<int>(kLogicalSize * dpr));
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);
    return pixmap;
}

void strokeCube(QPainter &p, const QRectF &r, bool solidTopFace)
{
    // İzometrik gövde: CAD katı gösterimi.
    const qreal d = r.width() * 0.26;
    const QRectF front(r.left(), r.top() + d, r.width() - d, r.height() - d);
    QPolygonF top;
    top << front.topLeft() << QPointF(front.left() + d, front.top() - d)
        << QPointF(front.right() + d, front.top() - d) << front.topRight();
    QPolygonF side;
    side << front.topRight() << QPointF(front.right() + d, front.top() - d)
         << QPointF(front.right() + d, front.bottom() - d) << front.bottomRight();
    if (solidTopFace) {
        QColor fill = p.pen().color();
        fill.setAlphaF(0.28f);
        p.setBrush(fill);
    }
    p.drawPolygon(top);
    p.drawPolygon(side);
    p.setBrush(Qt::NoBrush);
    p.drawRect(front);
}

void strokeMeshGrid(QPainter &p, const QRectF &r, int divisions)
{
    p.drawRect(r);
    for (int i = 1; i < divisions; ++i) {
        const qreal fx = r.left() + r.width() * i / divisions;
        const qreal fy = r.top() + r.height() * i / divisions;
        p.drawLine(QPointF(fx, r.top()), QPointF(fx, r.bottom()));
        p.drawLine(QPointF(r.left(), fy), QPointF(r.right(), fy));
    }
}

void strokeGroundSupport(QPainter &p, const QRectF &r)
{
    // Klasik ankastre mesnet sembolü: taban çizgisi + tarama.
    const qreal baseY = r.bottom() - r.height() * 0.22;
    p.drawLine(QPointF(r.left(), baseY), QPointF(r.right(), baseY));
    const int hatches = 4;
    for (int i = 0; i < hatches; ++i) {
        const qreal x = r.left() + r.width() * (i + 0.4) / hatches;
        p.drawLine(QPointF(x, baseY), QPointF(x - r.width() * 0.16, r.bottom()));
    }
    QPolygonF body;
    body << QPointF(r.center().x() - r.width() * 0.22, r.top())
         << QPointF(r.center().x() + r.width() * 0.22, r.top())
         << QPointF(r.center().x() + r.width() * 0.22, baseY)
         << QPointF(r.center().x() - r.width() * 0.22, baseY);
    p.drawPolygon(body);
}

void strokeArrow(QPainter &p, const QPointF &from, const QPointF &to, qreal headSize)
{
    p.drawLine(from, to);
    const QPointF dir = to - from;
    const qreal len = std::hypot(dir.x(), dir.y());
    if (len < 0.001) {
        return;
    }
    const QPointF unit(dir.x() / len, dir.y() / len);
    const QPointF normal(-unit.y(), unit.x());
    QPolygonF head;
    head << to
         << to - unit * headSize + normal * headSize * 0.55
         << to - unit * headSize - normal * headSize * 0.55;
    QColor fill = p.pen().color();
    p.setBrush(fill);
    p.drawPolygon(head);
    p.setBrush(Qt::NoBrush);
}

void strokeContourBands(QPainter &p, const QRectF &r)
{
    // Sonuç konturu: üç bant. Renk semantiği viewport'a aittir; ikon tek renktir.
    for (int i = 0; i < 3; ++i) {
        QPainterPath path;
        const qreal y = r.top() + r.height() * (0.22 + 0.28 * i);
        path.moveTo(r.left(), y);
        path.cubicTo(r.left() + r.width() * 0.33, y - r.height() * 0.20,
                     r.left() + r.width() * 0.66, y + r.height() * 0.20,
                     r.right(), y - r.height() * 0.06);
        p.drawPath(path);
    }
}

void strokeWave(QPainter &p, const QRectF &r)
{
    QPainterPath path;
    path.moveTo(r.left(), r.center().y());
    path.cubicTo(r.left() + r.width() * 0.25, r.top(),
                 r.left() + r.width() * 0.42, r.bottom(),
                 r.center().x() + r.width() * 0.06, r.center().y());
    path.cubicTo(r.left() + r.width() * 0.72, r.top() + r.height() * 0.16,
                 r.right() - r.width() * 0.08, r.bottom() - r.height() * 0.16,
                 r.right(), r.center().y());
    p.drawPath(path);
}

void strokeIBeamSection(QPainter &p, const QRectF &r)
{
    const qreal flange = r.height() * 0.20;
    const qreal web = r.width() * 0.24;
    QPainterPath path;
    path.addRect(QRectF(r.left(), r.top(), r.width(), flange));
    path.addRect(QRectF(r.center().x() - web / 2.0, r.top() + flange, web, r.height() - 2 * flange));
    path.addRect(QRectF(r.left(), r.bottom() - flange, r.width(), flange));
    QColor fill = p.pen().color();
    fill.setAlphaF(0.30f);
    p.fillPath(path.simplified(), fill);
    p.drawPath(path.simplified());
}

void strokeContact(QPainter &p, const QRectF &r)
{
    // İki temas eden yüzey.
    QPainterPath upper;
    upper.moveTo(r.left(), r.top() + r.height() * 0.34);
    upper.cubicTo(r.center().x() - r.width() * 0.10, r.top() + r.height() * 0.34,
                  r.center().x() + r.width() * 0.10, r.top() + r.height() * 0.10,
                  r.right(), r.top() + r.height() * 0.10);
    p.drawPath(upper);
    p.drawLine(QPointF(r.left(), r.top() + r.height() * 0.52), QPointF(r.right(), r.top() + r.height() * 0.52));
    for (int i = 0; i < 4; ++i) {
        const qreal x = r.left() + r.width() * (i + 0.4) / 4.0;
        p.drawLine(QPointF(x, r.top() + r.height() * 0.52), QPointF(x - r.width() * 0.14, r.bottom() - r.height() * 0.12));
    }
}

void strokeMaterialSphere(QPainter &p, const QRectF &r)
{
    QColor fill = p.pen().color();
    fill.setAlphaF(0.26f);
    p.setBrush(fill);
    p.drawEllipse(r);
    p.setBrush(Qt::NoBrush);
    QPainterPath sheen;
    sheen.moveTo(r.left() + r.width() * 0.24, r.top() + r.height() * 0.66);
    sheen.cubicTo(r.left() + r.width() * 0.20, r.top() + r.height() * 0.24,
                  r.left() + r.width() * 0.58, r.top() + r.height() * 0.16,
                  r.left() + r.width() * 0.78, r.top() + r.height() * 0.28);
    p.drawPath(sheen);
}

void strokeGear(QPainter &p, const QRectF &r)
{
    const QPointF c = r.center();
    const qreal outer = r.width() * 0.46;
    const qreal inner = r.width() * 0.30;
    for (int i = 0; i < 6; ++i) {
        const qreal a = i * M_PI / 3.0;
        p.drawLine(c + QPointF(std::cos(a) * inner, std::sin(a) * inner),
                   c + QPointF(std::cos(a) * outer, std::sin(a) * outer));
    }
    p.drawEllipse(c, inner, inner);
}

void strokeDocument(QPainter &p, const QRectF &r)
{
    const qreal fold = r.width() * 0.30;
    QPolygonF outline;
    outline << r.topLeft() << QPointF(r.right() - fold, r.top())
            << QPointF(r.right(), r.top() + fold) << r.bottomRight() << r.bottomLeft();
    p.drawPolygon(outline);
    p.drawLine(QPointF(r.right() - fold, r.top()), QPointF(r.right() - fold, r.top() + fold));
    p.drawLine(QPointF(r.right() - fold, r.top() + fold), QPointF(r.right(), r.top() + fold));
}

void strokeFolderBracket(QPainter &p, const QRectF &r)
{
    // Klasör değil: gruplama parantezi. CAE nesne grubu anlamı taşır.
    const qreal inset = r.width() * 0.16;
    p.drawLine(QPointF(r.left() + inset, r.top()), QPointF(r.left(), r.top()));
    p.drawLine(QPointF(r.left(), r.top()), QPointF(r.left(), r.bottom()));
    p.drawLine(QPointF(r.left(), r.bottom()), QPointF(r.left() + inset, r.bottom()));
    p.drawLine(QPointF(r.right() - inset, r.top()), QPointF(r.right(), r.top()));
    p.drawLine(QPointF(r.right(), r.top()), QPointF(r.right(), r.bottom()));
    p.drawLine(QPointF(r.right(), r.bottom()), QPointF(r.right() - inset, r.bottom()));
}

void strokeNamedSelection(QPainter &p, const QRectF &r)
{
    // Kalıcı scope: CAD gövdesi + sağda kompakt seçim listesi işareti.
    strokeCube(p, r.adjusted(1.0, 2.0, -4.0, -2.0), true);
    p.drawLine(QPointF(r.right() - 3.0, r.top() + 4.0), QPointF(r.right() - 0.8, r.top() + 4.0));
    p.drawLine(QPointF(r.right() - 3.0, r.top() + 7.0), QPointF(r.right() - 0.8, r.top() + 7.0));
}

void paintObjectGlyph(QPainter &p, const ObjectType type, const QRectF &r)
{
    switch (type) {
    case ObjectType::Project:           strokeDocument(p, r.adjusted(1.5, 0.5, -1.5, -0.5)); break;
    case ObjectType::Model:             strokeCube(p, r, false); break;
    case ObjectType::GeometryFolder:    strokeCube(p, r, true); break;
    case ObjectType::Body:              strokeCube(p, r.adjusted(0.8, 0.8, -0.8, -0.8), true); break;
    case ObjectType::MaterialsFolder:
    case ObjectType::Material:          strokeMaterialSphere(p, r.adjusted(1.0, 1.0, -1.0, -1.0)); break;
    case ObjectType::SectionsFolder:
    case ObjectType::Section:           strokeIBeamSection(p, r.adjusted(1.5, 1.0, -1.5, -1.0)); break;
    case ObjectType::ConnectionsFolder: strokeFolderBracket(p, r.adjusted(0.5, 1.5, -0.5, -1.5)); break;
    case ObjectType::ContactRegion:     strokeContact(p, r.adjusted(0.5, 0.5, -0.5, -0.5)); break;
    case ObjectType::NamedSelectionsFolder:
    case ObjectType::NamedSelection:    strokeNamedSelection(p, r); break;
    case ObjectType::Mesh:              strokeMeshGrid(p, r.adjusted(1.0, 1.0, -1.0, -1.0), 3); break;
    case ObjectType::Analysis:
        strokeCube(p, r.adjusted(0.0, 1.5, -4.0, -1.5), false);
        strokeArrow(p, QPointF(r.right() - 3.0, r.top() + 3.0), QPointF(r.right() - 0.5, r.bottom() - 3.0), 3.2);
        break;
    case ObjectType::AnalysisSettings:  strokeGear(p, r.adjusted(1.0, 1.0, -1.0, -1.0)); break;
    case ObjectType::FixedSupport:      strokeGroundSupport(p, r.adjusted(1.5, 1.5, -1.5, -0.5)); break;
    case ObjectType::Force:             strokeArrow(p, QPointF(r.center().x(), r.top() + 1.0), QPointF(r.center().x(), r.bottom() - 1.0), 4.2); break;
    case ObjectType::Solution:          strokeContourBands(p, r.adjusted(1.0, 1.0, -1.0, -1.0)); break;
    case ObjectType::TotalDeformation:  strokeWave(p, r.adjusted(1.0, 2.0, -1.0, -2.0)); break;
    case ObjectType::EquivalentStress:  strokeContourBands(p, r.adjusted(1.0, 1.0, -1.0, -1.0)); break;
    case ObjectType::ReactionForce:
        strokeArrow(p, QPointF(r.center().x(), r.bottom() - 3.0), QPointF(r.center().x(), r.top() + 1.5), 3.6);
        p.drawLine(QPointF(r.left() + 1.5, r.bottom() - 1.5), QPointF(r.right() - 1.5, r.bottom() - 1.5));
        break;
    case ObjectType::ModeShape:         strokeWave(p, r.adjusted(1.0, 2.0, -1.0, -2.0)); break;
    }
}

void paintCommandGlyph(QPainter &p, const CommandGlyph glyph, const QRectF &r)
{
    switch (glyph) {
    case CommandGlyph::New:
        strokeDocument(p, r.adjusted(2.0, 0.5, -2.0, -0.5));
        break;
    case CommandGlyph::Open: {
        QPolygonF tray;
        tray << QPointF(r.left(), r.top() + r.height() * 0.30) << QPointF(r.left() + r.width() * 0.36, r.top() + r.height() * 0.30)
             << QPointF(r.left() + r.width() * 0.46, r.top() + r.height() * 0.44) << QPointF(r.right(), r.top() + r.height() * 0.44)
             << QPointF(r.right(), r.bottom()) << QPointF(r.left(), r.bottom());
        p.drawPolygon(tray);
        break;
    }
    case CommandGlyph::Save: {
        p.drawRect(r.adjusted(1.0, 1.0, -1.0, -1.0));
        p.drawRect(QRectF(r.left() + r.width() * 0.30, r.top() + 1.0, r.width() * 0.40, r.height() * 0.30));
        p.drawRect(QRectF(r.left() + r.width() * 0.24, r.bottom() - r.height() * 0.40, r.width() * 0.52, r.height() * 0.28));
        break;
    }
    case CommandGlyph::ImportGeometry:
        strokeCube(p, r.adjusted(3.5, 1.5, -0.5, -1.5), true);
        strokeArrow(p, QPointF(r.left() + 0.5, r.center().y()), QPointF(r.left() + 4.6, r.center().y()), 3.0);
        break;
    case CommandGlyph::ReplaceGeometry:
        strokeCube(p, r.adjusted(0.5, 3.0, -4.5, -1.0), false);
        strokeArrow(p, QPointF(r.left() + 4.0, r.top() + 2.5), QPointF(r.right() - 0.8, r.top() + 2.5), 3.0);
        break;
    case CommandGlyph::GenerateMesh:
        strokeMeshGrid(p, r.adjusted(1.0, 1.0, -1.0, -1.0), 3);
        break;
    case CommandGlyph::InsertSupport:
        strokeGroundSupport(p, r.adjusted(1.5, 1.5, -1.5, -0.5));
        break;
    case CommandGlyph::InsertForce:
        strokeArrow(p, QPointF(r.center().x(), r.top() + 1.0), QPointF(r.center().x(), r.bottom() - 1.0), 4.2);
        break;
    case CommandGlyph::Solve: {
        QPolygonF play;
        play << QPointF(r.left() + r.width() * 0.26, r.top() + r.height() * 0.14)
             << QPointF(r.right() - r.width() * 0.16, r.center().y())
             << QPointF(r.left() + r.width() * 0.26, r.bottom() - r.height() * 0.14);
        QColor fill = p.pen().color();
        p.setBrush(fill);
        p.drawPolygon(play);
        p.setBrush(Qt::NoBrush);
        break;
    }
    case CommandGlyph::Stop: {
        QColor fill = p.pen().color();
        p.setBrush(fill);
        p.drawRect(r.adjusted(3.0, 3.0, -3.0, -3.0));
        p.setBrush(Qt::NoBrush);
        break;
    }
    case CommandGlyph::FitView: {
        const QRectF inner = r.adjusted(1.0, 1.0, -1.0, -1.0);
        const qreal c = inner.width() * 0.32;
        p.drawLine(inner.topLeft(), inner.topLeft() + QPointF(c, 0));
        p.drawLine(inner.topLeft(), inner.topLeft() + QPointF(0, c));
        p.drawLine(inner.topRight(), inner.topRight() - QPointF(c, 0));
        p.drawLine(inner.topRight(), inner.topRight() + QPointF(0, c));
        p.drawLine(inner.bottomLeft(), inner.bottomLeft() + QPointF(c, 0));
        p.drawLine(inner.bottomLeft(), inner.bottomLeft() - QPointF(0, c));
        p.drawLine(inner.bottomRight(), inner.bottomRight() - QPointF(c, 0));
        p.drawLine(inner.bottomRight(), inner.bottomRight() - QPointF(0, c));
        p.drawRect(inner.adjusted(c * 0.9, c * 0.9, -c * 0.9, -c * 0.9));
        break;
    }
    case CommandGlyph::Isometric:
        strokeCube(p, r.adjusted(1.0, 1.0, -1.0, -1.0), false);
        break;
    case CommandGlyph::SelectBody:
        strokeCube(p, r.adjusted(1.0, 1.0, -1.0, -1.0), true);
        break;
    case CommandGlyph::SelectFace: {
        strokeCube(p, r.adjusted(1.0, 1.0, -1.0, -1.0), false);
        QColor fill = p.pen().color();
        fill.setAlphaF(0.45f);
        p.fillRect(QRectF(r.left() + 1.0, r.top() + 5.2, r.width() - 6.2, r.height() - 6.2), fill);
        break;
    }
    case CommandGlyph::SelectEdge: {
        strokeCube(p, r.adjusted(1.0, 1.0, -1.0, -1.0), false);
        QPen strong = p.pen();
        strong.setWidthF(strong.widthF() * 2.2);
        p.save();
        p.setPen(strong);
        p.drawLine(QPointF(r.left() + 1.0, r.top() + 5.2), QPointF(r.right() - 5.2, r.top() + 5.2));
        p.restore();
        break;
    }
    case CommandGlyph::SelectVertex: {
        strokeCube(p, r.adjusted(1.0, 1.0, -1.0, -1.0), false);
        QColor fill = p.pen().color();
        p.setBrush(fill);
        p.drawEllipse(QPointF(r.left() + 1.0, r.top() + 5.2), 2.0, 2.0);
        p.setBrush(Qt::NoBrush);
        break;
    }
    case CommandGlyph::ShowNavigator:
        p.drawRect(r.adjusted(1.0, 2.0, -1.0, -2.0));
        p.drawLine(QPointF(r.left() + r.width() * 0.40, r.top() + 2.0), QPointF(r.left() + r.width() * 0.40, r.bottom() - 2.0));
        break;
    case CommandGlyph::ShowDetails:
        p.drawRect(r.adjusted(1.0, 2.0, -1.0, -2.0));
        p.drawLine(QPointF(r.left() + r.width() * 0.60, r.top() + 2.0), QPointF(r.left() + r.width() * 0.60, r.bottom() - 2.0));
        break;
    case CommandGlyph::ShowDiagnostics:
        p.drawRect(r.adjusted(1.0, 2.0, -1.0, -2.0));
        p.drawLine(QPointF(r.left() + 1.0, r.bottom() - r.height() * 0.34), QPointF(r.right() - 1.0, r.bottom() - r.height() * 0.34));
        break;
    case CommandGlyph::Probe:
        p.drawEllipse(QPointF(r.center().x() - 1.0, r.center().y() - 1.0), r.width() * 0.26, r.width() * 0.26);
        p.drawLine(QPointF(r.center().x() + 1.2, r.center().y() + 1.2), QPointF(r.right() - 1.5, r.bottom() - 1.5));
        break;
    case CommandGlyph::SectionCut:
        strokeCube(p, r.adjusted(1.0, 1.0, -1.0, -1.0), false);
        p.drawLine(QPointF(r.left() + 0.5, r.bottom() - 2.0), QPointF(r.right() - 0.5, r.top() + 2.0));
        break;
    case CommandGlyph::Export:
        p.drawLine(QPointF(r.left() + 1.5, r.bottom() - 1.5), QPointF(r.right() - 1.5, r.bottom() - 1.5));
        strokeArrow(p, QPointF(r.center().x(), r.bottom() - 4.0), QPointF(r.center().x(), r.top() + 1.5), 3.6);
        break;
    case CommandGlyph::Measure:
        p.drawLine(QPointF(r.left() + 1.5, r.bottom() - 3.0), QPointF(r.right() - 1.5, r.top() + 3.0));
        p.drawLine(QPointF(r.left() + 1.5, r.bottom() - 5.0), QPointF(r.left() + 3.5, r.bottom() - 1.0));
        p.drawLine(QPointF(r.right() - 3.5, r.top() + 5.0), QPointF(r.right() - 1.5, r.top() + 1.0));
        break;
    case CommandGlyph::NamedSelection:
        strokeNamedSelection(p, r);
        break;
    case CommandGlyph::Evaluate:
        strokeContourBands(p, r.adjusted(1.0, 1.0, -1.0, -1.0));
        break;
    }
}

QIcon buildIcon(const QColor &tint, const std::function<void(QPainter &, const QRectF &)> &painter)
{
    QIcon icon;
    for (const qreal dpr : {1.0, 2.0}) {
        QPixmap pixmap = makeCanvas(dpr);
        QPainter p(&pixmap);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPen pen(tint);
        pen.setWidthF(1.25);
        pen.setJoinStyle(Qt::RoundJoin);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        painter(p, QRectF(1.0, 1.0, kLogicalSize - 2.0, kLogicalSize - 2.0));
        p.end();
        icon.addPixmap(pixmap);
    }
    return icon;
}

} // namespace

namespace CaeIcons {

QIcon forType(const ObjectType type, const QColor &tint)
{
    const QString key = QStringLiteral("t%1-%2").arg(static_cast<int>(type)).arg(tint.name(QColor::HexArgb));
    const auto it = cache().constFind(key);
    if (it != cache().constEnd()) {
        return it.value();
    }
    QIcon icon = buildIcon(tint, [type](QPainter &p, const QRectF &r) { paintObjectGlyph(p, type, r); });
    cache().insert(key, icon);
    return icon;
}

QIcon forCommand(const CommandGlyph glyph, const QColor &tint)
{
    const QString key = QStringLiteral("c%1-%2").arg(static_cast<int>(glyph)).arg(tint.name(QColor::HexArgb));
    const auto it = cache().constFind(key);
    if (it != cache().constEnd()) {
        return it.value();
    }
    QIcon icon = buildIcon(tint, [glyph](QPainter &p, const QRectF &r) { paintCommandGlyph(p, glyph, r); });
    cache().insert(key, icon);
    return icon;
}

QIcon forState(const ObjectState state)
{
    if (state == ObjectState::None) {
        return {};
    }
    // Durum rozetleri semantik renk taşır (mühendislik durumu), fakat ton
    // görünüme göre ayarlanır; renk UiTheme'den gelir.
    ui::StatusTone tone = ui::StatusTone::Neutral;
    switch (state) {
    case ObjectState::NotReady:   tone = ui::StatusTone::Neutral; break;
    case ObjectState::Ready:      tone = ui::StatusTone::Ready; break;
    case ObjectState::UpToDate:   tone = ui::StatusTone::UpToDate; break;
    case ObjectState::OutOfDate:  tone = ui::StatusTone::OutOfDate; break;
    case ObjectState::Warning:    tone = ui::StatusTone::Warning; break;
    case ObjectState::Error:      tone = ui::StatusTone::Error; break;
    case ObjectState::Suppressed: tone = ui::StatusTone::Suppressed; break;
    case ObjectState::Solving:    tone = ui::StatusTone::Solving; break;
    case ObjectState::None:       return {};
    }
    const QColor color = ui::statusColor(tone);
    const QString key = QStringLiteral("s%1-%2").arg(static_cast<int>(state)).arg(color.name(QColor::HexArgb));
    const auto it = cache().constFind(key);
    if (it != cache().constEnd()) {
        return it.value();
    }
    QIcon icon;
    for (const qreal dpr : {1.0, 2.0}) {
        QPixmap pixmap(static_cast<int>(8 * dpr), static_cast<int>(8 * dpr));
        pixmap.setDevicePixelRatio(dpr);
        pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(QRectF(1.0, 1.0, 6.0, 6.0));
        p.end();
        icon.addPixmap(pixmap);
    }
    cache().insert(key, icon);
    return icon;
}

void invalidateCache()
{
    cache().clear();
}

} // namespace CaeIcons
} // namespace d26
