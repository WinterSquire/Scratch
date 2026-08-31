#pragma once

#include <QString>
#include <QWidget>

class CScratchReport : public QWidget
{
    Q_OBJECT
public:
    CScratchReport(QWidget* paremt = NULL);
    ~CScratchReport();

    int open(QString folder, QString file);

private:
    int dispatchOpen();

private:
    QString mFolder, mFile;
    class ICoreWebView2_3* mWebview;
};
