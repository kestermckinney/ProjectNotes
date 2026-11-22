// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// ProjectFolderStore against a local folder and, through a scripted
// Microsoft Graph, a Microsoft Teams channel folder.

#include "ProjectNotesIntegrations/MicrosoftOAuthManager.h"
#include "ProjectNotesIntegrations/Office365Service.h"
#include "ProjectNotesIntegrations/ProjectFolderStore.h"
#include "projectfolderlocation.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QUrlQuery>
#include <QtTest/QtTest>

using namespace PN::Comm;

namespace {

const QString kFolderUrl = QStringLiteral("https://contoso.sharepoint.com/sites/P4000/Shared%20Documents/General");

// Answers sign-in with a token, then hands Graph requests to `route`.
class ScriptedTransport final : public HttpTransport {
public:
    QList<HttpRequest> requests;
    std::function<HttpResponse(const HttpRequest &)> route;
    // What the sign-in grants.
    QString grantedScope = QStringLiteral("https://graph.microsoft.com/Files.ReadWrite.All");

    void send(HttpRequest request, Completion completion) override
    {
        if (request.url.host().contains(QLatin1String("login"))) {
            completion({request.operationId, 200, {}, QJsonDocument(QJsonObject{
                {"access_token", "graph-access"}, {"refresh_token", "graph-refresh"}, {"expires_in", 3600},
                {"scope", "offline_access " + grantedScope}}).toJson(QJsonDocument::Compact), {}});
            return;
        }
        requests.append(request);
        HttpResponse response = route ? route(request) : HttpResponse{};
        response.operationId = request.operationId;
        completion(std::move(response));
    }
    void cancel(const QUuid &) override {}

    // Graph requests whose decoded path contains `fragment`.
    QList<HttpRequest> matching(const QByteArray &method, const QString &fragment) const
    {
        QList<HttpRequest> out;
        for (const HttpRequest &request : requests)
            if (request.method == method && request.url.toString(QUrl::FullyDecoded).contains(fragment))
                out.append(request);
        return out;
    }
};

HttpResponse json(int status, const QJsonObject &body)
{
    return {{}, status, {}, QJsonDocument(body).toJson(QJsonDocument::Compact), {}};
}

QJsonObject item(const QString &id, const QString &name, bool folder, const QString &webUrl,
                 const QString &webDavUrl = {})
{
    QJsonObject object{{"id", id}, {"name", name}, {"webUrl", webUrl}};
    if (folder)
        object.insert("folder", QJsonObject{{"childCount", 0}});
    else
        object.insert("file", QJsonObject{});
    if (!webDavUrl.isEmpty())
        object.insert("webDavUrl", webDavUrl);
    return object;
}

// The folder the shares lookup resolves the channel folder URL to.
HttpResponse rootFolder()
{
    QJsonObject root = item("root-id", "General", true, kFolderUrl);
    root.insert("parentReference", QJsonObject{{"driveId", "b!drive"}});
    return json(200, root);
}

struct Signed {
    ScriptedTransport transport;
    MicrosoftOAuthManager manager;
    Office365Service service;
    explicit Signed(const QString &grantedScope = {})
    {
        if (!grantedScope.isEmpty())
            transport.grantedScope = grantedScope;
        service.setHttpTransport(&transport);
        service.setOAuthManager(&manager);
        manager.setSecretStore([](const QString &, QString *) { return QStringLiteral("saved-refresh"); }, {}, {});
        manager.configure("tenant", "client");
        service.restoreSession();
    }
};

template <typename T>
struct Result {
    bool done = false;
    T value{};
    ServiceError error;
};

} // namespace

class ProjectFolderStoreTest final : public QObject {
    Q_OBJECT
private slots:
    void eligibleLocations();
    void relativePathsStayInsideTheFolder();
    void shareIdIsBase64Url();
    void localFolderReadsAndWrites();
    void teamsWriteCreatesFoldersAndReturnsDocumentUrl();
    void teamsWriteWithoutOverwriteReportsExists();
    void teamsLargeFileUsesUploadSession();
    void teamsReadUsesDownloadUrlWithoutToken();
    void teamsListPagesAndMissingFolderIsEmpty();
    void teamsAccessDeniedAsksForWriteScope();
    void teamsReadOnlyWhenSavingIsOff();
    void teamsReadOnlyGrantFailsBeforeGraph();
};

void ProjectFolderStoreTest::eligibleLocations()
{
    QVERIFY(PN::ProjectFolder::isEligibleLocation("File Folder", "C:/Projects/P4000"));
    QVERIFY(PN::ProjectFolder::isEligibleLocation("Web Link", kFolderUrl));
    QVERIFY(!PN::ProjectFolder::isEligibleLocation("Web Link", "https://example.test/folder"));
    QVERIFY(!PN::ProjectFolder::isEligibleLocation("Web Link", "http://contoso.sharepoint.com/sites/x"));
    QVERIFY(!PN::ProjectFolder::isEligibleLocation("Word Document", kFolderUrl + "/a.docx"));
    QCOMPARE(ProjectFolder::fromLocation("Web Link", kFolderUrl).kind, ProjectFolder::Kind::Teams);
    QCOMPARE(ProjectFolder::fromLocation("File Folder", "/p").kind, ProjectFolder::Kind::Local);
    QVERIFY(!ProjectFolder::fromLocation("Web Link", "https://example.test").isValid());
}

void ProjectFolderStoreTest::relativePathsStayInsideTheFolder()
{
    QStringList segments;
    QVERIFY(ProjectFolderStore::splitRelative(QStringLiteral("Project Management\\Status Reports/./a.pdf"), &segments));
    QCOMPARE(segments, (QStringList{"Project Management", "Status Reports", "a.pdf"}));
    QVERIFY(ProjectFolderStore::splitRelative(QString(), &segments));
    QVERIFY(segments.isEmpty());
    QVERIFY(!ProjectFolderStore::splitRelative(QString(), &segments, false));
    QVERIFY(!ProjectFolderStore::splitRelative(QStringLiteral("../outside.txt"), &segments));
    QVERIFY(!ProjectFolderStore::splitRelative(QStringLiteral("a/../../b"), &segments));
    // Backslashes separate folders on every platform, so these leave the folder too.
    QVERIFY(!ProjectFolderStore::splitRelative(QStringLiteral("..\\outside.txt"), &segments));
    QVERIFY(!ProjectFolderStore::splitRelative(QStringLiteral("\\server\\share"), &segments));
    QVERIFY(!ProjectFolderStore::splitRelative(QStringLiteral("/etc/passwd"), &segments));
    QVERIFY(!ProjectFolderStore::splitRelative(QStringLiteral("C:/Windows"), &segments));
}

void ProjectFolderStoreTest::shareIdIsBase64Url()
{
    // Graph's documented example: u! + unpadded base64url of the URL.
    QCOMPARE(ProjectFolderStore::shareId(QStringLiteral("https://onedrive.live.com/redir?resid=1231244193912!12&authKey=1201919!12921!1")),
             QStringLiteral("u!aHR0cHM6Ly9vbmVkcml2ZS5saXZlLmNvbS9yZWRpcj9yZXNpZD0xMjMxMjQ0MTkzOTEyITEyJmF1dGhLZXk9MTIwMTkxOSExMjkyMSEx"));
}

void ProjectFolderStoreTest::localFolderReadsAndWrites()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    ProjectFolderStore store(nullptr);
    const ProjectFolder folder{ProjectFolder::Kind::Local, dir.path()};

    Result<QString> written;
    store.write(folder, QStringLiteral("Reports/2026/a.txt"), "hello", false, [&](QString location, ServiceError error) {
        written = {true, location, error};
    });
    QVERIFY(written.done);
    QVERIFY2(written.error.code.isEmpty(), qPrintable(written.error.displayText));
    QCOMPARE(written.value, dir.path() + QStringLiteral("/Reports/2026/a.txt"));

    Result<QString> again;
    store.write(folder, QStringLiteral("Reports/2026/a.txt"), "other", false, [&](QString location, ServiceError error) {
        again = {true, location, error};
    });
    QCOMPARE(again.error.code, QStringLiteral("exists"));

    Result<std::optional<QByteArray>> read;
    store.read(folder, QStringLiteral("Reports/2026/a.txt"), [&](std::optional<QByteArray> bytes, ServiceError error) {
        read = {true, bytes, error};
    });
    QVERIFY(read.value.has_value());
    QCOMPARE(*read.value, QByteArray("hello"));
    store.read(folder, QStringLiteral("Reports/missing.txt"), [&](std::optional<QByteArray> bytes, ServiceError error) {
        read = {true, bytes, error};
    });
    QVERIFY(!read.value.has_value());
    QVERIFY(read.error.code.isEmpty());

    Result<QList<ProjectFileEntry>> listed;
    store.list(folder, QStringLiteral("Reports"), [&](QList<ProjectFileEntry> entries, ServiceError error) {
        listed = {true, entries, error};
    });
    QCOMPARE(listed.value.size(), 1);
    QCOMPARE(listed.value.first().name, QStringLiteral("2026"));
    QVERIFY(listed.value.first().isDir);

    Result<bool> exists;
    store.exists(folder, QStringLiteral("../outside"), [&](bool found, ServiceError error) { exists = {true, found, error}; });
    QCOMPARE(exists.error.code, QStringLiteral("validation-failed"));
}

void ProjectFolderStoreTest::teamsWriteCreatesFoldersAndReturnsDocumentUrl()
{
    Signed s;
    QVERIFY(s.service.account().authenticated);
    const QString docUrl = QStringLiteral("https://contoso.sharepoint.com/sites/P4000/Shared%20Documents/General/Reports/Plan.docx");
    s.transport.route = [&](const HttpRequest &request) -> HttpResponse {
        const QString url = request.url.toString(QUrl::FullyDecoded);
        if (url.contains(QLatin1String("/shares/")))
            return rootFolder();
        if (request.method == "GET" && url.contains(QLatin1String("items/root-id:/Reports:")))
            return json(404, {{"error", QJsonObject{{"code", "itemNotFound"}, {"message", "missing"}}}});
        if (request.method == "POST" && url.contains(QLatin1String("items/root-id/children")))
            return json(201, item("reports-id", "Reports", true, kFolderUrl + "/Reports"));
        if (request.method == "PUT" && url.contains(QLatin1String("items/reports-id:/Plan.docx:/content")))
            return json(201, item("plan-id", "Plan.docx", false,
                                  "https://contoso.sharepoint.com/_layouts/15/Doc.aspx?sourcedoc=x", docUrl));
        return json(500, {});
    };
    ProjectFolderStore store(&s.service);
    Result<QString> written;
    store.write({ProjectFolder::Kind::Teams, kFolderUrl}, QStringLiteral("Reports/Plan.docx"), "docx-bytes", false,
                [&](QString location, ServiceError error) { written = {true, location, error}; });
    QVERIFY(written.done);
    QVERIFY2(written.error.code.isEmpty(), qPrintable(written.error.displayText));
    QCOMPARE(written.value, docUrl);   // Office files store the direct document URL

    const QList<HttpRequest> shares = s.transport.matching("GET", QStringLiteral("/shares/"));
    QCOMPARE(shares.size(), 1);
    QVERIFY(shares.first().url.toString().contains(ProjectFolderStore::shareId(kFolderUrl)));
    QVERIFY(shares.first().headers.value("Authorization").startsWith("Bearer "));
    const QList<HttpRequest> created = s.transport.matching("POST", QStringLiteral("children"));
    QCOMPARE(created.size(), 1);
    const QJsonObject createdBody = QJsonDocument::fromJson(created.first().body).object();
    QCOMPARE(createdBody.value("name").toString(), QStringLiteral("Reports"));
    QCOMPARE(createdBody.value("@microsoft.graph.conflictBehavior").toString(), QStringLiteral("fail"));
    const QList<HttpRequest> put = s.transport.matching("PUT", QStringLiteral("Plan.docx:/content"));
    QCOMPARE(put.size(), 1);
    QCOMPARE(put.first().body, QByteArray("docx-bytes"));
    QCOMPARE(QUrlQuery(put.first().url).queryItemValue("@microsoft.graph.conflictBehavior"), QStringLiteral("fail"));

    // A second write reuses the resolved folder.
    store.write({ProjectFolder::Kind::Teams, kFolderUrl}, QStringLiteral("Reports/Plan.docx"), "docx-bytes", true,
                [&](QString location, ServiceError error) { written = {true, location, error}; });
    QCOMPARE(s.transport.matching("GET", QStringLiteral("/shares/")).size(), 1);
}

void ProjectFolderStoreTest::teamsWriteWithoutOverwriteReportsExists()
{
    Signed s;
    s.transport.route = [&](const HttpRequest &request) -> HttpResponse {
        const QString url = request.url.toString(QUrl::FullyDecoded);
        if (url.contains(QLatin1String("/shares/")))
            return rootFolder();
        if (request.method == "PUT")
            return json(409, {{"error", QJsonObject{{"code", "nameAlreadyExists"}, {"message", "exists"}}}});
        return json(500, {});
    };
    ProjectFolderStore store(&s.service);
    Result<QString> written;
    store.write({ProjectFolder::Kind::Teams, kFolderUrl}, QStringLiteral("notes.txt"), "x", false,
                [&](QString location, ServiceError error) { written = {true, location, error}; });
    QVERIFY(written.done);
    QCOMPARE(written.error.code, QStringLiteral("exists"));
}

void ProjectFolderStoreTest::teamsLargeFileUsesUploadSession()
{
    Signed s;
    const QByteArray bytes(ProjectFolderStore::kSimpleUploadLimit + ProjectFolderStore::kUploadChunk / 2, 'z');
    const QString uploadUrl = QStringLiteral("https://contoso.sharepoint.com/_api/upload?session=1");
    s.transport.route = [&](const HttpRequest &request) -> HttpResponse {
        const QString url = request.url.toString(QUrl::FullyDecoded);
        if (url.contains(QLatin1String("/shares/")))
            return rootFolder();
        if (request.method == "POST" && url.contains(QLatin1String("big.pdf:/createUploadSession")))
            return json(200, {{"uploadUrl", uploadUrl}});
        if (request.method == "PUT" && request.url == QUrl(uploadUrl)) {
            const QByteArray range = request.headers.value("Content-Range");
            if (range.endsWith(QByteArray::number(bytes.size() - 1) + '/' + QByteArray::number(bytes.size())))
                return json(201, item("big-id", "big.pdf", false, kFolderUrl + "/big.pdf"));
            return json(202, {{"nextExpectedRanges", QJsonArray{}}});
        }
        return json(500, {});
    };
    ProjectFolderStore store(&s.service);
    Result<QString> written;
    store.write({ProjectFolder::Kind::Teams, kFolderUrl}, QStringLiteral("big.pdf"), bytes, true,
                [&](QString location, ServiceError error) { written = {true, location, error}; });
    QVERIFY(written.done);
    QVERIFY2(written.error.code.isEmpty(), qPrintable(written.error.displayText));
    QCOMPARE(written.value, kFolderUrl + QStringLiteral("/big.pdf"));

    QList<HttpRequest> chunks;
    for (const HttpRequest &request : s.transport.requests)
        if (request.url == QUrl(uploadUrl))
            chunks.append(request);
    QCOMPARE(chunks.size(), 2);
    qsizetype total = 0;
    for (const HttpRequest &chunk : chunks) {
        QVERIFY(!chunk.headers.contains("Authorization"));   // the session URL is pre-authorized
        QVERIFY(chunk.body.size() % (320 * 1024) == 0 || &chunk == &chunks.last());
        total += chunk.body.size();
    }
    QCOMPARE(total, bytes.size());
    QCOMPARE(chunks.first().headers.value("Content-Range"),
             "bytes 0-" + QByteArray::number(ProjectFolderStore::kUploadChunk - 1) + '/' + QByteArray::number(bytes.size()));
    const QJsonObject session = QJsonDocument::fromJson(
        s.transport.matching("POST", QStringLiteral("createUploadSession")).first().body).object();
    QCOMPARE(session.value("item").toObject().value("@microsoft.graph.conflictBehavior").toString(), QStringLiteral("replace"));
}

void ProjectFolderStoreTest::teamsReadUsesDownloadUrlWithoutToken()
{
    Signed s;
    const QString download = QStringLiteral("https://contoso.sharepoint.com/download.aspx?token=abc");
    s.transport.route = [&](const HttpRequest &request) -> HttpResponse {
        const QString url = request.url.toString(QUrl::FullyDecoded);
        if (url.contains(QLatin1String("/shares/")))
            return rootFolder();
        if (url.contains(QLatin1String("items/root-id:/Mail/download_state.json:"))) {
            QJsonObject object = item("state-id", "download_state.json", false, kFolderUrl + "/Mail/download_state.json");
            object.insert("@microsoft.graph.downloadUrl", download);
            return json(200, object);
        }
        if (request.url == QUrl(download))
            return {{}, 200, {}, QByteArray("{\"skip\":3}"), {}};
        if (url.contains(QLatin1String("missing.json")))
            return json(404, {});
        return json(500, {});
    };
    ProjectFolderStore store(&s.service);
    Result<std::optional<QByteArray>> read;
    store.read({ProjectFolder::Kind::Teams, kFolderUrl}, QStringLiteral("Mail/download_state.json"),
               [&](std::optional<QByteArray> bytes, ServiceError error) { read = {true, bytes, error}; });
    QVERIFY(read.done);
    QVERIFY2(read.error.code.isEmpty(), qPrintable(read.error.displayText));
    QCOMPARE(read.value.value_or(QByteArray()), QByteArray("{\"skip\":3}"));
    bool downloadHadToken = true;
    for (const HttpRequest &request : s.transport.requests)
        if (request.url == QUrl(download))
            downloadHadToken = request.headers.contains("Authorization");
    QVERIFY(!downloadHadToken);

    store.read({ProjectFolder::Kind::Teams, kFolderUrl}, QStringLiteral("missing.json"),
               [&](std::optional<QByteArray> bytes, ServiceError error) { read = {true, bytes, error}; });
    QVERIFY(!read.value.has_value());
    QVERIFY(read.error.code.isEmpty());
}

void ProjectFolderStoreTest::teamsListPagesAndMissingFolderIsEmpty()
{
    Signed s;
    const QString nextPage = QStringLiteral("https://graph.microsoft.com/v1.0/drives/b!drive/items/inbox-id/children?$skiptoken=2");
    s.transport.route = [&](const HttpRequest &request) -> HttpResponse {
        const QString url = request.url.toString(QUrl::FullyDecoded);
        if (url.contains(QLatin1String("/shares/")))
            return rootFolder();
        if (url.contains(QLatin1String("skiptoken")))
            return json(200, {{"value", QJsonArray{item("b", "b.eml", false, kFolderUrl + "/Mail/b.eml")}}});
        if (url.contains(QLatin1String("items/root-id:/Mail:/children")))
            return json(200, {{"value", QJsonArray{item("a", "A.eml", false, kFolderUrl + "/Mail/A.eml"),
                                                   item("s", "Sent", true, kFolderUrl + "/Mail/Sent")}},
                              {"@odata.nextLink", nextPage}});
        if (url.contains(QLatin1String("Nothing")))
            return json(404, {});
        return json(500, {});
    };
    ProjectFolderStore store(&s.service);
    Result<QList<ProjectFileEntry>> listed;
    store.list({ProjectFolder::Kind::Teams, kFolderUrl}, QStringLiteral("Mail"),
               [&](QList<ProjectFileEntry> entries, ServiceError error) { listed = {true, entries, error}; });
    QVERIFY(listed.done);
    QVERIFY2(listed.error.code.isEmpty(), qPrintable(listed.error.displayText));
    QCOMPARE(listed.value.size(), 3);
    QCOMPARE(listed.value.at(0).name, QStringLiteral("A.eml"));
    QCOMPARE(listed.value.at(1).name, QStringLiteral("b.eml"));
    QVERIFY(listed.value.at(2).isDir);

    store.list({ProjectFolder::Kind::Teams, kFolderUrl}, QStringLiteral("Nothing"),
               [&](QList<ProjectFileEntry> entries, ServiceError error) { listed = {true, entries, error}; });
    QVERIFY(listed.value.isEmpty());
    QVERIFY(listed.error.code.isEmpty());
}

void ProjectFolderStoreTest::teamsAccessDeniedAsksForWriteScope()
{
    Signed s;
    s.transport.route = [&](const HttpRequest &request) -> HttpResponse {
        if (request.url.toString(QUrl::FullyDecoded).contains(QLatin1String("/shares/")))
            return rootFolder();
        return json(403, {{"error", QJsonObject{{"code", "accessDenied"}, {"message", "Access denied"}}}});
    };
    ProjectFolderStore store(&s.service);
    Result<QString> written;
    store.write({ProjectFolder::Kind::Teams, kFolderUrl}, QStringLiteral("a.txt"), "x", true,
                [&](QString location, ServiceError error) { written = {true, location, error}; });
    QVERIFY(written.done);
    QCOMPARE(written.error.code, QStringLiteral("teams-access-denied"));
    QVERIFY(written.error.displayText.contains(QStringLiteral("Files.ReadWrite.All")));
    QVERIFY(s.manager.requestedScopeList().contains(QStringLiteral("https://graph.microsoft.com/Files.ReadWrite.All")));
}

void ProjectFolderStoreTest::teamsReadOnlyWhenSavingIsOff()
{
    // Saving into Teams turned off: writes fail at once without a Graph
    // call and never queue the write scope, but reading still works.
    Signed s;
    s.service.setTeamsFileWriteEnabled(false);
    s.transport.route = [&](const HttpRequest &request) -> HttpResponse {
        const QString url = request.url.toString(QUrl::FullyDecoded);
        if (url.contains(QLatin1String("/shares/")))
            return rootFolder();
        if (url.contains(QLatin1String("/children")))
            return json(200, {{"value", QJsonArray{item("f1", "a.txt", false, kFolderUrl + "/a.txt")}}});
        return json(404, {});
    };
    ProjectFolderStore store(&s.service);
    const ProjectFolder teams{ProjectFolder::Kind::Teams, kFolderUrl};
    s.transport.requests.clear();   // the sign-in's account lookup
    const QStringList requestedBefore = s.manager.requestedScopeList();
    QVERIFY(!store.canWrite(teams));
    QVERIFY(store.canWrite({ProjectFolder::Kind::Local, QStringLiteral("C:/Projects/P4000")}));

    Result<QString> written;
    store.write(teams, QStringLiteral("a.txt"), "x", true,
                [&](QString location, ServiceError error) { written = {true, location, error}; });
    QVERIFY(written.done);
    QCOMPARE(written.error.code, QStringLiteral("teams-read-only"));
    Result<QString> made;
    store.mkdir(teams, QStringLiteral("Reports"),
                [&](QString location, ServiceError error) { made = {true, location, error}; });
    QVERIFY(made.done);
    QCOMPARE(made.error.code, QStringLiteral("teams-read-only"));
    QVERIFY(s.transport.requests.isEmpty());
    QCOMPARE(s.manager.requestedScopeList(), requestedBefore);

    Result<QList<ProjectFileEntry>> listed;
    store.list(teams, QString(),
               [&](QList<ProjectFileEntry> entries, ServiceError error) { listed = {true, entries, error}; });
    QVERIFY(listed.done);
    QVERIFY2(listed.error.code.isEmpty(), qPrintable(listed.error.displayText));
    QCOMPARE(listed.value.size(), 1);
}

void ProjectFolderStoreTest::teamsReadOnlyGrantFailsBeforeGraph()
{
    // Saving is on, but Microsoft granted read access only: the write fails
    // without a Graph call and the next sign-in asks for the write scope.
    Signed s(QStringLiteral("https://graph.microsoft.com/Files.Read.All"));
    ProjectFolderStore store(&s.service);
    const ProjectFolder teams{ProjectFolder::Kind::Teams, kFolderUrl};
    QVERIFY(s.service.account().authenticated);
    s.transport.requests.clear();   // the sign-in's account lookup
    QVERIFY(!store.canWrite(teams));
    Result<QString> written;
    store.write(teams, QStringLiteral("a.txt"), "x", true,
                [&](QString location, ServiceError error) { written = {true, location, error}; });
    QVERIFY(written.done);
    QCOMPARE(written.error.code, QStringLiteral("teams-access-denied"));
    QVERIFY(written.error.displayText.contains(QStringLiteral("Files.ReadWrite.All")));
    QVERIFY(s.transport.requests.isEmpty());
    QVERIFY(s.manager.requestedScopeList().contains(QStringLiteral("https://graph.microsoft.com/Files.ReadWrite.All")));
}

QTEST_GUILESS_MAIN(ProjectFolderStoreTest)
#include "tst_projectfolderstore.moc"
