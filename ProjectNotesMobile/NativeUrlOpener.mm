// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "NativeUrlOpener.h"

#if defined(Q_OS_IOS)
#import <UIKit/UIKit.h>
#import <SafariServices/SafariServices.h>

bool NativeUrlOpener::openRawUrl(const QString& uri)
{
    NSURL *nsurl = [NSURL URLWithString:uri.toNSString()];
    if (!nsurl) return false;

    UIApplication *app = [UIApplication sharedApplication];
    if (![app canOpenURL:nsurl]) return false;

    [app openURL:nsurl options:@{} completionHandler:nil];
    return true;
}

// Finds the key window's root view controller to present from — there's no
// simpler "current" accessor once an app can have multiple scenes.
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

bool NativeUrlOpener::openInBrowser(const QString& uri)
{
    NSURL *nsurl = [NSURL URLWithString:uri.toNSString()];
    if (!nsurl) return false;

    UIViewController *presenter = topViewController();
    if (!presenter) return false;

    // Must present on the main thread; callers may not already be on it.
    dispatch_async(dispatch_get_main_queue(), ^{
        SFSafariViewController *safari = [[SFSafariViewController alloc] initWithURL:nsurl];
        [presenter presentViewController:safari animated:YES completion:nil];
    });
    return true;
}

#else // non-iOS stub

bool NativeUrlOpener::openRawUrl(const QString&) { return false; }
bool NativeUrlOpener::openInBrowser(const QString&) { return false; }

#endif
