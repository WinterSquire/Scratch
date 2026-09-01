#include <wrl.h>
#include <QVBoxLayout>
#include <QWebView>
#include "WebView2.h"
#include <Scratch.hpp>

#include "ScratchReport.hpp"

using Microsoft::WRL::ComPtr;

extern "C"
{
    extern const char index_html_start[];
    extern const char index_html_end[];
}

#define HOST_NAME L"app.local"

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

inline void openFolder(class ICoreWebView2_3* webview, QString folder)
{
    webview->SetVirtualHostNameToFolderMapping(
        HOST_NAME,
        (LPCWSTR)folder.utf16(),
        COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS
    );

    webview->Navigate(L"https://" HOST_NAME L"/index.html");
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
        EventRegistrationToken token;
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

        if (!mFolder.isEmpty())
            openFolder(mWebview, mFolder);
    }, Qt::SingleShotConnection);

    webview->setUrl(QUrl("about:blank"));
}

CScratchReport::~CScratchReport()
{
    if (mWebview)
        mWebview->Release();
}

void CScratchReport::setFolder(QString folder)
{
    mFolder = folder;

    if (!mWebview)
        return;

    openFolder(mWebview, mFolder);
}
