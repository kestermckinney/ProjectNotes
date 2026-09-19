// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "SnapshotTypes.h"
#include "ProjectNotesIntegrations/TemplateTypes.h"

namespace PN::Comm {

// The template context every workflow shares: project, client, manager and
// managing-company fields plus report.type. The editor offers each of these
// fields for every workflow, so each one must resolve (to an empty string when
// the snapshot has no value) or the whole review fails to prepare. Factories
// add their workflow-specific meeting.* and report.* fields on top.
[[nodiscard]] TemplateContext communicationTemplateContext(const CommunicationSnapshot &snapshot,
                                                           Workflow workflow);

// Stable Yes/No rendering for report.internal.
[[nodiscard]] QString templateBoolean(bool value);

} // namespace PN::Comm
