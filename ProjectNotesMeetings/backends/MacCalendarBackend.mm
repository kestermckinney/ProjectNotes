// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "MacCalendarBackend.h"

#import <AppKit/AppKit.h>
#import <EventKit/EventKit.h>

namespace PN::Comm {
namespace {

// Apple recommends one long-lived store per app; it is never released.
EKEventStore *sharedStore()
{
    static EKEventStore *store = [[EKEventStore alloc] init];
    return store;
}

bool hasFullAccess(EKAuthorizationStatus status)
{
    if (@available(macOS 14.0, *))
        return status == EKAuthorizationStatusFullAccess;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    return status == EKAuthorizationStatusAuthorized;
#pragma clang diagnostic pop
}

bool isUsableCalendar(EKCalendar *calendar)
{
    return calendar && calendar.allowsContentModifications && calendar.type != EKCalendarTypeBirthday
        && calendar.type != EKCalendarTypeSubscription;
}

// The user's default calendar for new events, or else the first calendar
// that takes new events (not Birthdays or a subscribed calendar).
EKCalendar *targetCalendar(EKEventStore *store)
{
    EKCalendar *calendar = store.defaultCalendarForNewEvents;
    if (isUsableCalendar(calendar))
        return calendar;
    for (EKCalendar *candidate in [store calendarsForEntityType:EKEntityTypeEvent]) {
        if (isUsableCalendar(candidate))
            return candidate;
    }
    return nil;
}

void bringCalendarForward()
{
    NSURL *appUrl = [[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier:@"com.apple.iCal"];
    if (!appUrl)
        return;
    NSWorkspaceOpenConfiguration *configuration = [NSWorkspaceOpenConfiguration configuration];
    configuration.activates = YES;
    [[NSWorkspace sharedWorkspace] openApplicationAtURL:appUrl configuration:configuration completionHandler:nil];
}

ServiceError accessDenied()
{
    return {QStringLiteral("mac-calendar-access-denied"),
            QStringLiteral("Project Notes does not have access to your calendars. Enable it in System Settings > "
                           "Privacy & Security > Calendars (Full Access), then try again."),
            RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
}

} // namespace

void MacCalendarBackend::handoff(MeetingRequest request, Completion completion)
{
    if (m_cancelled.remove(request.operationId)) {
        MeetingHandoffResult result;
        result.operationId = request.operationId;
        result.error = {QStringLiteral("operation-cancelled"), {}, RetryKind::None, OutcomeCertainty::Certain};
        completion(result);
        return;
    }

    const EKAuthorizationStatus status = [EKEventStore authorizationStatusForEntityType:EKEntityTypeEvent];
    if (status != EKAuthorizationStatusNotDetermined) {
        saveEvent(std::move(request), std::move(completion));
        return;
    }

    // The request completes on an arbitrary queue; the rest of the handoff
    // (and the completion) runs on the main thread.
    auto finish = [this, request, completion]() {
        dispatch_async(dispatch_get_main_queue(), ^{
            saveEvent(request, completion);
        });
    };
    if (@available(macOS 14.0, *)) {
        [sharedStore() requestFullAccessToEventsWithCompletion:^(BOOL, NSError *) {
            finish();
        }];
    } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        [sharedStore() requestAccessToEntityType:EKEntityTypeEvent
                                      completion:^(BOOL, NSError *) {
                                          finish();
                                      }];
#pragma clang diagnostic pop
    }
}

void MacCalendarBackend::saveEvent(MeetingRequest request, Completion completion)
{
    MeetingHandoffResult result;
    result.operationId = request.operationId;
    if (m_cancelled.remove(request.operationId)) {
        result.error = {QStringLiteral("operation-cancelled"), {}, RetryKind::None, OutcomeCertainty::Certain};
        completion(result);
        return;
    }

    if (!hasFullAccess([EKEventStore authorizationStatusForEntityType:EKEntityTypeEvent])) {
        result.error = accessDenied();
        completion(result);
        return;
    }

    QString calendarName;
    @autoreleasepool {
        EKEventStore *store = sharedStore();
        // A store created before access was granted can hold a stale, empty
        // view of the user's calendars.
        [store refreshSourcesIfNecessary];
        EKCalendar *calendar = targetCalendar(store);
        if (!calendar) {
            result.error = {QStringLiteral("mac-calendar-script-failed"),
                            QStringLiteral("No writable calendar is available."), RetryKind::ReviewAndRetry,
                            OutcomeCertainty::Certain};
            completion(result);
            return;
        }

        EKEvent *event = [EKEvent eventWithEventStore:store];
        event.calendar = calendar;
        event.title = request.subject.toNSString();
        event.location = request.location.toNSString();
        // The agenda alone; attendees are pasted in by the user (see the header).
        event.notes = request.plainAgenda.trimmed().toNSString();
        event.startDate = request.start.toNSDate();
        event.endDate = request.end.toNSDate();

        NSError *error = nil;
        if (![store saveEvent:event span:EKSpanThisEvent commit:YES error:&error]) {
            NSLog(@"[PN MacCalendar] save failed: %@", error);
            result.error = {QStringLiteral("mac-calendar-script-failed"),
                            error ? QString::fromNSString(error.localizedDescription) : QString(),
                            RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
            completion(result);
            return;
        }
        calendarName = QString::fromNSString(calendar.title);
        bringCalendarForward();
    }

    // The calendar name only; DesktopAppController names it in its
    // confirmation, since it is chosen automatically.
    result.certainty = OutcomeCertainty::Certain;
    result.eventIdentity = calendarName;
    completion(result);
}

void MacCalendarBackend::cancel(const QUuid &operationId)
{
    m_cancelled.insert(operationId);
}

} // namespace PN::Comm
