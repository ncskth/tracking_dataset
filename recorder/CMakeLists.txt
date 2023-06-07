project(recorder)

cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-O3")

find_package(MetavisionSDK COMPONENTS core driver ui REQUIRED)

add_library(libnatnet SHARED IMPORTED)
set_target_properties(libnatnet
    PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libnatnet/libNatNet.so)

add_executable(event_counter event_counter.cpp)

add_executable(visualizer visualizer.cpp)
target_link_libraries(visualizer -lSDL2)


add_executable(main main.cpp camera.cpp optitrack.cpp)
target_link_libraries(main MetavisionSDK::core MetavisionSDK::driver MetavisionSDK::ui)
target_link_libraries(main libnatnet)