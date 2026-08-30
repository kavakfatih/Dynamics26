#pragma once

#include "DetailsPage.h"

#include "../core/ServiceContext.h"

namespace d26 {

// Project / Model / Body / Sections / Connections / Solution gibi salt-okunur
// nesnelerin Details sayfası. İçerik nesne türüne göre her tazelemede yeniden
// kurulur; düzenlenebilir alan içermediği için odak kaybı sorunu yoktur.
class ObjectDetails final : public DetailsPage
{
    Q_OBJECT
public:
    explicit ObjectDetails(const ServiceContext &services, QWidget *parent = nullptr);
    void refresh() override;

private:
    void buildProject();
    void buildModel();
    void buildBody();
    void buildMaterialsFolder();
    void buildSections();
    void buildConnections();
    void buildSolution();

    ServiceContext services_;
};

} // namespace d26
