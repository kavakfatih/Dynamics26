#pragma once

// Dynamics26 V1.1.0-beta.1 / B1.2 — ContactRegion engineering inspector.
//
// Bu sayfa ProjectModel tree state'ini ikinci kez tutmaz. Name / Source / Target /
// formulation verisi ContactService'ten okunur; kalıcı değişiklikler yalnız
// ContactCommands üzerinden QUndoStack'e gider. Source/Target seçim oturumu
// transient SelectionCoordinator state'idir: pointer seçimi Undo üretmez, yalnız
// Apply Selection persistent ScopeReference komutu oluşturur; Cancel sıfır
// document mutation ile çıkar.

#include "DetailsPage.h"
#include "../commands/ContactCommands.h"
#include "../core/DocumentCommandManager.h"
#include "../core/ProjectModel.h"
#include "../core/ServiceContext.h"
#include "../services/ContactService.h"
#include "../shell/SelectionCoordinator.h"

#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include <QWidget>

namespace d26 {
namespace contact_details_detail {

inline QString domainName(const ScopeReference &scope)
{
    if (scope.entities.isEmpty()) {
        return QStringLiteral("—");
    }
    switch (scope.entities.front().domain) {
    case SelectionDomain::Geometry: return QStringLiteral("Geometry");
    case SelectionDomain::Mesh: return QStringLiteral("Mesh");
    case SelectionDomain::ProjectObject: return QStringLiteral("Project Object");
    }
    return QStringLiteral("—");
}

inline QString kindName(const ScopeReference &scope)
{
    if (scope.entities.isEmpty()) {
        return QStringLiteral("—");
    }
    switch (scope.entities.front().kind) {
    case SelectionKind::Face: return QStringLiteral("Face");
    case SelectionKind::Facet: return QStringLiteral("Facet");
    default: return QStringLiteral("Unsupported");
    }
}

inline QString lifecycleName(const ScopeReference &scope)
{
    if (scope.entities.isEmpty()) {
        return QStringLiteral("—");
    }
    return scope.entities.front().domain == SelectionDomain::Geometry
        ? QObject::tr("CAD Revision %1").arg(scope.sourceRevision)
        : QObject::tr("Mesh Generation %1").arg(scope.sourceRevision);
}

inline SelectionCoordinator *selectionCoordinator(QWidget *widget)
{
    if (widget == nullptr || widget->window() == nullptr) {
        return nullptr;
    }
    // SelectionCoordinator doğrudan MainWindow composition child'ıdır ve Q_OBJECT
    // gerektiren UI widget'ı değildir. Named Selection Inspector ile aynı RTTI
    // yaklaşımı kullanılır; ikinci coordinator/picker yaratılmaz.
    for (QObject *child : widget->window()->children()) {
        if (auto *candidate = dynamic_cast<SelectionCoordinator *>(child)) {
            return candidate;
        }
    }
    return nullptr;
}

inline QString editInstruction(const SelectionCoordinator *coordinator)
{
    if (coordinator == nullptr) {
        return {};
    }
    if (coordinator->editPreloadError() != ScopeReferenceValidationError::None) {
        return QObject::tr("Kayıtlı scope current model üzerinde preload edilemedi. Eski CAD/FEM kimlikleri "
                           "seçili gösterilmedi; yeni surface kapsamını açıkça yeniden seçin.");
    }
    return QObject::tr("Viewport'tan surface seçin. Yalnız Apply Selection kalıcı document değişikliği üretir.");
}

} // namespace contact_details_detail

class ContactDetails final : public DetailsPage
{
public:
    explicit ContactDetails(const ServiceContext &services, QWidget *parent = nullptr)
        : DetailsPage(parent), services_(services)
    {
        auto *definition = addSection(tr("Definition"));
        name_ = new QLineEdit(this);
        name_->setObjectName(QStringLiteral("Dynamics26ContactName"));
        name_->setMinimumHeight(22);
        definition->addRow(tr("Name"), name_);
        formulation_ = makeValueLabel(QStringLiteral("Bonded"));
        formulation_->setObjectName(QStringLiteral("Dynamics26ContactFormulation"));
        definition->addRow(tr("Formulation"), formulation_);

        auto *source = addSection(tr("Source Surface"));
        sourceSummary_ = makeValueLabel();
        sourceSummary_->setObjectName(QStringLiteral("Dynamics26ContactSourceSummary"));
        sourceSummary_->setWordWrap(true);
        source->addRow(tr("Scope"), sourceSummary_);
        sourceLifecycle_ = makeValueLabel();
        sourceLifecycle_->setObjectName(QStringLiteral("Dynamics26ContactSourceLifecycle"));
        source->addRow(tr("Lifecycle"), sourceLifecycle_);
        sourceEditStatus_ = makeValueLabel();
        sourceEditStatus_->setObjectName(QStringLiteral("Dynamics26ContactSourceEditStatus"));
        sourceEditStatus_->setWordWrap(true);
        source->addFullWidth(sourceEditStatus_);
        editSource_ = makeActionButton(tr("Edit Source Selection"));
        editSource_->setObjectName(QStringLiteral("Dynamics26ContactEditSource"));
        source->addFullWidth(editSource_);
        applySource_ = makeActionButton(tr("Apply Selection"));
        applySource_->setObjectName(QStringLiteral("Dynamics26ContactApplySource"));
        source->addFullWidth(applySource_);
        cancelSource_ = makeActionButton(tr("Cancel"));
        cancelSource_->setObjectName(QStringLiteral("Dynamics26ContactCancelSource"));
        source->addFullWidth(cancelSource_);
        clearSource_ = makeActionButton(tr("Source'u Temizle"));
        clearSource_->setObjectName(QStringLiteral("Dynamics26ContactClearSource"));
        source->addFullWidth(clearSource_);

        auto *target = addSection(tr("Target Surface"));
        targetSummary_ = makeValueLabel();
        targetSummary_->setObjectName(QStringLiteral("Dynamics26ContactTargetSummary"));
        targetSummary_->setWordWrap(true);
        target->addRow(tr("Scope"), targetSummary_);
        targetLifecycle_ = makeValueLabel();
        targetLifecycle_->setObjectName(QStringLiteral("Dynamics26ContactTargetLifecycle"));
        target->addRow(tr("Lifecycle"), targetLifecycle_);
        targetEditStatus_ = makeValueLabel();
        targetEditStatus_->setObjectName(QStringLiteral("Dynamics26ContactTargetEditStatus"));
        targetEditStatus_->setWordWrap(true);
        target->addFullWidth(targetEditStatus_);
        editTarget_ = makeActionButton(tr("Edit Target Selection"));
        editTarget_->setObjectName(QStringLiteral("Dynamics26ContactEditTarget"));
        target->addFullWidth(editTarget_);
        applyTarget_ = makeActionButton(tr("Apply Selection"));
        applyTarget_->setObjectName(QStringLiteral("Dynamics26ContactApplyTarget"));
        target->addFullWidth(applyTarget_);
        cancelTarget_ = makeActionButton(tr("Cancel"));
        cancelTarget_->setObjectName(QStringLiteral("Dynamics26ContactCancelTarget"));
        target->addFullWidth(cancelTarget_);
        clearTarget_ = makeActionButton(tr("Target'ı Temizle"));
        clearTarget_->setObjectName(QStringLiteral("Dynamics26ContactClearTarget"));
        target->addFullWidth(clearTarget_);

        auto *status = addSection(tr("Engineering Status"));
        validation_ = makeValueLabel();
        validation_->setObjectName(QStringLiteral("Dynamics26ContactValidation"));
        validation_->setWordWrap(true);
        status->addRow(tr("Definition"), validation_);
        solverSupport_ = makeValueLabel(tr("Model solve: henüz etkin değil"));
        solverSupport_->setObjectName(QStringLiteral("Dynamics26ContactSolverSupport"));
        solverSupport_->setWordWrap(true);
        status->addRow(tr("Solver"), solverSupport_);
        status->addNote(tr("Geçerli bir Contact tanımı bile model-tabanlı Static Structural Contact consumer "
                           "etkinleşene kadar Preflight'ta Solve'u engeller. Doğrulama preset'i ile gerçek model "
                           "Contact consumer aynı özellik değildir."));

        auto *identity = addSection(tr("Engineering Identity"), true, true);
        objectIdValue_ = makeValueLabel();
        identity->addRow(tr("Object ID"), objectIdValue_);

        addStretch();

        connect(name_, &QLineEdit::editingFinished, this, [this] {
            if (refreshing_ || services_.contacts == nullptr || services_.commands == nullptr) {
                return;
            }
            if (SelectionCoordinator *coordinator = contact_details_detail::selectionCoordinator(this);
                coordinator != nullptr && coordinator->contactEditActive()) {
                refresh();
                return;
            }
            const ContactDefinition *definition = services_.contacts->byId(objectId_);
            const QString requested = name_->text().trimmed();
            if (definition == nullptr || requested.isEmpty() || requested == definition->name) {
                refresh();
                return;
            }
            services_.commands->push(new commands::RenameContactCommand(services_, objectId_, requested));
            emit modelEdited();
        });

        connect(editSource_, &QPushButton::clicked, this, [this] {
            SelectionCoordinator *coordinator = contact_details_detail::selectionCoordinator(this);
            if (coordinator != nullptr && coordinator->beginContactSourceEdit(objectId_)) {
                refresh();
            }
        });
        connect(applySource_, &QPushButton::clicked, this, [this] {
            SelectionCoordinator *coordinator = contact_details_detail::selectionCoordinator(this);
            if (coordinator != nullptr && coordinator->editingContact() == objectId_
                && coordinator->editingContactSource() && coordinator->applyContactEdit()) {
                emit modelEdited();
            } else {
                refresh();
            }
        });
        connect(cancelSource_, &QPushButton::clicked, this, [this] {
            if (SelectionCoordinator *coordinator = contact_details_detail::selectionCoordinator(this)) {
                coordinator->cancelContactEdit();
            }
        });

        connect(clearSource_, &QPushButton::clicked, this, [this] {
            if (services_.contacts == nullptr || services_.commands == nullptr) {
                return;
            }
            if (SelectionCoordinator *coordinator = contact_details_detail::selectionCoordinator(this);
                coordinator != nullptr && coordinator->contactEditActive()) {
                return;
            }
            const ContactDefinition *definition = services_.contacts->byId(objectId_);
            if (definition == nullptr || definition->sourceScope.entities.isEmpty()) {
                return;
            }
            services_.commands->push(
                new commands::ReplaceContactSourceScopeCommand(services_, objectId_, ScopeReference{}));
            emit modelEdited();
        });

        connect(editTarget_, &QPushButton::clicked, this, [this] {
            SelectionCoordinator *coordinator = contact_details_detail::selectionCoordinator(this);
            if (coordinator != nullptr && coordinator->beginContactTargetEdit(objectId_)) {
                refresh();
            }
        });
        connect(applyTarget_, &QPushButton::clicked, this, [this] {
            SelectionCoordinator *coordinator = contact_details_detail::selectionCoordinator(this);
            if (coordinator != nullptr && coordinator->editingContact() == objectId_
                && coordinator->editingContactTarget() && coordinator->applyContactEdit()) {
                emit modelEdited();
            } else {
                refresh();
            }
        });
        connect(cancelTarget_, &QPushButton::clicked, this, [this] {
            if (SelectionCoordinator *coordinator = contact_details_detail::selectionCoordinator(this)) {
                coordinator->cancelContactEdit();
            }
        });

        connect(clearTarget_, &QPushButton::clicked, this, [this] {
            if (services_.contacts == nullptr || services_.commands == nullptr) {
                return;
            }
            if (SelectionCoordinator *coordinator = contact_details_detail::selectionCoordinator(this);
                coordinator != nullptr && coordinator->contactEditActive()) {
                return;
            }
            const ContactDefinition *definition = services_.contacts->byId(objectId_);
            if (definition == nullptr || definition->targetScope.entities.isEmpty()) {
                return;
            }
            services_.commands->push(
                new commands::ReplaceContactTargetScopeCommand(services_, objectId_, ScopeReference{}));
            emit modelEdited();
        });
    }

    void refresh() override
    {
        refreshing_ = true;
        const ContactDefinition *definition =
            services_.contacts != nullptr ? services_.contacts->byId(objectId_) : nullptr;
        const ProjectObject *object =
            services_.project != nullptr ? services_.project->object(objectId_) : nullptr;
        SelectionCoordinator *coordinator = contact_details_detail::selectionCoordinator(this);
        const bool editingThis = coordinator != nullptr && coordinator->editingContact() == objectId_;
        const bool editingSource = editingThis && coordinator->editingContactSource();
        const bool editingTarget = editingThis && coordinator->editingContactTarget();

        const auto syncEditControls = [this, coordinator, editingThis, editingSource, editingTarget] {
            const bool coordinatorAvailable = coordinator != nullptr;
            editSource_->setVisible(!editingThis);
            editTarget_->setVisible(!editingThis);
            editSource_->setEnabled(coordinatorAvailable);
            editTarget_->setEnabled(coordinatorAvailable);
            applySource_->setVisible(editingSource);
            cancelSource_->setVisible(editingSource);
            applyTarget_->setVisible(editingTarget);
            cancelTarget_->setVisible(editingTarget);
            sourceEditStatus_->setVisible(editingSource);
            targetEditStatus_->setVisible(editingTarget);
            clearSource_->setVisible(!editingThis);
            clearTarget_->setVisible(!editingThis);
            name_->setEnabled(!editingThis);

            if (editingSource) {
                sourceEditStatus_->setText(contact_details_detail::editInstruction(coordinator));
                applySource_->setEnabled(coordinator->selectionManager() != nullptr
                                         && !coordinator->selectionManager()->items().isEmpty());
            }
            if (editingTarget) {
                targetEditStatus_->setText(contact_details_detail::editInstruction(coordinator));
                applyTarget_->setEnabled(coordinator->selectionManager() != nullptr
                                         && !coordinator->selectionManager()->items().isEmpty());
            }
        };

        if (definition == nullptr) {
            name_->setText(object != nullptr ? object->name : tr("Contact Region"));
            formulation_->setText(QStringLiteral("—"));
            sourceSummary_->setText(tr("Tanım bulunamadı"));
            targetSummary_->setText(tr("Tanım bulunamadı"));
            sourceLifecycle_->setText(QStringLiteral("—"));
            targetLifecycle_->setText(QStringLiteral("—"));
            validation_->setText(tr("Contact engineering tanımı bulunamadı"));
            clearSource_->setEnabled(false);
            clearTarget_->setEnabled(false);
            editSource_->setEnabled(false);
            editTarget_->setEnabled(false);
            objectIdValue_->setText(QString::number(objectId_));
            syncEditControls();
            editSource_->setEnabled(false);
            editTarget_->setEnabled(false);
            refreshing_ = false;
            return;
        }

        name_->setText(definition->name);
        formulation_->setText(QStringLiteral("Bonded"));
        const auto describe = [](const ScopeReference &scope) {
            if (scope.entities.isEmpty()) {
                return QObject::tr("Tanımlanmadı");
            }
            return QObject::tr("%1 / %2 · %3 entity")
                .arg(contact_details_detail::domainName(scope),
                     contact_details_detail::kindName(scope))
                .arg(scope.entities.size());
        };
        sourceSummary_->setText(describe(definition->sourceScope));
        targetSummary_->setText(describe(definition->targetScope));
        sourceLifecycle_->setText(contact_details_detail::lifecycleName(definition->sourceScope));
        targetLifecycle_->setText(contact_details_detail::lifecycleName(definition->targetScope));
        clearSource_->setEnabled(!definition->sourceScope.entities.isEmpty());
        clearTarget_->setEnabled(!definition->targetScope.entities.isEmpty());
        validation_->setText(object != nullptr && !object->statusText.isEmpty()
                                 ? object->statusText : tr("Contact tanımı doğrulanamadı"));
        objectIdValue_->setText(QString::number(objectId_));
        syncEditControls();
        refreshing_ = false;
    }

private:
    ServiceContext services_;
    bool refreshing_{false};
    QLineEdit *name_{nullptr};
    QLabel *formulation_{nullptr};
    QLabel *sourceSummary_{nullptr};
    QLabel *sourceLifecycle_{nullptr};
    QLabel *sourceEditStatus_{nullptr};
    QPushButton *editSource_{nullptr};
    QPushButton *applySource_{nullptr};
    QPushButton *cancelSource_{nullptr};
    QPushButton *clearSource_{nullptr};
    QLabel *targetSummary_{nullptr};
    QLabel *targetLifecycle_{nullptr};
    QLabel *targetEditStatus_{nullptr};
    QPushButton *editTarget_{nullptr};
    QPushButton *applyTarget_{nullptr};
    QPushButton *cancelTarget_{nullptr};
    QPushButton *clearTarget_{nullptr};
    QLabel *validation_{nullptr};
    QLabel *solverSupport_{nullptr};
    QLabel *objectIdValue_{nullptr};
};

} // namespace d26
