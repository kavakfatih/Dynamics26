#pragma once

// Alt yardımcı çalışma alanı.
//
// Başlangıçta KAPALIDIR ve normal modelleme sırasında viewport alanını tüketmez.
// Yalnız gerçek bir olay onu açar: solver hatası, kullanıcının Results Table'ı
// açması veya Tanılama düğmesi. Kullanıcı kapattıktan sonra yalnız gerçek hata
// durumunda yeniden açılır.

#include "../core/ProjectTypes.h"
#include "../core/SolverTelemetry.h"

#include <QString>
#include <QVector>
#include <QWidget>

class QLabel;
class QPlainTextEdit;
class QTabWidget;
class QTableWidget;

namespace d26 {

class ConvergencePlotWidget;

// UtilityWorkspace Preflight görünümü validation sahibi değildir. Bu satırlar
// yalnız AnalysisService::preflight() çıktısının presentation DTO'sudur.
// subject ObjectId decimal-string/Qt integer olarak exact tutulur; görünen ad
// engineering identity'nin yerine geçmez.
struct PreflightUtilityRow {
    QString status;
    QString label;
    QString detail;
    QString subjectLabel;
    ObjectId subject{InvalidObjectId};
};

class UtilityWorkspace final : public QWidget
{
    Q_OBJECT
public:
    enum class Tab { Messages = 0, Preflight, Convergence, SolverOutput, ResultsTable, Timings };

    explicit UtilityWorkspace(QWidget *parent = nullptr);

    void appendMessage(const QString &text, Severity severity);
    void setPreflightRows(const QVector<PreflightUtilityRow> &rows);
    void appendSolverOutput(const QString &text);
    void clearSolverOutput();
    void setResultRows(const QVector<QPair<QString, QString>> &rows);

    // Beta.2 canonical presentation girişi. Typed snapshot document state veya
    // ikinci solver state değildir; yalnız türetilmiş telemetry'yi render eder.
    void setConvergenceData(const SolverConvergenceSnapshot &snapshot);

    void appendTiming(const QString &operation, double seconds);
    void clearAll();

    void showTab(Tab tab);
    // Kullanıcının paneli kapattığı bilgisini taşır: sadece gerçek hata bunu aşar.
    void noteUserDismissed();
    void noteUserOpened();
    [[nodiscard]] bool userDismissed() const noexcept { return userDismissed_; }

signals:
    // Panel kendi bölgesinden tek hareketle tekrar viewport lehine daraltılabilir.
    void collapseRequested();
    void openRequested(Tab tab);
    // Structured Preflight tablosu yalnız navigation isteği üretir. Document
    // state/Undo burada değiştirilmez; MainWindow canonical selectObject yolunu
    // uygular.
    void preflightSubjectActivated(ObjectId subject);

private:
    QTabWidget *tabs_{nullptr};
    QPlainTextEdit *messages_{nullptr};
    QTableWidget *preflight_{nullptr};
    QPlainTextEdit *solverOutput_{nullptr};
    QLabel *convergenceSummary_{nullptr};
    QTableWidget *convergence_{nullptr};
    ConvergencePlotWidget *residualPlot_{nullptr};
    ConvergencePlotWidget *displacementPlot_{nullptr};
    // B2.5 advanced diagnostics temel convergence tablosunun 7 kolonluk B2.1
    // contract'ını bozmaz; ayrı typed presentation yüzeyidir.
    QLabel *diagnosticsSummary_{nullptr};
    QTableWidget *diagnostics_{nullptr};
    // B2.5 coupled diagnostics yalnız real mixed/contact verification consumer'ı
    // veri ürettiğinde dolar; unsupported metric hücreleri explicit unavailable'dır.
    QLabel *coupledDiagnosticsSummary_{nullptr};
    QTableWidget *coupledDiagnostics_{nullptr};
    QTableWidget *results_{nullptr};
    QTableWidget *timings_{nullptr};
    // Legacy MainWindow::runPreflight() önce Messages'a marker + her kontrol
    // satırını gönderir. Structured Preflight ayrıntının tek presentation yüzeyi
    // olduğundan bu geçici echo state'i ayrıntı satırlarını yutar; yalnız final
    // Ready/Failed özeti Messages tarihçesinde kalır.
    bool suppressingPreflightEcho_{false};
    bool userDismissed_{false};
};

} // namespace d26
