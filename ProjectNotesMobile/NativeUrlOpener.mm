// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "NativeUrlOpener.h"

#if defined(Q_OS_IOS)
#import <UIKit/UIKit.h>

bool NativeUrlOpener::openRawUrl(const QString& uri)
{
    NSURL *nsurl = [NSURL URLWithString:uri.toNSString()];
    if (!nsurl) return false;

    UIApplication *app = [UIApplication sharedApplication];
    if (![app canOpenURL:nsurl]) return false;

    [app openURL:nsurl options:@{} completionHandler:nil];
    return true;
}

#else // non-iOS stub

bool NativeUrlOpener::openRawUrl(const QString&) { return false; }

#endif
