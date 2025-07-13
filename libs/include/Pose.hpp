#include <Eigen/Core>
#include <Eigen/Geometry>

class Pose {
public:
    Eigen::Vector3f position; // Position in 3D space
    Eigen::Quaternionf orientation; // Orientation as a quaternion
    Pose() : position(Eigen::Vector3f::Zero()), orientation(Eigen::Quaternionf::Identity()) {}
    Pose(const Eigen::Vector3f& pos, const Eigen::Quaternionf& orient)
        : position(pos), orientation(orient) {}
        
    // Convert to Eigen's Isometry3f for transformations
    Eigen::Isometry3f toIsometry() const {
        Eigen::Isometry3f iso = Eigen::Isometry3f::Identity();
        iso.translate(position);
        iso.rotate(orientation);
        return iso;
    }
    // Convert from Eigen's Isometry3f
    static Pose fromIsometry(const Eigen::Isometry3f& iso) {
        return Pose(iso.translation(), Eigen::Quaternionf(iso.rotation()));
    }
};