// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "CommunicationTemplateStore.h"
#include "TemplateTypes.h"

namespace PN::Comm {

// The only template token which carries already-rendered native content.  It
// is deliberately not in the insertable field catalog: users can move the
// protected block but cannot interpolate arbitrary HTML through normal fields.
inline constexpr auto NativeContentTemplateField = "content.body";

struct TemplateApplicationResult {
    QString subject;
    QString html;
    QString plainText;
    ValidationResult validation;
};

class TemplateApplication final {
public:
    // `nativeHtml` and `nativePlainText` are produced by a native report
    // builder.  A non-empty template body must contain exactly one protected
    // content token, so surrounding prose is safe while report markup survives.
    static TemplateApplicationResult apply(const CommunicationTemplate &templateValue,
                                           const TemplateContext &context,
                                           const QString &nativeHtml,
                                           const QString &nativePlainText);
    static int protectedContentBlockCount(const QString &body);
};

} // namespace PN::Comm
