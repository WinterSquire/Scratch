cmake_minimum_required(VERSION 4.2)

project(Scratch)

set(CMAKE_AUTOMOC ON)
option(PROFILE "Enable profiling" OFF)

# get required package
find_package(OpenCV REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets WebView Sql)
find_package(Tracy)

include("Scratch/Scratch.cmake")
include("UnitTest/UnitTest.cmake")
include("WebApp/WebApp.cmake")
include("DBApp/DBApp.cmake")