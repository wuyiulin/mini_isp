#pragma once

#include <string>
#include <opencv2/opencv.hpp>
#include <libraw/libraw.h>

namespace ImageIO
{
    cv::Mat ReadRaw16(const std::string& filepath, int width, int height);

    cv::Mat ReadDNG(LibRaw& processor);

    void ShowImage(const std::string& winName, const cv::Mat& img);

    void SaveImage(const std::string& filepath, const cv::Mat& img);
}
