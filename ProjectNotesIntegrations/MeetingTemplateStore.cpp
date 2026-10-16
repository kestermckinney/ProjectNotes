// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "MeetingTemplateStore.h"
#include "TemplateParser.h"
#include "databaseobjects.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>

namespace PN::Comm {
namespace {

QJsonObject encode(const MeetingTemplate &value)
{
    return {{"id", value.id}, {"name", value.name}, {"durationMinutes", value.durationMinutes}, {"subject", value.subject},
            {"agenda", value.agenda}};
}

std::optional<MeetingTemplate> decode(const QJsonObject &object)
{
    MeetingTemplate value;
    value.id = object.value("id").toString();
    if (value.id.isEmpty()) return std::nullopt;
    value.name = object.value("name").toString();
    const int duration = object.value("durationMinutes").toInt();
    if (duration > 0) value.durationMinutes = duration;
    value.subject = object.value("subject").toString();
    value.agenda = object.value("agenda").toString();
    return value;
}

MeetingTemplate standardTemplate(const QString &id, const QString &name,
                                 const QString &purpose, const QString &agenda)
{
    return {id, name, 60, QStringLiteral("{{ project.number }} {{ project.name }} - ") + name,
            QStringLiteral("<h3>Purpose</h3><p>") + purpose.toHtmlEscaped()
                + QStringLiteral("</p><h3>Agenda</h3>") + agenda};
}

QList<MeetingTemplate> standardTemplates()
{
    return {
        standardTemplate(
            QStringLiteral("standard-project-kickoff-internal"),
            QStringLiteral("Project Kickoff (Internal)"),
            QStringLiteral("Align the internal team on stakeholders, business drivers, scope, risks, company context, "
                           "and next steps before engaging the customer."),
            QStringLiteral(R"(<ul>
<li>Review Team Roles &amp; Introductions</li>
<li>Sales Overview of Company &amp; Estimate<ul>
<li>Project business purpose</li>
<li>Key business drivers</li>
<li>Stakeholders (people, partners, vendors)</li>
<li>Future sales opportunities / strategy</li>
</ul></li>
<li>Review Proposal<ul>
<li>Scope of work</li>
<li>Deliverables</li>
<li>Invoicing</li>
<li>Assumptions &amp; constraints</li>
</ul></li>
<li>Review Lessons Learned from Prior Similar Projects</li>
<li>Identify Risks</li>
<li>Review Company Standards<ul>
<li>Drafting</li>
<li>Programming</li>
<li>Testing</li>
<li>Other corporate standards</li>
</ul></li>
<li>Additional Items</li>
<li>Next Steps &amp; Action Items</li>
</ul>)")),
        standardTemplate(
            QStringLiteral("standard-project-kickoff"),
            QStringLiteral("Project Kickoff"),
            QStringLiteral("Establish shared understanding with the customer of roles, business drivers, scope, risks, "
                           "schedule expectations, communication protocols, methodology, and standards."),
            QStringLiteral(R"(<ul>
<li>Review Team Roles / Introductions</li>
<li>Review Project Business Drivers</li>
<li>Review Scope of Work<ul>
<li>Deliverables</li>
<li>Assumptions &amp; constraints</li>
<li>Invoicing</li>
</ul></li>
<li>Identify Risks</li>
<li>Review Key Schedule Drivers and Schedule Risks</li>
<li>Review Communication Plan<ul>
<li>Approval authority</li>
<li>Primary contacts</li>
<li>Security and safety items</li>
</ul></li>
<li>Project Methodology<ul>
<li>DeltaV waterfall approach</li>
<li>Syncade workshops &amp; workstreams</li>
</ul></li>
<li>Review Company Standards<ul>
<li>Drafting</li>
<li>Programming</li>
<li>Testing</li>
<li>Other corporate standards</li>
</ul></li>
<li>Additional Items</li>
<li>Next Steps &amp; Action Items</li>
</ul>)")),
        standardTemplate(
            QStringLiteral("standard-status-internal"),
            QStringLiteral("Status Meeting (Internal)"),
            QStringLiteral("Ensure work is progressing, assignments are clear, and roadblocks are identified and addressed."),
            QStringLiteral(R"(<ul>
<li>Review Action Items from Prior Meeting</li>
<li>Provide Updates on Assigned Tasks</li>
<li>Identify Any Roadblocks</li>
<li>Review Tasks to Start</li>
<li>Additional Items</li>
<li>Next Steps &amp; Action Items</li>
</ul>)")),
        standardTemplate(
            QStringLiteral("standard-status"),
            QStringLiteral("Status Meeting"),
            QStringLiteral("Keep the customer informed of progress, confirm understanding of assignments, surface roadblocks, "
                           "and review schedule health."),
            QStringLiteral(R"(<ul>
<li>Review Action Items from Prior Meeting</li>
<li>Provide Updates on Assigned Tasks</li>
<li>Identify Any Roadblocks</li>
<li>Review Tasks to Start</li>
<li>Review Status Report</li>
<li>Review Schedule</li>
<li>Additional Items</li>
<li>Next Steps &amp; Action Items</li>
</ul>)")),
        standardTemplate(
            QStringLiteral("standard-lessons-learned-internal"),
            QStringLiteral("Lessons Learned Meeting (Internal)"),
            QStringLiteral("Collectively identify and document what went well and what needs improvement so future projects "
                           "benefit from the experience."),
            QStringLiteral(R"(<ul>
<li>Project Planning</li>
<li>Project Execution</li>
<li>Testing</li>
<li>Project Communication</li>
<li>Obstacles</li>
<li>On-Site Implementation</li>
<li>Schedule and Budget</li>
<li>Additional Observations</li>
<li>Capture of Actionable Improvements / Best Practices</li>
</ul>)")),
        standardTemplate(
            QStringLiteral("standard-lessons-learned"),
            QStringLiteral("Lessons Learned Meeting"),
            QStringLiteral("Review the project with the customer, capture what worked well and what can be improved, "
                           "and agree on actions for future work."),
            QStringLiteral(R"(<ul>
<li>Project Objectives and Outcomes</li>
<li>Planning and Schedule</li>
<li>Communication and Collaboration</li>
<li>Design, Delivery, and Testing</li>
<li>Implementation and Handover</li>
<li>What Went Well</li>
<li>Opportunities for Improvement</li>
<li>Agreed Actions and Owners</li>
</ul>)")),
        standardTemplate(
            QStringLiteral("standard-project-closeout"),
            QStringLiteral("Project Closeout / Handover Meeting"),
            QStringLiteral("Formally close the project, confirm all deliverables are complete, transfer ownership, "
                           "and capture final administrative items."),
            QStringLiteral(R"(<ul>
<li>Confirm Completion of All Deliverables</li>
<li>Review Open Items / Punch List</li>
<li>Documentation Handover (drawings, code, manuals, as-builts)</li>
<li>Training / Knowledge Transfer Status</li>
<li>Warranty &amp; Support Handover</li>
<li>Final Invoice / Commercial Closeout</li>
<li>Lessons Learned Summary (high-level)</li>
<li>Formal Project Acceptance</li>
<li>Next Steps (if any residual work)</li>
</ul>)")),
        standardTemplate(
            QStringLiteral("standard-risk-issue-review"),
            QStringLiteral("Risk & Issue Review Meeting"),
            QStringLiteral("Proactively review the risk register and current issues, assign owners, "
                           "and decide on mitigation or escalation."),
            QStringLiteral(R"(<ul>
<li>Review Current Risk Register</li>
<li>New Risks Identified</li>
<li>Status of Mitigation Actions</li>
<li>Open Issues &amp; Resolution Plans</li>
<li>Escalation Decisions</li>
<li>Action Items</li>
</ul>)")),
        standardTemplate(
            QStringLiteral("standard-change-control"),
            QStringLiteral("Change Control / Scope Change Meeting"),
            QStringLiteral("Evaluate proposed changes for impact on scope, schedule, cost, and risk, "
                           "and decide on disposition."),
            QStringLiteral(R"(<ul>
<li>Review Proposed Change(s)</li>
<li>Impact Assessment (scope, schedule, cost, risk)</li>
<li>Customer / Stakeholder Input</li>
<li>Recommendation &amp; Decision</li>
<li>Update to Baseline (if approved)</li>
<li>Communication Plan for the Change</li>
<li>Action Items</li>
</ul>)")),
        standardTemplate(
            QStringLiteral("standard-design-review"),
            QStringLiteral("Design / Technical Review Meeting"),
            QStringLiteral("Review design packages, configurations, or architecture for technical correctness, "
                           "standards compliance, and readiness for the next phase."),
            QStringLiteral(R"(<ul>
<li>Review Design Package / Documents Under Review</li>
<li>Standards Compliance Check</li>
<li>Open Technical Questions / Clarifications</li>
<li>Risk / Constructability / Maintainability Feedback</li>
<li>Decisions Required</li>
<li>Approval or Conditional Approval</li>
<li>Action Items &amp; Next Review Gate</li>
</ul>)")),
        standardTemplate(
            QStringLiteral("standard-fat-sat-readiness"),
            QStringLiteral("FAT / SAT Planning or Readiness Review"),
            QStringLiteral("Confirm test plans, resources, success criteria, and logistics are ready before execution "
                           "of Factory or Site Acceptance Testing."),
            QStringLiteral(R"(<ul>
<li>Review Test Plan &amp; Procedures</li>
<li>Success / Pass-Fail Criteria</li>
<li>Resource &amp; Schedule Confirmation</li>
<li>Customer Participation Expectations</li>
<li>Logistics (hardware, software versions, environment, remote access)</li>
<li>Open Issues That Could Impact Testing</li>
<li>Go / No-Go Decision</li>
<li>Action Items</li>
</ul>)")),
        standardTemplate(
            QStringLiteral("standard-steering-committee"),
            QStringLiteral("Steering Committee / Executive Status Meeting"),
            QStringLiteral("Provide high-level visibility to sponsors and executives on overall health, "
                           "major risks, and decisions needed."),
            QStringLiteral(R"(<ul>
<li>Overall Project Health Snapshot</li>
<li>Key Accomplishments Since Last Meeting</li>
<li>Schedule &amp; Budget Summary</li>
<li>Major Risks &amp; Issues Requiring Attention</li>
<li>Decisions Needed from Steering Committee</li>
<li>Upcoming Milestones</li>
<li>Questions &amp; Discussion</li>
</ul>)")),
        standardTemplate(
            QStringLiteral("standard-requirements-discovery"),
            QStringLiteral("Requirements / Discovery Workshop"),
            QStringLiteral("Elicit and validate detailed requirements, especially useful early in Syncade "
                           "or complex migration projects."),
            QStringLiteral(R"(<ul>
<li>Workshop Objectives &amp; Ground Rules</li>
<li>Current-State Overview</li>
<li>Desired Future-State Discussion</li>
<li>Detailed Requirements Capture (by area / workstream)</li>
<li>Assumptions, Constraints &amp; Dependencies</li>
<li>Prioritization</li>
<li>Open Questions</li>
<li>Next Steps &amp; Documentation Plan</li>
</ul>)"))
    };
}

} // namespace

QStringList meetingTemplateFieldPaths()
{
    return {QStringLiteral("project.number"), QStringLiteral("project.name"), QStringLiteral("company.name"),
            QStringLiteral("preferences.managerName"), QStringLiteral("preferences.managingCompanyName"),
            QStringLiteral("current.date")};
}

QString MeetingTemplateStore::storageKey()
{
    return QStringLiteral("Meeting/v1/Templates");
}

QList<MeetingTemplate> MeetingTemplateStore::templates(ValidationResult *validation) const
{
    if (!global_DBObjects.isOpen()) {
        if (validation) validation->addError("template-store-unavailable", storageKey());
        return {};
    }
    const QByteArray stored = global_DBObjects.loadParameter(storageKey()).toUtf8();
    const QJsonDocument document = QJsonDocument::fromJson(stored);
    // As with email templates, an empty value is a valid first run but a
    // malformed one must not be silently treated as "no meeting types".
    if ((!stored.trimmed().isEmpty() && document.isNull()) || (!document.isNull() && !document.isArray())) {
        if (validation) validation->addError("template-store-corrupt", storageKey());
        return {};
    }
    QList<MeetingTemplate> result;
    for (const QJsonValue &entry : document.array()) {
        const auto value = decode(entry.toObject());
        if (!value) { if (validation) validation->addError("template-store-corrupt", storageKey()); continue; }
        result.append(*value);
    }
    return result;
}

bool MeetingTemplateStore::write(const QList<MeetingTemplate> &values)
{
    QJsonArray array; for (const MeetingTemplate &value : values) array.append(encode(value));
    return global_DBObjects.saveParameter(storageKey(),
                                          QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact)));
}

ValidationResult MeetingTemplateStore::initializeDefaults()
{
    ValidationResult result;
    const QList<MeetingTemplate> values = templates(&result);
    if (!result.ok() || !values.isEmpty()) return result;
    if (!write(standardTemplates())) result.addError("template-store-write-failed", storageKey());
    return result;
}

ValidationResult MeetingTemplateStore::resetDefaults()
{
    ValidationResult result;
    if (!global_DBObjects.isOpen()) {
        result.addError("template-store-unavailable", storageKey());
    } else if (!write(standardTemplates())) {
        result.addError("template-store-write-failed", storageKey());
    }
    return result;
}

ValidationResult MeetingTemplateStore::save(MeetingTemplate value)
{
    ValidationResult result;
    if (value.id.trimmed().isEmpty() || value.name.trimmed().isEmpty()) {
        result.addError("template-invalid", "template"); return result;
    }
    // Incomplete drafts may be stored for later repair, but a CR/LF in the
    // subject is unsafe to persist as a message header.
    for (const ValidationIssue &issue : validateTemplateSubject(value.subject).issues)
        if (issue.code != QStringLiteral("template-subject-required"))
            result.issues.append(issue);
    if (!result.ok()) return result;
    if (value.durationMinutes <= 0) value.durationMinutes = 60;
    QList<MeetingTemplate> values = templates(&result);
    if (!result.ok()) return result;
    const auto existing = std::find_if(values.begin(), values.end(),
                                       [&value](const MeetingTemplate &entry) { return entry.id == value.id; });
    if (existing != values.end()) *existing = value;
    else values.append(value);
    if (!write(values)) result.addError("template-store-write-failed", storageKey());
    return result;
}

ValidationResult MeetingTemplateStore::remove(const QString &id)
{
    ValidationResult result;
    if (id.trimmed().isEmpty()) {
        result.addError("template-invalid", "template");
        return result;
    }
    QList<MeetingTemplate> values = templates(&result);
    if (!result.ok()) return result;
    const auto end = std::remove_if(values.begin(), values.end(),
                                    [&id](const MeetingTemplate &value) { return value.id == id; });
    if (end != values.end()) {
        values.erase(end, values.end());
        if (!write(values)) result.addError("template-store-write-failed", storageKey());
    }
    return result;
}

} // namespace PN::Comm
