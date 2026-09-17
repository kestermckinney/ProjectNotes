// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "ArtifactStore.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QSaveFile>

namespace PN::Comm {
namespace {
QString stateName(OperationState state) {
    switch (state) { case OperationState::Staged: return "staged"; case OperationState::Cancelled: return "cancelled";
    case OperationState::DraftCreated: return "draft-created"; case OperationState::HandoffRequested: return "handoff-requested";
    case OperationState::Unknown: return "unknown"; } return "unknown";
}
OperationState stateFromName(const QString &value) {
    if (value == "staged") return OperationState::Staged; if (value == "cancelled") return OperationState::Cancelled;
    if (value == "draft-created") return OperationState::DraftCreated; if (value == "handoff-requested") return OperationState::HandoffRequested;
    return OperationState::Unknown;
}
void fail(ServiceError *error, const QString &code, const QString &text = {}) { if (error) *error = {code, text}; }
QJsonObject artifactJson(const Artifact &a) { return {{"id", a.artifactId.toString(QUuid::WithoutBraces)}, {"path", a.absolutePath}, {"name", a.displayName}, {"mime", a.mimeType}, {"size", QString::number(a.byteSize)}, {"sha256", QString::fromLatin1(a.sha256.toHex())}, {"generated", a.generatedByApp}}; }
Artifact artifactFromJson(const QJsonObject &o) { Artifact a; a.artifactId = QUuid(o["id"].toString()); a.absolutePath=o["path"].toString(); a.displayName=o["name"].toString(); a.mimeType=o["mime"].toString(); a.byteSize=o["size"].toString().toLongLong(); a.sha256=QByteArray::fromHex(o["sha256"].toString().toLatin1()); a.generatedByApp=o["generated"].toBool(); return a; }
}

ArtifactStore::ArtifactStore(QString profileDirectory) : m_profileDirectory(std::move(profileDirectory)) {}
QString ArtifactStore::cacheDirectory(const QString &developerProfile)
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (!developerProfile.isEmpty())
        path = QDir(path).filePath(developerProfile);
    return path;
}
QString ArtifactStore::stagingRoot() const { return QDir(m_profileDirectory).filePath(QStringLiteral("communications/staging")); }
QString ArtifactStore::operationDirectory(const QUuid &id) const { return QDir(stagingRoot()).filePath(id.toString(QUuid::WithoutBraces)); }

bool ArtifactStore::validFileName(const QString &name) { return !name.isEmpty() && name != "." && name != ".." && !name.contains('/') && !name.contains('\\') && !name.contains(QStringLiteral("..")); }

bool ArtifactStore::createOperation(OperationManifest manifest, ServiceError *error)
{
    if (manifest.operationId.isNull()) { fail(error, "operation-id-required"); return false; }
    const QString dir = operationDirectory(manifest.operationId);
    // A reused operation ID must never replace a recoverable manifest: doing so
    // would orphan any staged artifacts from the earlier operation.
    if (QFileInfo::exists(dir)) { fail(error, "operation-already-exists"); return false; }
    if (!QDir().mkpath(dir)) { fail(error, "staging-create-failed"); return false; }
    QFile::setPermissions(dir, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    manifest.createdAt = manifest.createdAt.isValid() ? manifest.createdAt : QDateTime::currentDateTimeUtc();
    return writeManifest(manifest, error);
}

bool ArtifactStore::writeManifest(const OperationManifest &m, ServiceError *error) const
{
    QJsonArray artifacts; for (const auto &a : m.artifacts) artifacts.append(artifactJson(a));
    const QJsonObject json{{"schemaVersion", 1}, {"operationId", m.operationId.toString(QUuid::WithoutBraces)}, {"workflow", toStableString(m.workflow)}, {"databaseKey", m.databaseKey}, {"projectId", m.projectId}, {"backend", toStableString(m.backend)}, {"state", stateName(m.state)}, {"providerIdentity", m.providerIdentity}, {"accountKey", m.accountKey}, {"createdAt", m.createdAt.toUTC().toString(Qt::ISODateWithMs)}, {"artifacts", artifacts}};
    QSaveFile file(QDir(operationDirectory(m.operationId)).filePath(QStringLiteral("manifest.json")));
    if (!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(json).toJson(QJsonDocument::Compact)) < 0 || !file.commit()) { fail(error, "manifest-write-failed"); return false; }
    QFile::setPermissions(file.fileName(), QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    return true;
}

bool ArtifactStore::readManifest(const QUuid &id, OperationManifest *m, ServiceError *error) const
{
    QFile file(QDir(operationDirectory(id)).filePath(QStringLiteral("manifest.json")));
    if (!file.open(QIODevice::ReadOnly)) { fail(error, "manifest-not-found"); return false; }
    QJsonParseError parse; const auto doc = QJsonDocument::fromJson(file.readAll(), &parse);
    const auto o = doc.object();
    if (parse.error != QJsonParseError::NoError || !doc.isObject() || o["schemaVersion"].toInt() != 1 || QUuid(o["operationId"].toString()) != id) { fail(error, "manifest-corrupt"); return false; }
    auto workflow = workflowFromStableString(o["workflow"].toString()); auto backend = backendIdFromStableString(o["backend"].toString());
    if (!workflow || !backend) { fail(error, "manifest-corrupt"); return false; }
    m->operationId=id; m->workflow=*workflow; m->databaseKey=o["databaseKey"].toString(); m->projectId=o["projectId"].toString(); m->backend=*backend; m->state=stateFromName(o["state"].toString()); m->providerIdentity=o["providerIdentity"].toString(); m->accountKey=o["accountKey"].toString(); m->createdAt=QDateTime::fromString(o["createdAt"].toString(), Qt::ISODateWithMs); for (const auto &v:o["artifacts"].toArray()) m->artifacts.append(artifactFromJson(v.toObject())); return true;
}

QByteArray ArtifactStore::hashFile(const QString &path, ServiceError *error) { QFile file(path); if (!file.open(QIODevice::ReadOnly)) { fail(error, "artifact-unreadable"); return {}; } QCryptographicHash hash(QCryptographicHash::Sha256); while (!file.atEnd()) hash.addData(file.read(65536)); return hash.result(); }
bool ArtifactStore::appendArtifact(const QUuid &id, const Artifact &artifact, ServiceError *error) { OperationManifest m; if (!readManifest(id, &m, error)) return false; m.artifacts.append(artifact); return writeManifest(m, error); }

ArtifactStoreResult ArtifactStore::stageFile(const QUuid &id, const QString &name, const QByteArray *content, const QString &source, const QString &mime, bool generated)
{
    ArtifactStoreResult result; OperationManifest m; if (!readManifest(id, &m, &result.error)) return result;
    if (!validFileName(name)) { result.error.code="artifact-name-invalid"; return result; }
    const QString attachmentDir=QDir(operationDirectory(id)).filePath(QStringLiteral("artifacts")); if (!QDir().mkpath(attachmentDir)) { result.error.code="artifact-directory-create-failed"; return result; }
    QString target=QDir(attachmentDir).filePath(name); int suffix=2; const QFileInfo base(name); while (QFileInfo::exists(target)) target=QDir(attachmentDir).filePath(base.completeBaseName()+QStringLiteral(" (%1)").arg(suffix++)+(base.suffix().isEmpty()?QString():QStringLiteral(".")+base.suffix()));
    QSaveFile output(target); if (!output.open(QIODevice::WriteOnly)) { result.error.code="artifact-write-failed"; return result; }
    if (content) output.write(*content); else { QFile input(source); if (!input.open(QIODevice::ReadOnly)) { result.error.code="artifact-unreadable"; return result; } while (!input.atEnd()) output.write(input.read(65536)); }
    if (!output.commit()) { result.error.code="artifact-write-failed"; return result; }
    QFile::setPermissions(target, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    result.artifact={QUuid::createUuid(), QFileInfo(target).absoluteFilePath(), QFileInfo(target).fileName(), mime, QFileInfo(target).size(), hashFile(target, &result.error), generated}; if (!result.ok()) return result;
    if (!appendArtifact(id, result.artifact, &result.error)) return result; return result;
}

ArtifactStoreResult ArtifactStore::stageUserAttachment(const QUuid &id, const QString &source, const QString &displayName) { QFileInfo info(source); if (!info.exists() || !info.isFile() || info.isSymLink()) { ArtifactStoreResult r; r.error.code="attachment-source-invalid"; return r; } return stageFile(id, displayName.isEmpty()?info.fileName():displayName, nullptr, info.absoluteFilePath(), {}, false); }
ArtifactStoreResult ArtifactStore::stageGeneratedContent(const QUuid &id, const QString &name, const QByteArray &content, const QString &mime) { return stageFile(id, name, &content, {}, mime, true); }

QString ArtifactStore::reserveGeneratedPath(const QUuid &id, const QString &name, ServiceError *error)
{
    OperationManifest manifest;
    if (!readManifest(id, &manifest, error)) return {};
    if (!validFileName(name)) { fail(error, "artifact-name-invalid"); return {}; }
    const QString directory = QDir(operationDirectory(id)).filePath(QStringLiteral("artifacts"));
    if (!QDir().mkpath(directory)) { fail(error, "artifact-directory-create-failed"); return {}; }
    QFile::setPermissions(directory, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    const QFileInfo base(name); QString target = QDir(directory).filePath(name); int suffix = 2;
    while (QFileInfo::exists(target))
        target = QDir(directory).filePath(base.completeBaseName() + QStringLiteral(" (%1)").arg(suffix++)
                                            + (base.suffix().isEmpty() ? QString() : QStringLiteral(".") + base.suffix()));
    return QFileInfo(target).absoluteFilePath();
}

ArtifactStoreResult ArtifactStore::registerGeneratedFile(const QUuid &id, const QString &path,
                                                         const QString &displayName, const QString &mime)
{
    ArtifactStoreResult result; OperationManifest manifest;
    if (!readManifest(id, &manifest, &result.error)) return result;
    if (!validFileName(displayName)) { result.error.code = "artifact-name-invalid"; return result; }
    const QFileInfo file(path);
    const QString directory = QDir::cleanPath(QFileInfo(QDir(operationDirectory(id)).filePath(QStringLiteral("artifacts"))).absoluteFilePath()) + QDir::separator();
    const QString absolutePath = QDir::cleanPath(file.absoluteFilePath());
    if (!file.exists() || !file.isFile() || file.isSymLink() || !absolutePath.startsWith(directory)) { result.error.code = "artifact-path-rejected"; return result; }
    for (const Artifact &artifact : manifest.artifacts)
        if (artifact.absolutePath == absolutePath) {
            result.error.code = "artifact-already-registered";
            return result;
        }
    result.artifact = {QUuid::createUuid(), absolutePath, displayName, mime, file.size(), hashFile(absolutePath, &result.error), true};
    if (!result.ok()) return result;
    if (!appendArtifact(id, result.artifact, &result.error)) return result;
    return result;
}

bool ArtifactStore::publish(const Artifact &artifact, const QString &destination, ServiceError *error) {
    const QFileInfo source(artifact.absolutePath);
    if (!artifact.generatedByApp || !source.exists() || !source.isFile() || source.isSymLink()
        || artifact.sha256.isEmpty() || source.size() != artifact.byteSize
        || hashFile(source.absoluteFilePath(), nullptr) != artifact.sha256) {
        fail(error, "publish-source-invalid");
        return false;
    }
    if (QFileInfo::exists(destination)) { fail(error,"destination-exists"); return false; }
    QSaveFile out(destination); QFile in(artifact.absolutePath);
    if (!in.open(QIODevice::ReadOnly) || !out.open(QIODevice::WriteOnly)) { fail(error,"destination-unwritable"); return false; }
    while (!in.atEnd()) out.write(in.read(65536));
    if (!out.commit()) { fail(error,"destination-unwritable"); return false; }
    return true;
}

QList<OperationManifest> ArtifactStore::loadRecoveryStatus(QList<ServiceError> *diagnostics) { QList<OperationManifest> result; QDir root(stagingRoot()); for (const auto &entry:root.entryList(QDir::Dirs|QDir::NoDotAndDotDot)) { const QUuid id(entry); if (id.isNull()) continue; OperationManifest m; ServiceError error; if (readManifest(id,&m,&error)) result.append(m); else { QFile broken(root.filePath(entry+QStringLiteral("/manifest.json"))); if (broken.exists()) broken.rename(root.filePath(entry+QStringLiteral("/manifest.corrupt.json"))); if (diagnostics) diagnostics->append(error); } } return result; }
bool ArtifactStore::cleanup(const QUuid &id, ServiceError *error) { OperationManifest m; if (!readManifest(id,&m,error)) return false; const QString dir=QFileInfo(operationDirectory(id)).absoluteFilePath()+QDir::separator(); for (const auto &a:m.artifacts) if (!QFileInfo(a.absolutePath).absoluteFilePath().startsWith(dir) || QFileInfo(a.absolutePath).isSymLink()) { fail(error,"cleanup-path-rejected"); return false; } if (!QDir(operationDirectory(id)).removeRecursively()) { fail(error,"cleanup-failed"); return false; } return true; }

bool ArtifactStore::cleanupAll(ServiceError *error)
{
    const QFileInfo root(stagingRoot());
    if (!root.exists()) return true;
    if (!root.isDir() || root.isSymLink()) { fail(error, "cleanup-root-rejected"); return false; }
    if (!QDir(root.absoluteFilePath()).removeRecursively()) { fail(error, "cleanup-failed"); return false; }
    return true;
}

} // namespace PN::Comm
