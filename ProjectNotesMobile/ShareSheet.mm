// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "ShareSheet.h"

#if defined(Q_OS_IOS)
#import <UIKit/UIKit.h>

static UIViewController *topViewController()
{
    UIViewController *root = nil;
    for (UIWindowScene *scene in UIApplication.sharedApplication.connectedScenes) {
        if (![scene isKindOfClass:[UIWindowScene class]]) continue;
        for (UIWindow *window in ((UIWindowScene *)scene).windows) {
            if (window.isKeyWindow) { root = window.rootViewController; break; }
        }
        if (root) break;
    }
    while (root.presentedViewController) root = root.presentedViewController;
    return root;
}

bool ShareSheet::shareFile(const QString& path)
{
    NSURL *url = [NSURL fileURLWithPath:path.toNSString()];
    if (![[NSFileManager defaultManager] fileExistsAtPath:url.path]) return false;

    UIViewController *presenter = topViewController();
    if (!presenter) return false;

    dispatch_async(dispatch_get_main_queue(), ^{
        UIActivityViewController *sheet =
            [[UIActivityViewController alloc] initWithActivityItems:@[url] applicationActivities:nil];
        // iPad presents the sheet as a popover, which must have an anchor or
        // UIKit throws. Center it with no arrow, since the long-pressed row
        // isn't a UIView.
        UIPopoverPresentationController *popover = sheet.popoverPresentationController;
        if (popover) {
            popover.sourceView = presenter.view;
            popover.sourceRect = CGRectMake(CGRectGetMidX(presenter.view.bounds),
                                            CGRectGetMidY(presenter.view.bounds), 0, 0);
            popover.permittedArrowDirections = 0;
        }
        [presenter presentViewController:sheet animated:YES completion:nil];
    });
    return true;
}

#else // non-iOS stub

bool ShareSheet::shareFile(const QString&) { return false; }

#endif
