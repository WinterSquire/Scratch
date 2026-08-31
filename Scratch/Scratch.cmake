set(ScratchSource
    # 控制器逻辑
    Scratch/Scratch.cpp

    # 遮罩处理算法
    Scratch/Mask/Masking.cpp
    Scratch/Mask/Envelope.cpp
    Scratch/Mask/NoEnvelope.cpp

    # 轮廓处理算法
    Scratch/Contour/Gaussian.cpp
    Scratch/Contour/Skeleton.cpp

    # 序列化
    Scratch/Serialization/CSV.cpp
    Scratch/Serialization/HTML.cpp
    Scratch/Serialization/JSON.cpp

    Scratch/WebView/ScratchReport.cpp
    Scratch/WebView/ScratchReport.hpp
)

# add library
add_library(Scratch STATIC 
    ${ScratchSource}
)

target_include_directories(Scratch PUBLIC Scratch)
target_link_libraries(Scratch PUBLIC ${OpenCV_LIBS} Qt6::Core Qt6::Widgets Qt6::WebView)

if(PROFILE)
    target_compile_definitions(Scratch PUBLIC TRACY_ENABLE)
    target_link_libraries(Scratch PUBLIC Tracy::TracyClient)
endif()