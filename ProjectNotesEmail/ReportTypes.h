#pragma once
#include "EmailTypes.h"
#include <QPageLayout>
namespace PN::Comm {
struct ReportDocument { Workflow workflow=Workflow::TrackerItemsReport; QString htmlDocument,emailFragment,defaultSubject,fileStem; QPageLayout pdfLayout; };
}
