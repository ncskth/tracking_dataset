#pragma once

#include "flow_struct.h"
#include <map>
#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Geometry>

struct tracked_object {
    int id;
    Eigen::Vector3<double> position;
    Eigen::Quaternion<double> attitude;
};

extern std::map<int, tracked_object> tracked_objects;

void init_optitrack(struct flow_struct &data);