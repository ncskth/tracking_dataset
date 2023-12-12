#pragma once

#include <eigen3/Eigen/Geometry>
#include <eigen3/Eigen/Core>


#define UNDISTORT_K1 -0.04699230178896435
#define UNDISTORT_K2 0.3204033625166395
#define UNDISTORT_K3 0

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define PI 3.141525

#define RAD_TO_DEG (180.0/PI)
#define DEG_TO_RAD (PI/180.0)


Eigen::Vector2<double> position_to_pixel(Eigen::Vector3<double> pos);

Eigen::Vector2<double> undistort_pixel(Eigen::Vector2<double> pixel);