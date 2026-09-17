#pragma once
#include "CommunicationTypes.h"
#include <QStringList>
namespace PN::Comm {
enum class TemplateFieldType { Text, Date, Boolean };
enum class TemplateRenderMode { PlainText, Html };
struct TemplateField { QString path; TemplateFieldType type=TemplateFieldType::Text; QSet<Workflow> workflows; bool optional=false; };
struct TemplateContext {
    QHash<QString, QString> values;
    Workflow workflow = Workflow::SendMeetingNotes;
};
struct TemplateRender { QString text; ValidationResult validation; };
const QList<TemplateField> &templateFieldCatalog();
QStringList templateFieldPaths(Workflow workflow);
bool templateFieldIsApplicable(const QString &path, Workflow workflow);
}
