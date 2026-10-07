// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "MacMailBackend.h"
#include <QFileInfo>

#import <AppKit/AppKit.h>

namespace PN::Comm {

void MacMailBackend::handoff(EmailRequest request, Completion completion)
{
    EmailHandoffResult result;
    result.operationId = request.operationId;
    if (m_cancelled.remove(request.operationId)) {
        result.error = {QStringLiteral("operation-cancelled"), {}, RetryKind::None, OutcomeCertainty::Certain};
        completion(result);
        return;
    }

    @autoreleasepool {
        // NSSharingService's compose-email item has one flat recipients list, so
        // To and Cc addresses both land there; Bcc is dropped rather than shown
        // as a visible recipient (EmailContentBuilder warns about this before
        // the reviewer ever gets here).
        NSMutableArray<NSString *> *recipients = [NSMutableArray array];
        for (const EmailAddress &recipient : request.recipients) {
            if (recipient.role == RecipientRole::Bcc)
                continue;
            const QString address = recipient.address.trimmed();
            if (address.isEmpty())
                continue;
            [recipients addObject:address.toNSString()];
        }

        NSData *htmlData = [request.html.toNSString() dataUsingEncoding:NSUTF8StringEncoding];
        NSAttributedString *body = request.html.isEmpty()
            ? [[NSAttributedString alloc] initWithString:@""]
            : [[NSAttributedString alloc] initWithHTML:htmlData documentAttributes:nil];
        if (!body) {
            result.error = {QStringLiteral("mac-mail-body-invalid"), {}, RetryKind::ReviewAndRetry,
                            OutcomeCertainty::Certain};
            completion(result);
            return;
        }

        NSMutableArray *items = [NSMutableArray arrayWithObject:body];
        for (const Artifact &attachment : request.attachments) {
            const QFileInfo file(attachment.absolutePath);
            if (!file.isFile() || !file.isReadable()) {
                result.error = {QStringLiteral("attachment-source-invalid"),
                                QStringLiteral("An email attachment is no longer available."),
                                RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
                completion(result);
                return;
            }
            [items addObject:[NSURL fileURLWithPath:file.absoluteFilePath().toNSString()]];
        }
        NSSharingService *service = [NSSharingService sharingServiceNamed:NSSharingServiceNameComposeEmail];
        if (!service || ![service canPerformWithItems:items]) {
            result.error = {QStringLiteral("mac-mail-unavailable"),
                            QStringLiteral("No mail account is configured in Mail."), RetryKind::ReviewAndRetry,
                            OutcomeCertainty::Certain};
            completion(result);
            return;
        }

        service.subject = request.subject.toNSString();
        service.recipients = recipients;
        [service performWithItems:items];
    }

    // Mail.app opens the compose window asynchronously and reports nothing
    // back to the caller once handed off.
    result.certainty = OutcomeCertainty::Uncertain;
    completion(result);
}

void MacMailBackend::cancel(const QUuid &operationId)
{
    m_cancelled.insert(operationId);
}

} // namespace PN::Comm
