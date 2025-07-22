#pragma once

#include <string>
#include <opencv2/opencv.hpp>
#include <libraw/libraw.h>

namespace Utils
{
    // 建立顏色遮罩
    std::string GetBayerPattern(LibRaw& processor);
    void GenerateBayerMasks(int height, int width, cv::Mat& maskR, cv::Mat& maskG, cv::Mat& maskB, std::string& pattern);

    // 利用遮罩分離通道顏色
    cv::Mat AssignInitialChannels(const cv::Mat& gray, const cv::Mat& maskR, const cv::Mat& maskG, const cv::Mat& maskB);

    // 解馬賽克
    cv::Mat NearestNeighborInterpolation(const cv::Mat& input);

    // 白平衡
    cv::Mat ApplyWhiteBalance(const cv::Mat& input_bgr, const float cam_mul[4]); // 傳遞 cam_mul 陣列

    // 套用 CCM
    cv::Mat ApplyCCM(const cv::Mat& raw_img, const cv::Mat& ccm);

    // Gamma 校正
    cv::Mat ApplyGammaCorrection(const cv::Mat& input_bgr, double gamma);
}
