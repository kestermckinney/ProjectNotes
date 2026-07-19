// Copyright (C) 2026 Paul McKinney
#ifndef SUPPORTBUNDLE_H
#define SUPPORTBUNDLE_H

#include <QString>

class QWidget;

// "Send Logs to Support" helper.
//
// Collects every *.log file from the application log directory, compresses
// them into a single timestamped zip on the user's Desktop, reveals that zip
// in the system file manager, and opens a pre-addressed mail draft to support.
// The user attaches the revealed zip and sends. All user-facing errors are
// reported via message boxes (parented to `parent`) and recorded in the log.
namespace SupportBundle
{
    // Returns the support contact address used for the mail draft.
    QString supportEmailAddress();

    // Runs the full flow. Returns true if the zip was created and the mail
    // draft/reveal were launched; false if anything failed (the user is
    // already informed via a message box in that case).
    bool sendLogsToSupport(QWidget *parent);
}

#endif // SUPPORTBUNDLE_H
