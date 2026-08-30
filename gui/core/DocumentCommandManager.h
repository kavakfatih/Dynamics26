#pragma once

// Doküman komut yöneticisi.
//
// Undo/Redo iki araç çubuğu düğmesi DEĞİLDİR: model üzerindeki her anlamlı
// değişiklik bir DOMAIN COMMAND olarak ifade edilir ve buradaki QUndoStack'e
// itilir. Böylece Undo/Redo, dirty/clean durumu ve bağımlılık yeniden
// değerlendirmesi tek bir yerden yönetilir.
//
// Mutation akışı:
//
//   UI  →  Domain Command  →  Service  →  Domain Object
//                                            ↓
//                              DependencyEngine  →  ProjectTreeModel / Details / Graphics
//
// MODEL STATE  → undoable (buraya girer)
// VIEW STATE   → kamera, seçim, panel görünürlüğü — buraya GİRMEZ
// DERIVED DATA → üretilmiş mesh, hesaplanmış sonuç alanları — buraya GİRMEZ

#include <QObject>
#include <QString>

class QAction;
class QUndoCommand;
class QUndoStack;

namespace d26 {

class DocumentCommandManager final : public QObject
{
    Q_OBJECT
public:
    explicit DocumentCommandManager(QObject *parent = nullptr);

    [[nodiscard]] QUndoStack *stack() const noexcept { return stack_; }

    // Komutu yığına iter ve ilk kez çalıştırır.
    void push(QUndoCommand *command);
    // Birden fazla komutu tek Undo adımında gruplar (transaction).
    void beginMacro(const QString &text);
    void endMacro();

    // Dosya kaydedildiğinde çağrılır: mevcut nokta "temiz" işaretlenir.
    void markSaved();
    // Yeni/Aç sonrası: geçmiş tamamen sıfırlanır ve doküman temiz olur.
    void resetHistory();
    [[nodiscard]] bool isDirty() const;

    [[nodiscard]] QAction *createUndoAction(QObject *parent) const;
    [[nodiscard]] QAction *createRedoAction(QObject *parent) const;

signals:
    // Doküman modified durumu değişti (pencere başlığı, Save durumu).
    void dirtyChanged(bool dirty);
    // Model üzerinde bir komut uygulandı/geri alındı: bağımlılıklar yeniden
    // değerlendirilmeli ve arayüz tazelenmelidir.
    void documentMutated();

private:
    QUndoStack *stack_{nullptr};
    bool lastDirty_{false};
};

} // namespace d26
