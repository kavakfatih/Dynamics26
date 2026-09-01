#pragma once

// Alt yardımcı çalışma alanı.
//
// Başlangıçta KAPALIDIR ve normal modelleme sırasında viewport alanını tüketmez.
// Yalnız gerçek bir olay onu açar: solver hatası, kullanıcının Results Table'ı
// açması veya Tanılama düğmesi. Kullanıcı kapattıktan sonra yalnız gerçek hata
// durumunda yeniden açılır.

#include "../core/ProjectTypes.h"

#include <QString>
#include <QVector>
#include <QWidget>

class QPlainTextEdit;
class QTabWidget;
class QTableWidget;

namespace d26 {

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
    void setConvergenceRows(const QVector<QStringList> &rows);
    void appendTiming(const QString &operation, double seconds);
    void clearAll();

    void showTab(Tab tab);
    // Kullanıcının paneli kapattığı bilgisini taşır: sadece gerçek hata bunu aşar.
    void noteUserDismissed();
    [[nodiscard]] bool userDismissed() const noexcept { return userDismissed_; }

signals:
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
    QTableWidget *convergence_{nullptr};
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
