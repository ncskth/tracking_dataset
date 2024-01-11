#pragma once

#include <eigen3/Eigen/Geometry>
#include <eigen3/Eigen/Core>


#define UNDISTORT_K1 -0.09559888418664889

#define UNDISTORT_K2 0.415120236918694
#define UNDISTORT_K3 0
#define UNDISTORT_P1 -0.0001747786982571883
#define UNDISTORT_P2 0.001924572657234908

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define PI 3.141525

#define RAD_TO_DEG (180.0/PI)
#define DEG_TO_RAD (PI/180.0)

#define CAMERA_MATRIX_INIT_CV 1684.004555487931, 0, 639.9317701157134, \
 0, 1684.004555487931, 352.0646675706986, \
 0, 0, 1\

extern std::map<int, std::vector<Eigen::Vector3<double>>> id_to_polygon;

Eigen::Vector2<double> position_to_pixel(Eigen::Vector3<double> pos);

Eigen::Vector2<double> undistort_pixel(Eigen::Vector2<double> pixel);

void populate_id_to_polygons();