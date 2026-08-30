#pragma once

// Alt yardımcı çalışma alanı.
//
// Başlangıçta KAPALIDIR ve normal modelleme sırasında viewport alanını tüketmez.
// Yalnız gerçek bir olay onu açar: solver hatası, kullanıcının Results Table'ı
// açması veya Tanılama düğmesi. Kullanıcı kapattıktan sonra yalnız gerçek hata
// durumunda yeniden açılır.

#include "../core/ProjectTypes.h"

#include <QWidget>

class QPlainTextEdit;
class QTabWidget;
class QTableWidget;

namespace d26 {

class UtilityWorkspace final : public QWidget
{
    Q_OBJECT
public:
    enum class Tab { Messages = 0, Convergence, SolverOutput, ResultsTable, Timings };

    explicit UtilityWorkspace(QWidget *parent = nullptr);

    void appendMessage(const QString &text, Severity severity);
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

private:
    QTabWidget *tabs_{nullptr};
    QPlainTextEdit *messages_{nullptr};
    QPlainTextEdit *solverOutput_{nullptr};
    QTableWidget *convergence_{nullptr};
    QTableWidget *results_{nullptr};
    QTableWidget *timings_{nullptr};
    bool userDismissed_{false};
};

} // namespace d26
