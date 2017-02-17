#ifndef VOXBLOX_SIMULATION_OBJECTS_H_
#define VOXBLOX_SIMULATION_OBJECTS_H_

#include <algorithm>
#include <iostream>

#include "voxblox/core/common.h"
#include "voxblox/core/layer.h"
#include "voxblox/core/voxel.h"

// Heavily inspired by @mfehr's OccupancyGridGenerator.
namespace voxblox {

// Base class for objects.
// Should this be full virtual?
class Object {
 public:
  // A wall is an infinite plane.
  enum Type { kSphere = 0, kCube, kPlane };

  Object(const Point& center, Type type)
      : Object(center, type, Color::White()) {}
  Object(const Point& center, Type type, const Color& color)
      : center_(center), type_(type), color_(color) {}
  virtual ~Object() {}

  // Map-building accessors.
  virtual FloatingPoint getDistanceToPoint(const Point& point) const {
    return 0.0;
  }
  Color getColor() const { return color_; }

  // Raycasting accessors.
  virtual bool getRayIntersection(const Point& ray_origin,
                                  const Point& ray_direction,
                                  FloatingPoint max_dist,
                                  Point* intersect_point,
                                  FloatingPoint* intersect_dist) const {
    return false;
  }

 protected:
  Point center_;
  Type type_;
  Color color_;
};

class Sphere : public Object {
 public:
  Sphere(const Point& center, FloatingPoint radius)
      : Object(center, Type::kSphere), radius_(radius) {}
  Sphere(const Point& center, FloatingPoint radius, const Color& color)
      : Object(center, Type::kSphere, color), radius_(radius) {}

  virtual FloatingPoint getDistanceToPoint(const Point& point) const {
    FloatingPoint distance = (center_ - point).norm() - radius_;
    return distance;
  }

  virtual bool getRayIntersection(const Point& ray_origin,
                                  const Point& ray_direction,
                                  FloatingPoint max_dist,
                                  Point* intersect_point,
                                  FloatingPoint* intersect_dist) const {
    // From https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection
    // x = o + dl is the ray equation
    // r = sphere radius, c = sphere center
    FloatingPoint under_square_root =
        pow(ray_direction.dot(ray_origin - center_), 2) -
        (ray_origin - center_).squaredNorm() + pow(radius_, 2);

    // No real roots = no intersection.
    if (under_square_root < 0.0) {
      return false;
    }

    FloatingPoint d =
        -(ray_direction.dot(ray_origin - center_)) - sqrt(under_square_root);

    // Intersection behind the origin.
    if (d < 0.0) {
      return false;
    }
    // Intersection greater than max dist, so no intersection in the sensor
    // range.
    if (d > max_dist) {
      return false;
    }

    *intersect_point = ray_origin + d * ray_direction;
    *intersect_dist = d;
    return true;
  }

 protected:
  FloatingPoint radius_;
};

class Cube : public Object {
 public:
  Cube(const Point& center, const Point& size)
      : Object(center, Type::kCube), size_(size) {}
  Cube(const Point& center, const Point& size, const Color& color)
      : Object(center, Type::kCube, color), size_(size) {}

  virtual FloatingPoint getDistanceToPoint(const Point& point) const {
    // Solution from http://stackoverflow.com/questions/5254838/
    // calculating-distance-between-a-point-and-a-rectangular-box-nearest-point

    Point distance_vector = Point::Zero();
    distance_vector.x() =
        std::max(std::max(center_.x() - size_.x() - point.x(), 0.0),
                 point.x() - center_.x() - size_.x());
    distance_vector.y() =
        std::max(std::max(center_.y() - size_.y() - point.y(), 0.0),
                 point.y() - center_.y() - size_.y());
    distance_vector.z() =
        std::max(std::max(center_.z() - size_.z() - point.z(), 0.0),
                 point.z() - center_.z() - size_.z());

    // TODO(helenol): check if inside, and see if the 0s there are necessary.

    FloatingPoint distance = distance_vector.norm();

    // Basically 0... Means it's inside!
    if (distance < 1e-6) {
      distance_vector.x() = std::max(center_.x() - size_.x() - point.x(),
                                     point.x() - center_.x() - size_.x());
      distance_vector.y() = std::max(center_.y() - size_.y() - point.y(),
                                     point.y() - center_.y() - size_.y());
      distance_vector.z() = std::max(center_.z() - size_.z() - point.z(),
                                     point.z() - center_.z() - size_.z());
      distance = distance_vector.maxCoeff();
    }

    return distance;
  }

 protected:
  Point size_;
};

// Requires normal being passed in to ALREADY BE NORMALIZED!!!!
class Plane : public Object {
 public:
  Plane(const Point& center, const Point& normal)
      : Object(center, Type::kPlane), normal_(normal) {}
  Plane(const Point& center, const Point& normal, const Color& color)
      : Object(center, Type::kPlane, color), normal_(normal) {}

  virtual FloatingPoint getDistanceToPoint(const Point& point) const {
    // Compute the 'd' in ax + by + cz + d = 0:
    // This is actually the scalar product I guess.
    FloatingPoint d = -normal_.dot(center_);
    FloatingPoint p = d / normal_.norm();

    FloatingPoint distance = normal_.dot(point) + p;
    return distance;
  }

  virtual bool getRayIntersection(const Point& ray_origin,
                                  const Point& ray_direction,
                                  FloatingPoint max_dist,
                                  Point* intersect_point,
                                  FloatingPoint* intersect_dist) const {
    // From https://en.wikipedia.org/wiki/Line%E2%80%93plane_intersection
    // Following notation of sphere more...
    // x = o + dl is the ray equation
    // n = normal, c = plane 'origin'
    FloatingPoint denominator = ray_direction.dot(normal_);
    if (std::abs(denominator) < 1e-6) {
      // Lines are parallel, no intersection.
      return false;
    }
    FloatingPoint d = (center_ - ray_origin).dot(normal_) / denominator;
    std::cout << "d: " << d << std::endl;
    if (d < 0.0) {
      return false;
    }
    if (d > max_dist) {
      return false;
    }
    *intersect_point = ray_origin + d * ray_direction;
    *intersect_dist = d;
    return true;
  }

 protected:
  Point normal_;
};

}  // namespace voxblox

#endif  // VOXBLOX_SIMULATION_OBJECTS_H_
