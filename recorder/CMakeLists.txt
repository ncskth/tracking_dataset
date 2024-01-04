project(recorder)

cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O3")
find_package(MetavisionSDK COMPONENTS core driver ui REQUIRED)
find_package(OpenCV REQUIRED)

add_library(libnatnet SHARED IMPORTED)
set_target_properties(libnatnet
    PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libnatnet/libNatNet.so)

add_executable(stat_analyzer stat_analyzer.cpp)

add_executable(event_counter event_counter.cpp)

add_executable(data_converter data_converter.cpp camera_stuff.cpp)
target_link_libraries(data_converter -llz4)
target_link_libraries(data_converter -lhdf5_serial)
target_include_directories(data_converter PRIVATE /usr/include/hdf5/serial/)
target_include_directories(data_converter PRIVATE /usr/include/opencv4)
target_link_libraries(data_converter -lopencv_calib3d)
target_link_libraries(data_converter ${OpenCV_LIBS})

add_executable(visualizer visualizer.cpp camera_stuff.cpp)
target_link_libraries(visualizer -lSDL2)
target_link_libraries(visualizer -lopencv_calib3d)
target_include_directories(visualizer PRIVATE /usr/include/opencv4)
target_link_libraries(visualizer ${OpenCV_LIBS})

add_executable(main main.cpp camera.cpp optitrack.cpp recorder_visualizer.cpp camera_stuff.cpp)
target_link_libraries(main MetavisionSDK::core MetavisionSDK::driver MetavisionSDK::ui)
target_link_libraries(main libnatnet)
target_link_libraries(main -lSDL2)
target_link_libraries(main ${OpenCV_LIBS})
# target_link_libraries(main -lopencv_calib3d)
# target_link_libraries(main -lopencv_core)


target_include_directories(main PRIVATE /usr/include/opencv4)
