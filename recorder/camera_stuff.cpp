#include <eigen3/Eigen/Geometry>
#include <eigen3/Eigen/Core>
#include <map>

#include "camera_stuff.h"
#include "protocol.h"

std::map<int, std::vector<Eigen::Vector3<double>>> id_to_polygon;



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

void populate_id_to_polygons() {
    // rectangle
    const double rectangle_width = 0.4;
    const double rectengle_height = 0.3;
    id_to_polygon[RECTANGLE0] = {
        {-rectangle_width / 2.0, 0, -rectengle_height / 2.0},
        {rectangle_width / 2.0, 0, -rectengle_height / 2.0},
        {rectangle_width / 2.0, 0, rectengle_height / 2.0},
        {-rectangle_width / 2.0, 0, rectengle_height / 2.0},
        {-rectangle_width / 2.0, 0, -rectengle_height / 2.0},
    };

    id_to_polygon[CHECKERBOARD] = {
        {-rectengle_height / 2.0, 0, -rectangle_width / 2.0},
        {rectengle_height / 2.0, 0, -rectangle_width / 2.0},
        {rectengle_height / 2.0, 0, rectangle_width / 2.0},
        {-rectengle_height / 2.0, 0, rectangle_width / 2.0},
        {-rectengle_height / 2.0, 0, -rectangle_width / 2.0},
    };

    // square
    const double square_width = 0.29;
    id_to_polygon[SQUARE0] = {
        {-square_width / 2.0, 0, -square_width / 2.0},
        {square_width / 2.0, 0, -square_width / 2.0},
        {square_width / 2.0, 0, square_width / 2.0},
        {-square_width / 2.0, 0, square_width / 2.0},
        {-square_width / 2.0, 0, -square_width / 2.0},
    };

    // triangle
    const double triangle_width = 0.29;
    const double triangle_height =  triangle_width / 2 * tan(60 * DEG_TO_RAD);
    id_to_polygon[TRIANGLE0] = {
        {-triangle_height / 2, 0, 0},
        {triangle_height / 2, 0, -triangle_width / 2},
        {triangle_height / 2, 0, triangle_width / 2},
        {-triangle_height / 2, 0, 0}
    };

    // plier
    id_to_polygon[PLIER0] = {
        {-0.025, 0, -0.15},
        {0, 0, 0},
        {0, 0, 0.045},
        {0, 0, 0},
        {0.025, 0, -0.15}
    };

    // hammer
    id_to_polygon[HAMMER0] = {
        {0, 0, -0.10},
        {0, 0, 0.155},
        {0.04, 0, 0.12},
        {0, 0, 0.155},
        {-0.04, 0, 0.155},
    };
    id_to_polygon[HAMMER1] = {
        {0, 0, -0.10},
        {0, 0, 0.155},
        {0.04, 0, 0.12},
        {0, 0, 0.155},
        {-0.04, 0, 0.155},
    };
    id_to_polygon[HAMMER_NEW] = {
        {0, 0, -0.24},
        {0, 0, 0.015},
        {0.05, 0, -0.03},
        {0, 0, 0.015},
        {-0.05, 0, 0.015},
    };
    id_to_polygon[HAMMER_NEW_NEW] = {
        {0, 0, -0.24},
        {0, 0, 0.015},
        {0.05, 0, -0.03},
        {0, 0, 0.015},
        {-0.05, 0, 0.015},
    };

    id_to_polygon[OLD_RECT] = {
        {-rectangle_width / 2.0, 0, -rectengle_height / 2.0},
    };



    id_to_polygon[BLOB0] = {
        {-0.14, 0, 0.08},
        {-0.14, 0, -0.17},
        {0.14, 0, -0.17},
        {0.14, 0, 0.05},
        {0.09, 0, 0.095},
        {0.09, 0, 0.195},
        {0.07, 0, 0.195},
        {-0.14, 0, 0.08},
    };

    // circle
    const double circle_radius = 0.29/2;
    int iterations = 16;
    for (int i = 0; i <= iterations; i++) {
        id_to_polygon[CIRCLE0].push_back({
            sin(2 * PI * i / iterations) * circle_radius,
            0,
            cos(2 * PI * i / iterations) * circle_radius,
        });
    }
}