
add_executable(DBApp
    DBAPP/Main.cpp
    DBAPP/ScratchAnalyser.cpp
    Data/Resource.obj
)

target_link_libraries(DBApp PRIVATE Qt6::Core Qt6::Sql Scratch)