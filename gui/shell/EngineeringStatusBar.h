#pragma once

// Mühendislik durum çubuğu.
//
// Genel amaçlı "hazır" mesajı yerine modelin gerçek büyüklüklerini gösterir:
// gövde / eleman / serbestlik derecesi, seçim bilgisi ve çözücü durumu.

#include <QStatusBar>

class QLabel;
class QToolButton;

namespace d26 {

enum class SolverState { Idle, NotReady, Ready, Solving, Solved, Failed };

class EngineeringStatusBar final : public QStatusBar
{
    Q_OBJECT
public:
    explicit EngineeringStatusBar(QWidget *parent = nullptr);

    void setModelStatistics(int bodyCount, int elementCount, int dofCount, bool hasMesh);
    void setSelection(const QString &text);
    // Doküman durumu (Edited) ve güncellik uyarısı (Mesh Out of Date …).
    void setDocumentState(bool dirty, const QString &staleWarning);
    void setSolverState(SolverState state, const QString &detail = QString());
    void setDiagnosticsChecked(bool checked);
    void refreshAppearance();
    [[nodiscard]] QToolButton *diagnosticsButton() const noexcept { return diagnostics_; }

signals:
    void diagnosticsToggled(bool visible);

private:
    QLabel *statistics_{nullptr};
    QLabel *documentState_{nullptr};
    QLabel *staleWarning_{nullptr};
    QLabel *selection_{nullptr};
    QLabel *solverState_{nullptr};
    QToolButton *diagnostics_{nullptr};
};

} // namespace d26
