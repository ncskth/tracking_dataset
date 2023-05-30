project(recorder)

cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)
find_package(MetavisionSDK COMPONENTS core driver ui REQUIRED)

add_library(libnatnet SHARED IMPORTED)
set_target_properties(libnatnet
    PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libnatnet/libNatNet.so)

add_executable(main main.cpp camera.cpp optitrack.cpp)
add_executable(event_counter event_counter.cpp)


target_link_libraries(main MetavisionSDK::core MetavisionSDK::driver MetavisionSDK::ui)
target_link_libraries(main libnatnet)