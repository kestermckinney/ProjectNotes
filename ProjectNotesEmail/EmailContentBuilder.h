#pragma once
#include "ReportTypes.h"
namespace PN::Comm {
struct EmailPreparation { QUuid operationId; SourceContext source; quint64 previewRevision=0; BackendId backend=BackendId::Mailto; EmailMode mode=EmailMode::InlineHtml; QString accountKey; quint64 accountGeneration=0; QList<EmailAddress> recipients; ReportDocument document; QList<Artifact> attachments; QList<Artifact> exports; QString projectFolderPath; QString retainedHtmlExportSubfolder; bool addressLater=false; bool retainHtml=false; bool displayPdf=false; bool createHtmlExport=false; bool createPdfExport=false; };
struct EmailPreparationResult { std::optional<EmailRequest> request; ValidationResult validation; bool noEmail=false; };
class EmailContentBuilder final { public: static EmailPreparationResult prepare(const EmailPreparation &); };
}
