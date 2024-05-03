#pragma once

#include <eigen3/Eigen/Geometry>
#include <eigen3/Eigen/Core>

#define UNDISTORT_K1 -0.0994876
#define UNDISTORT_K2 0.66786257
#define UNDISTORT_K3 -1.08571509
#define UNDISTORT_P1 -0.01207911
#define UNDISTORT_P2 0.00450405

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define PI 3.141525

#define RAD_TO_DEG (180.0/PI)
#define DEG_TO_RAD (PI/180.0)

#define CAMERA_MATRIX_INIT_CV 1700.74372, 0, 650.172869, \
                              0, 1700.74372, 285.398327, \
                              0, 0, 1

extern std::map<int, std::vector<Eigen::Vector3<double>>> id_to_polygon;
extern std::map<int, std::vector<Eigen::Vector3<double>>> id_to_extremes;

Eigen::Vector2<double> position_to_pixel(Eigen::Vector3<double> pos, bool mirror = false);

Eigen::Vector2<double> undistort_pixel(Eigen::Vector2<double> pixel, bool mirror = false);

void populate_id_to_polygons();