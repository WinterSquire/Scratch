
add_executable(WebApp
    WebApp/Main.cpp
)

target_link_libraries(WebApp PRIVATE Qt6::Widgets Qt6::WebView Scratch)