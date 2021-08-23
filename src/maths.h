#pragma once

#include <cmath>

#define PI 3.1415926535897932385f

struct Vector3 {
    float x, y, z;

    Vector3() : x{ 0 }, y{ 0 }, z{ 0 } {}
    Vector3(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {}

    Vector3 operator-() const {
        return Vector3(-this->x, -this->y, -this->z);
    }

    Vector3 operator+(const Vector3& v) const {
        return Vector3(this->x + v.x, this->y + v.y, this->z + v.z);
    }

    Vector3 operator-(const Vector3& v) const {
        return Vector3(this->x - v.x, this->y - v.y, this->z - v.z);
    }

    Vector3 operator*(const float& s) const {
        return Vector3(this->x * s, this->y * s, this->z * s);
    }

    Vector3 operator/(const float& s) const {
        return Vector3(this->x / s, this->y / s, this->z / s);
    }

    Vector3 operator+=(const Vector3& v) {
        this->x += v.x;
        this->y += v.y;
        this->z += v.z;
        return *this;
    }

    Vector3 operator-=(const Vector3& v) {
        return *this += -v;
    }

    Vector3 operator*=(const float& s) {
        this->x *= s;
        this->y *= s;
        this->z *= s;
        return *this;
    }

    Vector3 operator/=(const float& s) {
        return *this *= 1 / s;
    }
};

inline float dot(const Vector3& a, const Vector3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vector3 cross(const Vector3& a, const Vector3& b) { return Vector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
inline float length(const Vector3& vec3) { return std::sqrt(vec3.x * vec3.x + vec3.y * vec3.y + vec3.z * vec3.z); }
inline Vector3 unit_vector(const Vector3& vec3) { return vec3 / length(vec3); }
inline Vector3 reflect(const Vector3 vec3, const Vector3 normal) { return vec3 - normal * 2 * dot(vec3, normal); }
inline Vector3 lerp(const Vector3& start, const Vector3& end, float t) { return start + (end - start) * t; }
inline float lerp(float start, float end, float t) { return start + (end - start) * t; }
inline float deg2rad(float degrees) { return degrees * PI / 180.0f; }

struct Sphere {
    Vector3 center;
	float radius;

    Sphere(Vector3 cen, float r) : center(cen), radius(r) {}
};

struct Ray {
    Vector3 origin_point;
    Vector3 direction;

    Ray() {}
    Ray(Vector3 origin, Vector3 dir) : origin_point(origin), direction(dir) {}
};

inline Vector3 ray_at(const Ray& ray, float t) { return ray.origin_point + ray.direction * t; }

struct Camera {
    float aspect_ratio;
    float lens_radius;
    Vector3 origin, lower_left_corner, horizontal, vertical;
    Vector3 u, v, w;

    Camera(Vector3 position, Vector3 look_at, Vector3 up, float aspect_r, float vertical_fov, float aperture, float focus_dist) : aspect_ratio(aspect_r) {
        auto theta = deg2rad(vertical_fov);
        auto h = tan(theta / 2);
        auto viewport_height = 2.0f * h;
        auto viewport_width = aspect_ratio * viewport_height;

        w = unit_vector(position - look_at);
        u = unit_vector(cross(up, w));
        v = unit_vector(cross(w, u));

        origin = position;
        horizontal = u * viewport_width * focus_dist;
        vertical = v * viewport_height * focus_dist;
        lower_left_corner = origin - horizontal / 2 - vertical / 2 - w * focus_dist;

        lens_radius = aperture / 2;
    }
};
