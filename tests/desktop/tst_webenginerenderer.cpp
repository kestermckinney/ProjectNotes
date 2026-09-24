// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "ProjectNotesEmail/ArtifactStore.h"
#include "ProjectNotesEmailRendering/WebEngineReportRenderer.h"

#include <QApplication>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <QtWebEngineQuick/QtWebEngineQuick>

using namespace PN::Comm;

class WebEngineRendererTest final : public QObject {
    Q_OBJECT
private slots:
    void rendersPdfIntoReservedOperationPath();
};

void WebEngineRendererTest::rendersPdfIntoReservedOperationPath()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    ArtifactStore store(directory.path());
    OperationManifest manifest;
    manifest.operationId = QUuid::createUuid();
    manifest.workflow = Workflow::StatusReport;
    manifest.databaseKey = QStringLiteral("synthetic-db");
    manifest.projectId = QStringLiteral("synthetic-project");
    QVERIFY(store.createOperation(manifest));
    ServiceError error;
    const QString output = store.reserveGeneratedPath(manifest.operationId, QStringLiteral("native-report.pdf"), &error);
    QVERIFY2(!output.isEmpty(), qPrintable(error.code));

    WebEngineReportRenderer renderer;
    bool completed = false;
    RenderResult result;
    renderer.render({manifest.operationId, 1, QStringLiteral("<html><body><h1>Native report</h1><p>synthetic</p></body></html>"), {}, output},
                    [&completed, &result](RenderResult value) { result = std::move(value); completed = true; });
    QTRY_VERIFY_WITH_TIMEOUT(completed, 45000);
    QVERIFY2(result.error.code.isEmpty(), qPrintable(result.error.code));
    QVERIFY(QFileInfo::exists(output));
    QVERIFY(QFileInfo(output).size() > 100);
    const auto artifact = store.registerGeneratedFile(manifest.operationId, output, QStringLiteral("native-report.pdf"), QStringLiteral("application/pdf"));
    QVERIFY2(artifact.ok(), qPrintable(artifact.error.code));
}

int main(int argc, char **argv)
{
    QtWebEngineQuick::initialize();
    QApplication application(argc, argv);
    WebEngineRendererTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_webenginerenderer.moc"
