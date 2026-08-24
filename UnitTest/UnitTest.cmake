add_executable(ScratchTest
    UnitTest/Test.cpp
    UnitTest/AnalyseScratchTest.cpp
    UnitTest/SerializeScratchTest.cpp
)

target_link_libraries(ScratchTest Scratch)