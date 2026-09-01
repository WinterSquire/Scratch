#include <wrl.h>
#include <QVBoxLayout>
#include <QWebView>
#include "WebView2.h"
#include <Scratch.hpp>

#include "ScratchReport.hpp"

#define PREFIX_HTTPS "https://"
#define HOST_NAME "app.local"
#define _WTEXT(text) L##text 
#define WTEXT(text) _WTEXT(text)

using Microsoft::WRL::ComPtr;

inline ICoreWebView2* getWebView2(QWebView& webview)
{
    struct _Container {
        char _[0x28];
        struct {
            char _[0x28];
            ICoreWebView2* webview;
        } *_private;
    } *_container = (_Container*)&webview;

    return _container->_private->webview;
}

CScratchReport::CScratchReport(QWidget* parent) : 
    QWidget(parent), mWebview(NULL)
{
    setMinimumSize(QSize(1280, 720));

    auto webview = new QWebView();
    auto widget = QWidget::createWindowContainer(webview, this);
    auto layout = new QVBoxLayout(this);

    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(widget);

    connect(webview, &QWebView::loadingChanged, this, [this, webview](const QWebViewLoadingInfo &loadingInfo){
        ComPtr<ICoreWebView2_3> webview3;
        ComPtr<ICoreWebView2Settings> settings;
        ComPtr<ICoreWebView2Settings3> setting3;

        getWebView2(*webview)->QueryInterface(IID_PPV_ARGS(&webview3));
        webview3->get_Settings(&settings);
        settings->QueryInterface(IID_PPV_ARGS(&setting3));

        setting3->put_AreDevToolsEnabled(FALSE);
        setting3->put_AreDefaultContextMenusEnabled(FALSE);
        setting3->put_AreBrowserAcceleratorKeysEnabled(FALSE);

        mWebview = webview3.Get();
        mWebview->AddRef();

        if (!mFolder.isEmpty() && !mFile.isEmpty())
            dispatchOpen();
    }, Qt::SingleShotConnection);

    webview->setUrl(QUrl("about:blank"));
}

CScratchReport::~CScratchReport()
{
    if (mWebview)
        mWebview->Release();
}

int CScratchReport::open(QString folder, QString file)
{
    mFolder = folder;
    mFile = file;

    if (!mWebview)
        return ScratchErrorDelay;

    dispatchOpen();

    return ScratchErrorSuccess;
}

int CScratchReport::dispatchOpen()
{
    mWebview->SetVirtualHostNameToFolderMapping(
        WTEXT(HOST_NAME),
        (LPCWSTR)mFolder.utf16(),
        COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS
    );

    auto url = PREFIX_HTTPS HOST_NAME "/" + mFile;

    mWebview->Navigate((LPCWSTR)url.utf16());

    return 0;
}
