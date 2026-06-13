// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "MailComposer.h"

#if defined(Q_OS_IOS)
#import <MessageUI/MessageUI.h>
#import <UIKit/UIKit.h>

@interface MailComposerDelegate : NSObject <MFMailComposeViewControllerDelegate>
@end

@implementation MailComposerDelegate
- (void)mailComposeController:(MFMailComposeViewController *)controller
          didFinishWithResult:(MFMailComposeResult)result
                        error:(NSError *)error
{
    [controller dismissViewControllerAnimated:YES completion:nil];
}
@end

static MailComposerDelegate *s_delegate = nil;

bool MailComposer::isAvailable()
{
    return [MFMailComposeViewController canSendMail];
}

void MailComposer::present(const QStringList& toAddresses,
                           const QString&     subject,
                           const QString&     body)
{
    if (![MFMailComposeViewController canSendMail])
        return;

    NSMutableArray<NSString *> *recipients = [NSMutableArray array];
    for (const QString& addr : toAddresses)
        [recipients addObject:addr.toNSString()];

    MFMailComposeViewController *vc = [[MFMailComposeViewController alloc] init];
    if (!s_delegate)
        s_delegate = [[MailComposerDelegate alloc] init];
    vc.mailComposeDelegate = s_delegate;
    [vc setToRecipients:recipients];
    [vc setSubject:subject.toNSString()];
    [vc setMessageBody:body.toNSString() isHTML:NO];

    // Walk connected scenes to find a window, then walk to the topmost presented VC
    UIViewController *rootVC = nil;
    for (UIScene *scene in [UIApplication sharedApplication].connectedScenes) {
        if ([scene isKindOfClass:[UIWindowScene class]]) {
            UIWindowScene *ws = (UIWindowScene *)scene;
            for (UIWindow *w in ws.windows) {
                if (w.isKeyWindow) { rootVC = w.rootViewController; break; }
            }
            if (rootVC) break;
        }
    }
    while (rootVC.presentedViewController)
        rootVC = rootVC.presentedViewController;

    if (rootVC)
        [rootVC presentViewController:vc animated:YES completion:nil];
}

#else // non-iOS stub

bool MailComposer::isAvailable() { return false; }
void MailComposer::present(const QStringList&, const QString&, const QString&) {}

#endif
