#pragma once

#include <eigen3/Eigen/Geometry>
#include <eigen3/Eigen/Core>


#define UNDISTORT_K1 -0.095116746166
#define UNDISTORT_K2 0.4070337898
#define UNDISTORT_K3 0
#define UNDISTORT_P1 -0.00302149026747
#define UNDISTORT_P2 -0.0016091353910

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define PI 3.141525

#define RAD_TO_DEG (180.0/PI)
#define DEG_TO_RAD (PI/180.0)


extern std::map<int, std::vector<Eigen::Vector3<double>>> id_to_polygon;

Eigen::Vector2<double> position_to_pixel(Eigen::Vector3<double> pos);

Eigen::Vector2<double> undistort_pixel(Eigen::Vector2<double> pixel);

void populate_id_to_polygons();