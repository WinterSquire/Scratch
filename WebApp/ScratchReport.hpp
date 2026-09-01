#pragma once

#include <QString>
#include <QWidget>

class CScratchReport : public QWidget
{
    Q_OBJECT
public:
    CScratchReport(QWidget* paremt = NULL);
    ~CScratchReport();

    void setFolder(QString folder);

private:
    QString mFolder;
    class ICoreWebView2_3* mWebview;
};
