#pragma once
#include "ProjectNotesEmail/ReportRenderer.h"
#include <QObject>
#include <QHash>
#include <QSet>
class QWebEnginePage;
namespace PN::Comm {
class WebEngineReportRenderer final : public QObject, public ReportRenderer {
    Q_OBJECT
public:
    explicit WebEngineReportRenderer(QObject *parent=nullptr);
    void render(RenderRequest request, Completion completion) override;
    void cancel(const QUuid &operationId) override;
private:
    QSet<QUuid> m_cancelled;
    QHash<QUuid, QWebEnginePage *> m_pages;
};
}
