// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "CommunicationDiagnostics.h"

#include <QCoreApplication>
#include <QHash>

namespace PN::Comm {
namespace {

QString tr(const char *text) { return QCoreApplication::translate("PN::Comm", text); }

} // namespace

QString diagnosticDisplayText(const QString &code)
{
    // mailto carries the whole message inside a URL, so its limits are about
    // length and content rather than connectivity. Every message names the way
    // out, because the user cannot inspect the generated URL themselves.
    if (code == QLatin1String("mailto-url-too-long"))
        return tr("This email is too long to open in your default mail client. "
                  "Choose Microsoft 365 or Thunderbird as the email client in Settings, "
                  "or save the report and attach it manually.");
    if (code == QLatin1String("mailto-unsupported-content"))
        return tr("Your default mail client cannot receive formatted text or attachments this way. "
                  "Choose Microsoft 365 or Thunderbird as the email client in Settings.");
    if (code == QLatin1String("mailto-header-injection"))
        return tr("The subject contains a line break, which cannot be sent to your default mail client. "
                  "Remove the line break and try again.");
    if (code == QLatin1String("mailto-launch-failed"))
        return tr("Your default mail client could not be started. "
                  "Check that a mail client is set as the system default for email.");
    if (code == QLatin1String("mailto-launch-already-attempted"))
        return tr("This email was already handed to your mail client. "
                  "Check for an open compose window before preparing it again.");

    if (code == QLatin1String("thunderbird-unavailable"))
        return tr("Thunderbird was not found at the configured path. "
                  "Correct the Thunderbird location in Settings.");
    if (code == QLatin1String("thunderbird-launch-failed"))
        return tr("Thunderbird could not be started. Check the Thunderbird location in Settings.");
    if (code == QLatin1String("thunderbird-body-unavailable")
        || code == QLatin1String("thunderbird-path-not-operation-owned"))
        return tr("The prepared message body could not be handed to Thunderbird. "
                  "Prepare the email again.");

    if (code == QLatin1String("graph-service-unavailable"))
        return tr("Microsoft 365 is not connected. Sign in to Microsoft 365 in Settings.");
    if (code == QLatin1String("graph-draft-create-failed")
        || code == QLatin1String("graph-draft-id-missing"))
        return tr("Microsoft 365 did not create the draft. "
                  "Check your connection and Microsoft 365 sign-in, then try again.");
    if (code == QLatin1String("graph-upload-session-failed")
        || code == QLatin1String("graph-upload-chunk-failed")
        || code == QLatin1String("graph-attachment-upload-failed"))
        return tr("The attachment could not be uploaded to the Microsoft 365 draft. "
                  "Check your connection and try again; an incomplete draft may be left in Drafts.");

    if (code == QLatin1String("attachment-integrity-mismatch"))
        return tr("A generated attachment changed after it was prepared. Prepare the email again.");
    if (code == QLatin1String("attachment-read-failed")
        || code == QLatin1String("attachment-unreadable")
        || code == QLatin1String("attachment-unavailable"))
        return tr("An attachment could not be read. Prepare the email again.");

    if (code == QLatin1String("email-backend-unavailable"))
        return tr("The selected email client is not available. Choose an email client in Settings.");
    if (code == QLatin1String("email-operation-busy"))
        return tr("Another email is still being handed off. Wait for it to finish, then try again.");
    if (code == QLatin1String("operation-cancelled"))
        return tr("The email was cancelled.");

    if (code == QLatin1String("email-subject-required"))
        return tr("The email needs a subject.");
    if (code == QLatin1String("email-body-required"))
        return tr("The email has no body to send.");
    if (code == QLatin1String("email-header-injection"))
        return tr("The subject contains a line break. Remove it and try again.");
    if (code == QLatin1String("email-attachment-required"))
        return tr("The report attachment was not generated. Prepare the report again.");

    if (code == QLatin1String("recipient-required"))
        return tr("Select at least one recipient, or choose to address the email later.");
    if (code == QLatin1String("recipient-address-invalid"))
        return tr("A recipient has an invalid email address. Correct it in the person's record.");
    if (code == QLatin1String("recipient-address-duplicate"))
        return tr("The same email address is listed more than once. Remove the duplicate.");
    if (code == QLatin1String("recipient-role-invalid"))
        return tr("A recipient has an invalid To/Cc/Bcc setting.");

    if (code == QLatin1String("report-workflow-mismatch")
        || code == QLatin1String("report-review-required"))
        return tr("The prepared report no longer matches this window. Prepare the report again.");
    if (code == QLatin1String("operation-id-required"))
        return tr("The email was not prepared correctly. Prepare it again.");

    return {};
}

} // namespace PN::Comm
