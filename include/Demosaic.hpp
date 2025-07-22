#pragma once

#include <string>
#include <opencv2/opencv.hpp>
#include <libraw/libraw.h>

namespace Demosaic
{
    cv::Mat NearestNeighborInterpolation(const cv::Mat& input);
}
