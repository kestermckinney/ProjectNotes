#include "TemplateParser.h"

namespace PN::Comm {

const QList<TemplateField> &templateFieldCatalog()
{
    static const QList<TemplateField> fields = {
        {"project.number"}, {"project.name"}, {"client.name"},
        {"preferences.managerName"}, {"preferences.managingCompanyName"},
        // Only Send Meeting Notes has a single meeting; reports span many.
        {"meeting.title", TemplateFieldType::Text, {Workflow::SendMeetingNotes}},
        {"meeting.date", TemplateFieldType::Date,
         {Workflow::SendMeetingNotes, Workflow::MeetingNotesReport}, true},
        {"report.date", TemplateFieldType::Date,
         {Workflow::TrackerItemsReport, Workflow::StatusReport, Workflow::MeetingNotesReport}},
        {"report.internal", TemplateFieldType::Boolean}, {"report.type"}
    };
    return fields;
}

QStringList templateFieldPaths(Workflow workflow)
{
    QStringList paths;
    for (const TemplateField &field : templateFieldCatalog())
        if (templateFieldIsApplicable(field.path, workflow))
            paths.append(field.path);
    return paths;
}

bool templateFieldIsApplicable(const QString &path, Workflow workflow)
{
    for (const TemplateField &field : templateFieldCatalog())
        if (field.path == path)
            return field.workflows.isEmpty() || field.workflows.contains(workflow);
    return false;
}

ValidationResult validateTemplateSubject(const QString &subject)
{
    ValidationResult result;
    if (subject.trimmed().isEmpty())
        result.addError(QStringLiteral("template-subject-required"), QStringLiteral("subject"));
    if (subject.contains(u'\r') || subject.contains(u'\n'))
        result.addError(QStringLiteral("template-subject-header-injection"), QStringLiteral("subject"));
    return result;
}

TemplateRender renderTemplateFields(const QString &input, const TemplateContext &context,
                                    TemplateRenderMode mode)
{
    TemplateRender result;
    for (int index = 0; index < input.size();) {
        if (input.mid(index, 3) == QStringLiteral("\\{{")) {
            result.text += QStringLiteral("{{");
            index += 3;
            continue;
        }
        if (input.mid(index, 2) != QStringLiteral("{{")) {
            result.text += input[index++];
            continue;
        }
        const int end = input.indexOf(QStringLiteral("}}"), index + 2);
        if (end < 0) {
            result.validation.addError("template-token-unclosed", "body");
            return result;
        }
        const QString key = input.mid(index + 2, end - index - 2).trimmed();
        bool valid = !key.isEmpty();
        for (const QString &part : key.split('.')) {
            if (part.isEmpty() || part == "__proto__" || part == "prototype"
                || part == "constructor" || (!part[0].isLetter() && part[0] != '_'))
                valid = false;
            for (int character = 1; character < part.size(); ++character)
                if (!part[character].isLetterOrNumber() && part[character] != '_')
                    valid = false;
        }
        const TemplateField *field = nullptr;
        for (const TemplateField &candidate : templateFieldCatalog())
            if (candidate.path == key) {
                field = &candidate;
                break;
            }
        if (!valid || !field || !templateFieldIsApplicable(key, context.workflow)
            || (!context.values.contains(key) && !field->optional)) {
            result.validation.addError("template-field-unresolved", key);
        } else if (context.values.contains(key)) {
            const QString value = context.values.value(key);
            result.text += mode == TemplateRenderMode::Html ? value.toHtmlEscaped() : value;
        }
        index = end + 2;
    }
    return result;
}

} // namespace PN::Comm
