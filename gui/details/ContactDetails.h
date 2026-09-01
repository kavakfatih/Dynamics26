#pragma once

// Dynamics26 V1.1.0-beta.1 / B1.2 — ContactRegion engineering inspector.
//
// Bu sayfa ProjectModel tree state'ini ikinci kez tutmaz. Name / Source / Target /
// formulation verisi ContactService'ten okunur; kalıcı değişiklikler yalnız
// ContactCommands üzerinden QUndoStack'e gider. Source/Target için selection edit
// oturumu SelectionCoordinator'a bağlanmadan önce bile mevcut persistent scope ve
// lifecycle açıkça görünür; solver desteği olduğundan fazla gösterilmez.

#include "DetailsPage.h"
#include "../commands/ContactCommands.h"
#include "../core/DocumentCommandManager.h"
#include "../core/ProjectModel.h"
#include "../core/ServiceContext.h"
#include "../services/ContactService.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

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
            const ContactDefinition *definition = services_.contacts->byId(objectId_);
            const QString requested = name_->text().trimmed();
            if (definition == nullptr || requested.isEmpty() || requested == definition->name) {
                refresh();
                return;
            }
            services_.commands->push(new commands::RenameContactCommand(services_, objectId_, requested));
            emit modelEdited();
        });

        connect(clearSource_, &QPushButton::clicked, this, [this] {
            if (services_.contacts == nullptr || services_.commands == nullptr) {
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
        connect(clearTarget_, &QPushButton::clicked, this, [this] {
            if (services_.contacts == nullptr || services_.commands == nullptr) {
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
            objectIdValue_->setText(QString::number(objectId_));
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
        refreshing_ = false;
    }

private:
    ServiceContext services_;
    bool refreshing_{false};
    QLineEdit *name_{nullptr};
    QLabel *formulation_{nullptr};
    QLabel *sourceSummary_{nullptr};
    QLabel *sourceLifecycle_{nullptr};
    QPushButton *clearSource_{nullptr};
    QLabel *targetSummary_{nullptr};
    QLabel *targetLifecycle_{nullptr};
    QPushButton *clearTarget_{nullptr};
    QLabel *validation_{nullptr};
    QLabel *solverSupport_{nullptr};
    QLabel *objectIdValue_{nullptr};
};

} // namespace d26
