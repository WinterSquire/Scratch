#include <QtWebView>
#include <QApplication>

#include <WebView/ScratchReport.hpp>
#include <Scratch.hpp>

int main(int argc, char** argv)
{
    QtWebView::initialize();
    QApplication app(argc, argv);

    CScratchReport resport;

    resport.open("D:\\work\\Project\\Scratch\\Data\\HTML", "index.html");

    resport.show();
    
    return app.exec();
}