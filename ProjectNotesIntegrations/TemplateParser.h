#pragma once
#include "TemplateTypes.h"
namespace PN::Comm {
TemplateRender renderTemplateFields(const QString &, const TemplateContext &,
                                    TemplateRenderMode mode = TemplateRenderMode::PlainText);
ValidationResult validateTemplateSubject(const QString &subject);
}
