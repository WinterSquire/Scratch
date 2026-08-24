cmake_minimum_required(VERSION 4.2)

project(Scratch)

# get required package
find_package(OpenCV REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core)

include("Scratch/Scratch.cmake")
include("UnitTest/UnitTest.cmake")