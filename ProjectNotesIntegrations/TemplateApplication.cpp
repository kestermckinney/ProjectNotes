// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "TemplateApplication.h"

#include "TemplateParser.h"

#include <QRegularExpression>

namespace PN::Comm {
namespace {

const QRegularExpression &contentTokenExpression()
{
    static const QRegularExpression expression(
        QStringLiteral(R"(\{\{\s*content\.body\s*\}\})"));
    return expression;
}

TemplateRender renderWithProtectedContent(QString body, const TemplateContext &context,
                                          TemplateRenderMode mode, const QString &nativeContent,
                                          ValidationResult *validation)
{
    const auto match = contentTokenExpression().match(body);
    if (!match.hasMatch())
        return renderTemplateFields(body, context, mode);

    // Render the two user-controlled segments independently, then insert the
    // native block between them. A sentinel marker is unsafe here: a literal
    // field value may legally contain the same character and would otherwise
    // be rewritten into report markup after escaping/validation.
    TemplateRender before = renderTemplateFields(body.left(match.capturedStart()), context, mode);
    TemplateRender after = renderTemplateFields(body.mid(match.capturedEnd()), context, mode);
    TemplateRender rendered;
    rendered.validation.issues += before.validation.issues;
    rendered.validation.issues += after.validation.issues;
    validation->issues += rendered.validation.issues;
    if (rendered.validation.ok())
        rendered.text = before.text + nativeContent + after.text;
    return rendered;
}

} // namespace

int TemplateApplication::protectedContentBlockCount(const QString &body)
{
    int count = 0;
    auto match = contentTokenExpression().globalMatch(body);
    while (match.hasNext()) { match.next(); ++count; }
    return count;
}

TemplateApplicationResult TemplateApplication::apply(const CommunicationTemplate &templateValue,
                                                      const TemplateContext &context,
                                                      const QString &nativeHtml,
                                                      const QString &nativePlainText)
{
    TemplateApplicationResult result;
    if (templateValue.workflow != context.workflow)
        result.validation.addError(QStringLiteral("template-workflow-mismatch"), QStringLiteral("workflow"));
    result.validation.issues += validateTemplateSubject(templateValue.subject).issues;
    const TemplateRender subject = renderTemplateFields(templateValue.subject, context);
    result.validation.issues += subject.validation.issues;
    if (!result.validation.ok()) return result;
    result.subject = subject.text;
    // The stored template itself can be header-safe while a project/name field
    // supplied at review time is not. Validate the expanded header too.
    result.validation.issues += validateTemplateSubject(result.subject).issues;
    if (!result.validation.ok()) return result;

    const int richBlocks = protectedContentBlockCount(templateValue.body);
    if (richBlocks != 1) {
        result.validation.addError(QStringLiteral("template-content-block-required"), QStringLiteral("body"));
        return result;
    }
    const TemplateRender html = renderWithProtectedContent(templateValue.body, context,
                                                            TemplateRenderMode::Html, nativeHtml,
                                                            &result.validation);
    if (!result.validation.ok()) return result;
    result.html = html.text;

    if (templateValue.plainBody.isEmpty()) {
        result.plainText = nativePlainText;
        return result;
    }
    const int plainBlocks = protectedContentBlockCount(templateValue.plainBody);
    if (plainBlocks != 1) {
        result.validation.addError(QStringLiteral("template-content-block-required"), QStringLiteral("plainBody"));
        return result;
    }
    const TemplateRender plain = renderWithProtectedContent(templateValue.plainBody, context,
                                                             TemplateRenderMode::PlainText, nativePlainText,
                                                             &result.validation);
    if (result.validation.ok()) result.plainText = plain.text;
    return result;
}

} // namespace PN::Comm
