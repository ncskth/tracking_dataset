#include <eigen3/Eigen/Geometry>
#include <eigen3/Eigen/Core>
#include <map>
#include <opencv2/calib3d.hpp>
#include <vector>

#include "iostream"
#include "camera_stuff.h"
#include "protocol.h"

std::map<int, std::vector<Eigen::Vector3<double>>> id_to_polygon;

// Camera matrix and distortion coefficients
cv::Mat cameraMatrix = (cv::Mat_<double>(3, 3) << CAMERA_MATRIX_INIT_CV);
cv::Mat distCoeffs = (cv::Mat_<double>(1, 5) << UNDISTORT_K1, UNDISTORT_K2, UNDISTORT_P1, UNDISTORT_P2, UNDISTORT_K3);
cv::Mat zeroDistCoeffs = (cv::Mat_<double>(1, 5) << 0, 0, 0, 0, 0);

cv::Mat rVec = (cv::Mat_<double>(1, 3) << 0, 0, 0);
cv::Mat tVec = (cv::Mat_<double>(1, 3) << 0, 0, 0);
cv::Mat identity = (cv::Mat_<double>(3, 3) << 1,0,0,0,1,0,0,0,1);

// open cv
Eigen::Vector2<double> position_to_pixel(Eigen::Vector3<double> pos, bool mirror) {
    Eigen::Vector2<double> out;

    std::vector<cv::Point3f> rawPositions;
    rawPositions.push_back(cv::Point3f(pos[0], -pos[1], -pos[2]));

    // Undistort the points
    std::vector<cv::Point2f> projectedPositions;
    cv::projectPoints(rawPositions, rVec, tVec, cameraMatrix, zeroDistCoeffs, projectedPositions);
    out[0] = projectedPositions[0].x;
    out[1] = projectedPositions[0].y;
    if (mirror) {
        out[0] = 1280 - out[0];
    }
    // out = undistort_pixel(out);

    return out;
}

// open cv
Eigen::Vector2<double> undistort_pixel(Eigen::Vector2<double> pixel, bool mirror) {
    Eigen::Vector2<double> out;

    std::vector<cv::Point2f> distortedPoints;
    distortedPoints.push_back(cv::Point2f(pixel[0], pixel[1]));

    // Camera matrix and distortion coefficients

    double fx = cameraMatrix.at<double>(0, 0);
    double fy = cameraMatrix.at<double>(1, 1);
    double cx = cameraMatrix.at<double>(0, 2);
    double cy = cameraMatrix.at<double>(1, 2);


    // Undistort the points
    std::vector<cv::Point2f> undistortedPoints;

    cv::undistortPoints(distortedPoints, undistortedPoints, cameraMatrix, distCoeffs);

    out[0] = (undistortedPoints[0].x * fx + cx);
    out[1] = (undistortedPoints[0].y * fx + cy);

    if (mirror) {
        out[0] = 1280 - out[0];
    }

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
    const double triangle_offset =  -0;
    const double triangle_width = 0.29;
    const double triangle_height =  triangle_width / 2 * tan(60 * DEG_TO_RAD);
    const double triangle_center = triangle_width * sqrt(3) / 3;
    id_to_polygon[TRIANGLE0] = {
        {-(triangle_center), 0, 0},
        {triangle_height - triangle_center, 0, -triangle_width / 2},
        {triangle_height - triangle_center, 0, triangle_width / 2},
        {-(triangle_center), 0, 0}
    };

    id_to_polygon[OLD_RECT] = {
        {-rectangle_width / 2.0, 0, -rectengle_height / 2.0},
    };

    id_to_polygon[WAND] = {
        {-rectangle_width / 2.0, 0, -rectengle_height / 2.0},
    };


    float offset_x = 0.28 / 2 - 0.022154 - 0.05;
    float offset_z = 0.22 / 2 + 0.1 + 0.05;
    id_to_polygon[BLOB0] = {
        {offset_x, 0, offset_z},
        {offset_x + 0.022154, 0, offset_z},
        {offset_x + 0.022154, 0, offset_z - 0.1},
        {offset_x + 0.022154 + 0.05, 0, offset_z - 0.05 - 0.1},
        {offset_x + 0.022154 + 0.05, 0, offset_z - 0.05 - 0.1 - 0.22},
        {offset_x + 0.022154 + 0.05 - 0.28, 0, offset_z - 0.05 - 0.1 - 0.22},
        {offset_x + 0.022154 + 0.05 - 0.28, 0, offset_z - 0.05 - 0.1 - 0.22 + 0.25},
        {offset_x, 0, offset_z},
    };

    // circle
    const double circle_radius = 0.292/2;
    int iterations = 150;
    for (int i = 0; i <= iterations; i++) {
        id_to_polygon[CIRCLE0].push_back({
            sin(2 * PI * i / iterations) * circle_radius,
            0,
            cos(2 * PI * i / iterations) * circle_radius,
        });
    }
}