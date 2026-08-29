#pragma once

#include <QJsonObject>
#include <QString>

namespace femcae::gui {

struct ProjectMigrationResult {
    bool ok = false;
    bool migrated = false;
    int sourceSchema = -1;
    int targetSchema = -1;
    QJsonObject project;
    QString message;
};

class ProjectFileMigrator final {
public:
    static ProjectMigrationResult migrate(const QJsonObject& input, int currentSchema);
};

} // namespace femcae::gui
