cmake_minimum_required(VERSION 4.2)

project(Scratch)

option(PROFILE "Enable profiling" OFF)

# get required package
find_package(OpenCV REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Tracy)

include("Scratch/Scratch.cmake")
include("UnitTest/UnitTest.cmake")