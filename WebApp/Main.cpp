#include <QtWebView>
#include <QApplication>

#include <Scratch.hpp>
#include "ScratchReport.hpp"

int main(int argc, char** argv)
{
    QtWebView::initialize();
    QApplication app(argc, argv);

    CScratchReport resport;

    resport.open("./", "index.html");

    resport.show();
    
    return app.exec();
}