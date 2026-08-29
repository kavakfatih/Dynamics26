#include "ProjectFileMigrator.h"

#include <QJsonObject>
#include <QString>
#include <iostream>

namespace {
int fail(const char* message)
{
    std::cerr << "FAIL: " << message << '\n';
    return 1;
}
}

int main()
{
    QJsonObject legacy;
    legacy["application_version"] = "0.8.0";
    legacy["material"] = QJsonObject{{"young_gpa", 210.0}, {"poisson", 0.3}};
    legacy["section"] = QJsonObject{{"area_mm2", 100.0}, {"length_mm", 1000.0}};
    legacy["load"] = QJsonObject{{"force_n", 1000.0}};

    auto r = femcae::gui::ProjectFileMigrator::migrate(legacy, 1);
    if (!r.ok || !r.migrated) return fail("schema-less legacy project did not migrate");
    if (r.project.value("project_schema").toInt(-1) != 1) return fail("target schema is not 1");
    if (!r.project.value("geometry").isObject()) return fail("geometry defaults missing");
    if (!r.project.value("prepost").isObject()) return fail("prepost defaults missing");
    if (!r.project.value("nonlinear").isObject()) return fail("nonlinear defaults missing");
    if (r.project.value("migrated_from_schema").toInt(-1) != 0) return fail("migration provenance missing");

    QJsonObject current = legacy;
    current["project_schema"] = 1;
    r = femcae::gui::ProjectFileMigrator::migrate(current, 1);
    if (!r.ok || r.migrated) return fail("current schema should pass without migration");

    QJsonObject future = current;
    future["project_schema"] = 2;
    r = femcae::gui::ProjectFileMigrator::migrate(future, 1);
    if (r.ok) return fail("future schema must be rejected");

    QJsonObject arbitrary{{"hello", "world"}};
    r = femcae::gui::ProjectFileMigrator::migrate(arbitrary, 1);
    if (r.ok) return fail("arbitrary schema-less JSON must be rejected");

    QJsonObject invalid = current;
    invalid["project_schema"] = "1";
    r = femcae::gui::ProjectFileMigrator::migrate(invalid, 1);
    if (r.ok) return fail("string project_schema must be rejected");

    std::cout << "project migration PASS\n";
    return 0;
}
