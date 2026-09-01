#pragma once

// Dynamics26 V1.1.0-beta.1 / B1.2 — Connections authoring inspector.
//
// Connections klasörü ProjectModel tree container'dır; Contact engineering
// verisinin sahibi ContactService'tir. Bu sayfa servis durumunu okur ve yeni
// ContactRegion oluşturmayı yalnız QUndoStack domain command üzerinden yapar.

#include "DetailsPage.h"
#include "../commands/ContactCommands.h"
#include "../core/DocumentCommandManager.h"
#include "../core/ServiceContext.h"
#include "../services/ContactService.h"

#include <QPushButton>

namespace d26 {

class ConnectionsDetails final : public DetailsPage
{
public:
    explicit ConnectionsDetails(const ServiceContext &services, QWidget *parent = nullptr)
        : DetailsPage(parent), services_(services)
    {
    }

    void refresh() override
    {
        clearSections();

        auto *definition = addSection(tr("Definition"));
        if (services_.contacts == nullptr) {
            definition->addValueRow(tr("Status"), tr("ContactService kullanılamıyor"));
            addStretch();
            return;
        }

        definition->addValueRow(tr("Contact Regions"), QString::number(services_.contacts->count()));
        definition->addValueRow(tr("Joints"), QStringLiteral("0"));

        auto *authoring = addSection(tr("Authoring"));
        authoring->addNote(tr("Yeni Contact Region önce eksik document state olarak oluşturulur. "
                              "Source ve Target surface kapsamları Contact Inspector içinde tanımlanır."));
        auto *create = makeActionButton(tr("Yeni Contact Region"));
        create->setObjectName(QStringLiteral("Dynamics26ConnectionsAddContact"));
        create->setEnabled(services_.commands != nullptr);
        authoring->addFullWidth(create);
        connect(create, &QPushButton::clicked, this, [this] {
            if (services_.commands == nullptr || services_.contacts == nullptr) {
                return;
            }
            ContactDefinition definition;
            auto *command = new commands::CreateContactCommand(services_, definition);
            services_.commands->push(command);
            if (command->createdId() != InvalidObjectId) {
                emit modelEdited();
            }
        });

        auto *solver = addSection(tr("Solver Support"));
        solver->addValueRow(tr("Current Formulation"), QStringLiteral("Bonded"));
        solver->addValueRow(tr("Model Solve"), tr("Henüz etkin değil"));
        solver->addNote(tr("Contact tanımı engineering document state olarak saklanır ve Preflight tarafından "
                           "doğrulanır. Model-tabanlı Static Structural Contact consumer etkinleşene kadar "
                           "aktif Contact Region Solve'u güvenli biçimde engeller."));

        addStretch();
    }

private:
    ServiceContext services_;
};

} // namespace d26
