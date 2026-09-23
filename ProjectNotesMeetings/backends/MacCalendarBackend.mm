// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "MacCalendarBackend.h"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
// AEDeterminePermissionToAutomateTarget/AECreateDesc live in CoreServices'
// AE.framework, not ApplicationServices (AXIsProcessTrusted, above, is the
// one that's actually in ApplicationServices/HIServices).
#import <CoreServices/CoreServices.h>
#import <EventKit/EventKit.h>

namespace PN::Comm {
namespace {

// AppleScript string literals support the same \\, \", \n, \r, \t escapes as
// C. Everything embedded in the generated script (subject, location, notes)
// comes from user-entered project data, so it must be escaped rather than
// trusted as script source.
QString escapeAppleScriptLiteral(QString value)
{
    value.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    value.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    value.replace(QStringLiteral("\r\n"), QStringLiteral("\\n"));
    value.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
    value.replace(QLatin1Char('\r'), QStringLiteral("\\n"));
    value.replace(QLatin1Char('\t'), QStringLiteral("\\t"));
    return value;
}

// Sets a date variable's components with `day` pinned to 1 first, so an
// intermediate year/month combination can never overflow into the wrong
// month (e.g. setting month on the 31st of a 30-day month).
QString appleScriptDateAssignment(const QString &variableName, const QDateTime &value)
{
    const QDateTime local = value.toLocalTime();
    const QDate date = local.date();
    const QTime time = local.time();
    return QStringLiteral(
               "set %1 to current date\n"
               "set day of %1 to 1\n"
               "set year of %1 to %2\n"
               "set month of %1 to %3\n"
               "set day of %1 to %4\n"
               "set hours of %1 to %5\n"
               "set minutes of %1 to %6\n"
               "set seconds of %1 to 0\n")
        .arg(variableName)
        .arg(date.year())
        .arg(date.month())
        .arg(date.day())
        .arg(time.hour())
        .arg(time.minute());
}

// NSAppleScript is expected to prompt for Automation consent itself, on the
// first Apple Event sent to a new target, then let that same event through
// once approved. In practice this has proven unreliable: the very event that
// triggers the dialog can still come back rejected even after the user
// clicks Allow, with no further indication anything went wrong. Asking here
// first — synchronously, before the script that actually creates the event —
// means the real script is never the one racing that dialog.
//
// The check only works against a running target: with Calendar closed it
// returns procNotFound (-600) without prompting, so Calendar is launched
// (in the background) first and the check retried until it has come up.
bool ensureAutomationPermission(NSString *targetBundleIdentifier)
{
    NSData *bundleIdData = [targetBundleIdentifier dataUsingEncoding:NSUTF8StringEncoding];
    AEAddressDesc targetDesc;
    const OSStatus createStatus =
        AECreateDesc(typeApplicationBundleID, bundleIdData.bytes, (Size)bundleIdData.length, &targetDesc);
    if (createStatus != noErr) {
        NSLog(@"[PN MacCalendar] AECreateDesc failed: %d", (int)createStatus);
        return true; // Could not build the descriptor; let the script itself surface any real failure.
    }

    if ([NSRunningApplication runningApplicationsWithBundleIdentifier:targetBundleIdentifier].count == 0) {
        if (NSURL *appUrl = [[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier:targetBundleIdentifier]) {
            NSWorkspaceOpenConfiguration *configuration = [NSWorkspaceOpenConfiguration configuration];
            configuration.activates = NO;
            [[NSWorkspace sharedWorkspace] openApplicationAtURL:appUrl
                                                  configuration:configuration
                                              completionHandler:nil];
        }
    }

    OSStatus permission = procNotFound;
    for (int attempt = 0; attempt < 60; ++attempt) { // ~15 seconds
        permission = AEDeterminePermissionToAutomateTarget(&targetDesc, typeWildCard, typeWildCard, true);
        if (permission != procNotFound)
            break;
        [NSThread sleepForTimeInterval:0.25];
    }
    AEDisposeDesc(&targetDesc);
    NSLog(@"[PN MacCalendar] AEDeterminePermissionToAutomateTarget(%@) = %d", targetBundleIdentifier,
          (int)permission);
    // Only an explicit refusal is reported as one. Anything else (Calendar
    // still not up, an unexpected status) falls through to the script, which
    // launches Calendar itself and surfaces a real error if one remains.
    return permission != errAEEventNotPermitted && permission != errAEEventWouldRequireUserConsent;
}

// The description shown in Calendar is the agenda alone. Attendees can't be
// added to the event without Calendar auto-sending invitations, so the review
// dialog offers "Copy Attendees" for pasting into the invitees field instead.
QString buildNotes(const MeetingRequest &request)
{
    return request.plainAgenda.trimmed();
}

QString buildAppleScript(const MeetingRequest &request, bool includeAutoOpen)
{
    const QString subject = escapeAppleScriptLiteral(request.subject);
    const QString location = escapeAppleScriptLiteral(request.location);
    const QString notes = escapeAppleScriptLiteral(buildNotes(request));

    QString script;
    script += QStringLiteral("use scripting additions\n\n");
    script += QStringLiteral("set evtTitle to \"%1\"\n").arg(subject);
    script += QStringLiteral("set evtLocation to \"%1\"\n").arg(location);
    script += QStringLiteral("set evtNotes to \"%1\"\n\n").arg(notes);
    script += appleScriptDateAssignment(QStringLiteral("startDate"), request.start);
    script += QLatin1Char('\n');
    script += appleScriptDateAssignment(QStringLiteral("endDate"), request.end);
    script += QLatin1Char('\n');
    script += QStringLiteral(
        "tell application \"Calendar\"\n"
        "    launch\n"
        "    activate\n"
        "    with timeout of 30 seconds\n"
        "        set targetCal to missing value\n"
        "        set allCals to every calendar whose writable is true\n"
        "        repeat with aCal in allCals\n"
        "            set calName to name of aCal\n"
        "            if calName is not in {\"Birthdays\", \"US Holidays\", \"Siri Suggestions\", \"Scheduled "
        "Reminders\"} then\n"
        "                set targetCal to aCal\n"
        "                exit repeat\n"
        "            end if\n"
        "        end repeat\n"
        "        if targetCal is missing value then\n"
        "            if (count of allCals) > 0 then\n"
        "                set targetCal to item 1 of allCals\n"
        "            else\n"
        "                error \"No writable calendar is available.\"\n"
        "            end if\n"
        "        end if\n"
        "        set targetCalName to name of targetCal\n"
        "        set newEvent to make new event at end of events of targetCal with properties "
        "{summary:evtTitle, location:evtLocation, description:evtNotes, start date:startDate, end date:endDate}\n"
        "        view calendar at startDate\n"
        "        show newEvent\n"
        "        activate\n"
        "    end timeout\n"
        "end tell\n");
    if (includeAutoOpen) {
        script += QStringLiteral(
            "\nignoring application responses\n"
            "    try\n"
            "        tell application \"System Events\"\n"
            "            tell process \"Calendar\"\n"
            "                set frontmost to true\n"
            "                delay 2\n"
            "                keystroke return\n"
            "            end tell\n"
            "        end tell\n"
            "    end try\n"
            "end ignoring\n");
    }
    // The script's overall result — read back so the caller can confirm to
    // the user which calendar the event actually landed on, since it is
    // chosen automatically and may not be the one they were last looking at.
    script += QStringLiteral("\nreturn targetCalName\n");
    return script;
}

} // namespace

void MacCalendarBackend::handoff(MeetingRequest request, Completion completion)
{
    MeetingHandoffResult result;
    result.operationId = request.operationId;
    if (m_cancelled.remove(request.operationId)) {
        result.error = {QStringLiteral("operation-cancelled"), {}, RetryKind::None, OutcomeCertainty::Certain};
        completion(result);
        return;
    }

    // Automation consent alone is not enough: Calendar also refuses scripting
    // commands ("Event Not Permitted") from a sender without Calendars access
    // (Privacy & Security > Calendars). The prompt is only possible when the
    // bundle declares NSCalendarsFullAccessUsageDescription; unbundled debug
    // builds have no Info.plist, so they go straight to the script and rely
    // on whatever access their launching app (Terminal, an IDE) already has.
    const EKAuthorizationStatus status = [EKEventStore authorizationStatusForEntityType:EKEntityTypeEvent];
    const bool canPrompt =
        [[NSBundle mainBundle] objectForInfoDictionaryKey:@"NSCalendarsFullAccessUsageDescription"] != nil;
    if (status != EKAuthorizationStatusNotDetermined || !canPrompt) {
        runScript(std::move(request), std::move(completion));
        return;
    }

    EKEventStore *store = [[EKEventStore alloc] init];
    // The request completes on an arbitrary queue; hop back to the main
    // thread, where the rest of the handoff (and NSAppleScript) must run.
    auto finish = [this, request, completion](BOOL, NSError *) {
        dispatch_async(dispatch_get_main_queue(), ^{
            runScript(request, completion);
        });
    };
    if (@available(macOS 14.0, *)) {
        [store requestFullAccessToEventsWithCompletion:^(BOOL granted, NSError *error) {
            finish(granted, error);
            (void)store; // keep the store alive until the request completes
        }];
    } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        [store requestAccessToEntityType:EKEntityTypeEvent
                              completion:^(BOOL granted, NSError *error) {
                                  finish(granted, error);
                                  (void)store;
                              }];
#pragma clang diagnostic pop
    }
}

void MacCalendarBackend::runScript(MeetingRequest request, Completion completion)
{
    MeetingHandoffResult result;
    result.operationId = request.operationId;
    if (m_cancelled.remove(request.operationId)) {
        result.error = {QStringLiteral("operation-cancelled"), {}, RetryKind::None, OutcomeCertainty::Certain};
        completion(result);
        return;
    }

    const EKAuthorizationStatus status = [EKEventStore authorizationStatusForEntityType:EKEntityTypeEvent];
    if (status == EKAuthorizationStatusDenied || status == EKAuthorizationStatusRestricted) {
        result.error = {QStringLiteral("mac-calendar-access-denied"),
                        QStringLiteral("Project Notes does not have access to your calendars. Enable it in System "
                                       "Settings > Privacy & Security > Calendars (Full Access), then try again."),
                        RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
        completion(result);
        return;
    }

    // Accessibility access is never requested here (that would mean stopping
    // the user mid-flow to send them to System Settings); the auto-open step
    // is simply skipped when it has not already been granted.
    const bool canAutoOpen = AXIsProcessTrusted();
    const QString script = buildAppleScript(request, canAutoOpen);
    QString targetCalendarName;

    @autoreleasepool {
        if (!ensureAutomationPermission(@"com.apple.iCal")) {
            result.error = {QStringLiteral("mac-calendar-automation-denied"),
                            QStringLiteral("Calendar automation was not allowed. Enable it in System Settings > "
                                          "Privacy & Security > Automation > Project Notes > Calendar, then try "
                                          "again."),
                            RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
            completion(result);
            return;
        }

        NSAppleScript *appleScript = [[NSAppleScript alloc] initWithSource:script.toNSString()];
        NSDictionary *errorInfo = nil;
        NSAppleEventDescriptor *scriptResult = [appleScript executeAndReturnError:&errorInfo];
        if (errorInfo) {
            NSLog(@"[PN MacCalendar] script failed: %@", errorInfo);
            // Calendar answers a sender lacking Calendars access with the same
            // -1743 as a missing Automation grant, so its own message points at
            // the wrong setting.
            if ([errorInfo[NSAppleScriptErrorNumber] intValue] == errAEEventNotPermitted
                && [EKEventStore authorizationStatusForEntityType:EKEntityTypeEvent] != EKAuthorizationStatusFullAccess) {
                result.error = {QStringLiteral("mac-calendar-access-denied"),
                                QStringLiteral("Calendar refused the new event because Project Notes does not have "
                                               "full access to your calendars. Enable it in System Settings > "
                                               "Privacy & Security > Calendars, then try again."),
                                RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
                completion(result);
                return;
            }
            NSString *message = errorInfo[NSAppleScriptErrorMessage];
            result.error = {QStringLiteral("mac-calendar-script-failed"),
                            message ? QString::fromNSString(message) : QString(), RetryKind::ReviewAndRetry,
                            OutcomeCertainty::Certain};
            completion(result);
            return;
        }
        if (NSString *calendarName = scriptResult.stringValue)
            targetCalendarName = QString::fromNSString(calendarName);
    }

    result.certainty = OutcomeCertainty::Uncertain;
    // The calendar name only — DesktopAppController turns this into a
    // confirmation naming the calendar, since it is chosen automatically and
    // may not be the one the user was last looking at.
    result.eventIdentity = targetCalendarName;
    completion(result);
}

void MacCalendarBackend::cancel(const QUuid &operationId)
{
    m_cancelled.insert(operationId);
}

} // namespace PN::Comm
