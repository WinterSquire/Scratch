set(ScratchSource
    # 控制器逻辑
    Scratch/Scratch.cpp

    # 遮罩处理算法
    Scratch/Mask/Masking.cpp
    Scratch/Mask/Envelope.cpp
    Scratch/Mask/NoEnvelope.cpp

    # 轮廓处理算法
    Scratch/Contour/Gaussian.cpp
)

# add library
add_library(Scratch STATIC 
    ${ScratchSource}
)

target_include_directories(Scratch PUBLIC Scratch)
target_link_libraries(Scratch PUBLIC ${OpenCV_LIBS} Qt6::Core)