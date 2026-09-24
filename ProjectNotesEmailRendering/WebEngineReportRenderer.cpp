#include "WebEngineReportRenderer.h"
#include "RendererResourcePolicy.h"
#include <QDir>
#include <QFileInfo>
#include <QPageLayout>
#include <QPageSize>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineUrlRequestInfo>
#include <QWebEngineUrlRequestInterceptor>
#include <QTimer>

#include <memory>
namespace PN::Comm {
namespace {

class RestrictedRequestInterceptor final : public QWebEngineUrlRequestInterceptor {
public:
    void interceptRequest(QWebEngineUrlRequestInfo &info) override
    {
        if (!rendererResourceRequestAllowed(
                info.requestUrl(),
                info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeMainFrame))
            info.block(true);
    }
};

} // namespace

WebEngineReportRenderer::WebEngineReportRenderer(QObject *parent): QObject(parent) {}
void WebEngineReportRenderer::cancel(const QUuid &id) {
    m_cancelled.insert(id);
    if (auto *page = m_pages.value(id))
        page->triggerAction(QWebEnginePage::Stop);
}
void WebEngineReportRenderer::render(RenderRequest request, Completion completion) {
    RenderResult result{request.operationId, request.previewRevision};
    const QFileInfo output(request.outputPdfPath);
    if (request.operationId.isNull() || request.html.isEmpty() || !output.isAbsolute()
        || output.exists() || output.suffix().compare(QStringLiteral("pdf"), Qt::CaseInsensitive) != 0
        || !output.dir().exists()) {
        result.error={"renderer-input-invalid",{}}; completion(std::move(result)); return;
    }
    if (m_cancelled.remove(request.operationId)) { result.error={"operation-cancelled",{}}; completion(std::move(result)); return; }
    auto *profile = new QWebEngineProfile(this);
    profile->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
    profile->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, false);
    profile->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
    profile->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
    profile->setUrlRequestInterceptor(new RestrictedRequestInterceptor);
    auto *page=new QWebEnginePage(profile, this);
    m_pages.insert(request.operationId, page);
    auto *timeout = new QTimer(page);
    timeout->setSingleShot(true);
    auto completed = std::make_shared<bool>(false);
    auto finish = [this, page, timeout, request, completed, completion = std::move(completion)](ServiceError error) mutable {
        if (*completed)
            return;
        *completed = true;
        RenderResult result{request.operationId,request.previewRevision};
        timeout->stop(); m_pages.remove(request.operationId);
        if (m_cancelled.remove(request.operationId)) error = {"operation-cancelled", {}};
        if (error.code.isEmpty()) result.pdf={QUuid::createUuid(),request.outputPdfPath,QFileInfo(request.outputPdfPath).fileName(),"application/pdf",QFileInfo(request.outputPdfPath).size(),{},true};
        else result.error = std::move(error);
        completion(std::move(result)); page->deleteLater();
    };
    connect(timeout, &QTimer::timeout, page, [page, finish]() mutable { page->triggerAction(QWebEnginePage::Stop); finish({"renderer-timeout", {}}); });
    connect(page,&QWebEnginePage::renderProcessTerminated,page,[finish](QWebEnginePage::RenderProcessTerminationStatus, int) mutable { finish({"renderer-crashed", {}}); });
    connect(page,&QWebEnginePage::loadFinished,page,[page,request,finish](bool ok) mutable {
        if (!ok) { finish({"renderer-load-failed", {}}); return; }
        connect(page,&QWebEnginePage::pdfPrintingFinished,page,[request,finish](const QString &,bool ok) mutable {
            if (!ok || QFileInfo(request.outputPdfPath).size() <= 0) finish({"renderer-pdf-failed", {}});
            else finish({});
        });
        const QPageLayout layout = request.pageLayout.isValid()
            ? request.pageLayout
            : QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, {});
        page->printToPdf(request.outputPdfPath, layout);
    });
    timeout->start(30000);
    page->setHtml(request.html, QUrl(QStringLiteral("about:blank")));
}
}
