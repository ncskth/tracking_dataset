#include <eigen3/Eigen/Geometry>
#include <eigen3/Eigen/Core>

#include "camera_stuff.h"

#include "iostream"





Eigen::Vector2<double> position_to_pixel(Eigen::Vector3<double> pos) {
    Eigen::Vector2<double> out;
    Eigen::Matrix<double, 3, 4> camera_matrix;
    // Eigen::Vector3<double> {2.7 , 4.2, 1};
    camera_matrix << 1753.384, 0, 610.8581, 0,
                     0, 1753.384, 369, 0,
                     0, 0, 1, 0;

    Eigen::Matrix<double, 4,1> long_pos;
    long_pos = {pos.x(), pos.y(), pos.z(), 1};
    Eigen::Vector3<double> test = camera_matrix * long_pos;
    test /= test.z();
    out.x() = 1280 - test.x();
    out.y() = test.y();
    return out;
}

Eigen::Vector2<double> undistort_pixel(Eigen::Vector2<double> pixel) {
    Eigen::Vector2<double> out;
    pixel[0] -= WINDOW_WIDTH / 2;
    pixel[1] -= WINDOW_HEIGHT / 2;
    double r = sqrt(pow(pixel[0], 2) + pow(pixel[1], 2));
    // r /= sqrt(pow(WINDOW_WIDTH, 2) + pow(WINDOW_HEIGHT, 2));
    r /= sqrt(pow(WINDOW_WIDTH, 2) + pow(WINDOW_HEIGHT, 2) / 2);
    // r /= WINDOW_WIDTH / 2;
    // r /= WINDOW_HEIGHT / 2;
    // r = 0;
    double correction = 1 + UNDISTORT_K1 * pow(r, 2) + UNDISTORT_K2 * pow(r, 4) + UNDISTORT_K3 * pow(r, 6);
    out[0] = pixel[0] / correction;
    out[1] = pixel[1] / correction;
    out[0] += WINDOW_WIDTH / 2;
    out[1] += WINDOW_HEIGHT / 2;
    return out;
}