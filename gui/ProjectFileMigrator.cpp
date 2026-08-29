#include "ProjectFileMigrator.h"

#include <QJsonArray>
#include <QJsonValue>

namespace femcae::gui {
namespace {

constexpr int kLegacySchema = 0;
constexpr int kMaxObjectKeys = 256;

bool looksLikeLegacyProject(const QJsonObject& root)
{
    // V0.5-V0.13 project files always carried the main engineering groups.
    // A schema-less JSON is migrated only when it resembles a FEMCAE project;
    // arbitrary JSON must not silently become a project file.
    return root.contains("material") && root.value("material").isObject()
        && root.contains("section") && root.value("section").isObject()
        && root.contains("load") && root.value("load").isObject();
}

bool objectIsReasonablySized(const QJsonObject& object)
{
    if (object.size() > kMaxObjectKeys) {
        return false;
    }
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        if (it.value().isObject() && it.value().toObject().size() > kMaxObjectKeys) {
            return false;
        }
        if (it.value().isArray() && it.value().toArray().size() > 100000) {
            return false;
        }
    }
    return true;
}

void ensureObject(QJsonObject& root, const char* key)
{
    if (!root.value(QLatin1String(key)).isObject()) {
        root[QLatin1String(key)] = QJsonObject{};
    }
}

ProjectMigrationResult reject(int source, int target, const QString& message)
{
    ProjectMigrationResult result;
    result.sourceSchema = source;
    result.targetSchema = target;
    result.message = message;
    return result;
}

} // namespace

ProjectMigrationResult ProjectFileMigrator::migrate(const QJsonObject& input, int currentSchema)
{
    if (currentSchema < 1) {
        return reject(-1, currentSchema, QStringLiteral("Geçersiz uygulama project schema sürümü."));
    }
    if (!objectIsReasonablySized(input)) {
        return reject(-1, currentSchema, QStringLiteral("Proje JSON yapısı izin verilen sınırları aşıyor."));
    }

    const bool hasSchema = input.contains(QStringLiteral("project_schema"));
    int sourceSchema = -1;
    if (!hasSchema) {
        if (!looksLikeLegacyProject(input)) {
            return reject(-1, currentSchema,
                QStringLiteral("Project schema eksik ve dosya tanınan eski FEMCAE proje yapısına uymuyor."));
        }
        sourceSchema = kLegacySchema;
    } else {
        const QJsonValue schemaValue = input.value(QStringLiteral("project_schema"));
        if (!schemaValue.isDouble()) {
            return reject(-1, currentSchema, QStringLiteral("project_schema tam sayı olmalıdır."));
        }
        sourceSchema = schemaValue.toInt(-1);
        if (sourceSchema < 0) {
            return reject(sourceSchema, currentSchema, QStringLiteral("Negatif project schema desteklenmiyor."));
        }
    }

    if (sourceSchema > currentSchema) {
        return reject(sourceSchema, currentSchema,
            QStringLiteral("Proje daha yeni bir FEMCAE project schema sürümü kullanıyor."));
    }

    QJsonObject migrated = input;
    bool changed = false;

    if (sourceSchema == kLegacySchema) {
        // Schema 0 was never a persisted formal contract. It denotes recognized
        // schema-less V0.x JSON files only. Missing V1 groups are added with
        // empty objects so MainWindow can apply its engineering defaults.
        ensureObject(migrated, "nonlinear");
        ensureObject(migrated, "geometry");
        ensureObject(migrated, "prepost");
        if (!migrated.value(QStringLiteral("analysis")).isString()) {
            migrated[QStringLiteral("analysis")] = QStringLiteral("linear_static_axial_demo");
        }
        migrated[QStringLiteral("project_schema")] = 1;
        migrated[QStringLiteral("migrated_from_schema")] = 0;
        changed = true;
        sourceSchema = 0;
    }

    // Future migrations are deliberately sequential. When schema 2 appears,
    // add a 1 -> 2 transform here instead of skipping directly to the newest.
    const int resultingSchema = migrated.value(QStringLiteral("project_schema")).toInt(-1);
    if (resultingSchema != currentSchema) {
        return reject(sourceSchema, currentSchema,
            QStringLiteral("Bu kaynak sürüm için gerekli schema migration zinciri tanımlı değil."));
    }

    ProjectMigrationResult result;
    result.ok = true;
    result.migrated = changed;
    result.sourceSchema = sourceSchema;
    result.targetSchema = currentSchema;
    result.project = migrated;
    result.message = changed
        ? QStringLiteral("Eski FEMCAE proje dosyası project schema %1 sürümüne yükseltildi.").arg(currentSchema)
        : QStringLiteral("Project schema güncel.");
    return result;
}

} // namespace femcae::gui
