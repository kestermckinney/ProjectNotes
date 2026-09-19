// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "sqlquerymodel.h"
class ProjectEmailAudiencesModel : public SqlQueryModel {
public:
    explicit ProjectEmailAudiencesModel(DatabaseObjects *dbo);
};
