#ifndef HELPBROWSER_H
#define HELPBROWSER_H

#include <QTextBrowser>
#include <QHelpEngine>

class HelpBrowser : public QTextBrowser
{

    Q_OBJECT

public:
    HelpBrowser(QWidget* parent = 0);
    ~HelpBrowser();

    QHelpEngine* helpEngine() { return m_helpengine; }

    QVariant loadResource (int type, const QUrl& name) override;

private:
    QHelpEngine* m_helpengine = nullptr;
};

#endif // HELPBROWSER_H
