// Simplified 4-DOF alignment solver for the Monado/WiVRn port.
// Core idea from original QuestLHSync:
//   p_headset = R_yaw * p_lighthouse_ref + t
// Gravity-aligned spaces → only yaw + 3 translation DOF.
//
// This is a stripped-down version focused on the geometric fit.
// The original has extensive timing, outlier rejection, layout checks, etc.

#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <iostream>

struct Vec3 {
    double x = 0, y = 0, z = 0;
    Vec3() = default;
    Vec3(double a, double b, double c) : x(a), y(b), z(c) {}
    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
    double dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    double norm() const { return std::sqrt(dot(*this)); }
};

struct Observation {
    Vec3 ray_origin;   // in headset frame at exposure time
    Vec3 ray_dir;      // unit
    Vec3 station_pos;  // in lighthouse reference frame
    double weight = 1.0;
};

struct Alignment {
    double yaw = 0;          // radians
    Vec3 translation{0, 0, 0};
    double residual = 1e9;
    int inliers = 0;
    bool valid = false;
};

class SimpleSolver {
public:
    // Fit yaw + translation so that transformed station positions
    // best match the rays observed from the headset.
    Alignment Fit(const std::vector<Observation>& obs, Alignment seed = {}) {
        if (obs.size() < 6) return {};

        Alignment best = seed;
        best.residual = 1e9;

        // Coarse yaw search + translation least-squares
        for (int i = 0; i < 72; ++i) {  // 5 degree steps
            double yaw = seed.yaw + (i - 36) * (M_PI / 36.0);
            auto a = OptimizeYaw(obs, yaw);
            if (a.residual < best.residual) best = a;
        }

        // Local refinement
        best = OptimizeYaw(obs, best.yaw);
        best.valid = best.residual < 0.05 && best.inliers >= 8;
        return best;
    }

private:
    Alignment OptimizeYaw(const std::vector<Observation>& obs, double yaw) {
        Alignment a;
        a.yaw = yaw;

        // For fixed yaw, solve for translation that minimizes
        // distance from ray to transformed station point.
        // This is a simple least-squares on the closest points.

        Vec3 sum_origin{}, sum_station{};
        int n = 0;
        for (const auto& o : obs) {
            Vec3 st_rot = RotateYaw(o.station_pos, yaw);
            // Approximate: pull station toward ray
            Vec3 closest = o.ray_origin + o.ray_dir * o.ray_dir.dot(st_rot - o.ray_origin);
            sum_origin = sum_origin + closest;
            sum_station = sum_station + st_rot;
            n++;
        }
        if (n < 3) return a;

        a.translation = (sum_origin - sum_station) * (1.0 / n);

        // Compute residual
        double sum_err = 0;
        int inl = 0;
        for (const auto& o : obs) {
            Vec3 st = RotateYaw(o.station_pos, yaw) + a.translation;
            Vec3 to_st = st - o.ray_origin;
            double along = o.ray_dir.dot(to_st);
            Vec3 closest = o.ray_origin + o.ray_dir * along;
            double dist = (st - closest).norm();
            sum_err += dist * dist;
            if (dist < 0.05) inl++;
        }
        a.residual = std::sqrt(sum_err / n);
        a.inliers = inl;
        return a;
    }

    static Vec3 RotateYaw(const Vec3& p, double yaw) {
        double c = std::cos(yaw), s = std::sin(yaw);
        return {c * p.x - s * p.z, p.y, s * p.x + c * p.z};
    }
};
