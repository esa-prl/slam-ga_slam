#include "ga_slam/DataRegistration.hpp"

#include "ga_slam/CloudProcessing.hpp"

namespace ga_slam {

DataRegistration::DataRegistration(void)
        : map_() {
    processedCloud_.reset(new Cloud);
}

void DataRegistration::setParameters(
        double mapLengthX, double mapLengthY, double mapResolution,
        double minElevation, double maxElevation, double voxelSize) {
    map_.setMapParameters(mapLengthX, mapLengthY, mapResolution,
            minElevation, maxElevation);

    voxelSize_ = voxelSize;
}

void DataRegistration::registerData(
        const Cloud::ConstPtr& cloud,
        const Pose& sensorToMapTF,
        const Pose& estimatedPose) {
    CloudProcessing::processCloud(cloud, processedCloud_, cloudVariances_,
            sensorToMapTF, map_, voxelSize_);

    map_.translate(estimatedPose.translation());

    updateMap();
}

void DataRegistration::updateMap(void) {
    auto& meanData = map_.getMeanZ();
    auto& varianceData = map_.getVarianceZ();

    size_t cloudIndex = 0;
    size_t mapIndex;

    for (const auto& point : processedCloud_->points) {
        cloudIndex++;

        if (!map_.getIndexFromPosition(point.x, point.y, mapIndex)) continue;

        float& mean = meanData(mapIndex);
        float& variance = varianceData(mapIndex);
        const float& pointVariance = cloudVariances_[cloudIndex - 1];

        if (!std::isfinite(mean)) {
            mean = point.z;
            variance = pointVariance;
        } else {
            fuseGaussians(mean, variance, point.z, pointVariance);
        }
    }

    map_.setTimestamp(processedCloud_->header.stamp);
}

void DataRegistration::fuseGaussians(
        float& mean1, float& variance1,
        const float& mean2, const float& variance2) {
    const double innovation = mean2 - mean1;
    const double gain = variance1 / (variance1 + variance2);

    mean1 = mean1 + (gain * innovation);
    variance1 = variance1 * (1. - gain);
}

}  // namespace ga_slam
