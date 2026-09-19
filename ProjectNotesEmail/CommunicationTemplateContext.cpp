// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "CommunicationTemplateContext.h"

namespace PN::Comm {
namespace {

QString reportTypeName(Workflow workflow)
{
    switch (workflow) {
    case Workflow::SendMeetingNotes: return QStringLiteral("Meeting Notes");
    case Workflow::MeetingNotesReport: return QStringLiteral("Meeting Notes Report");
    case Workflow::StatusReport: return QStringLiteral("Status Report");
    case Workflow::TrackerItemsReport: return QStringLiteral("Tracker Items Report");
    }
    return {};
}

} // namespace

TemplateContext communicationTemplateContext(const CommunicationSnapshot &snapshot, Workflow workflow)
{
    TemplateContext context;
    context.workflow = workflow;
    context.values.insert(QStringLiteral("project.number"), snapshot.projectNumber);
    context.values.insert(QStringLiteral("project.name"), snapshot.projectName);
    context.values.insert(QStringLiteral("client.name"), snapshot.clientName);
    context.values.insert(QStringLiteral("preferences.managerName"), snapshot.projectManagerName);
    context.values.insert(QStringLiteral("preferences.managingCompanyName"), snapshot.managingCompanyName);
    context.values.insert(QStringLiteral("report.type"), reportTypeName(workflow));
    return context;
}

QString templateBoolean(bool value)
{
    return value ? QStringLiteral("Yes") : QStringLiteral("No");
}

} // namespace PN::Comm
