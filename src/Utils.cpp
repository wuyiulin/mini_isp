#include "Utils.hpp"
#include <fstream>
#include <iostream>
#include <libraw/libraw.h>

namespace Utils
{

    cv::Mat ReadRaw16(const std::string& filepath, int width, int height)
    {
        std::ifstream file(filepath, std::ios::binary);
        if (!file)
        {
            std::cerr << "Failed to open raw file: " << filepath << std::endl;
            return cv::Mat();
        }

        std::vector<uint16_t> buffer(width * height);
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size() * sizeof(uint16_t));

        cv::Mat rawImg(height, width, CV_32FC1);
        for (int i = 0; i < width * height; ++i)
        {
            rawImg.at<float>(i / width, i % width) = static_cast<float>(buffer[i]) / 1023.0f;
        }
        return rawImg;
    }

    cv::Mat ReadDNG(LibRaw& processor)
    {
        int width = processor.imgdata.sizes.raw_width;
        int height = processor.imgdata.sizes.raw_height;
        
        ushort max_val = processor.imgdata.color.maximum; 
        
        if (max_val == 0) 
        {
            std::cerr << "Warning: LibRaw data_maximum is 0, using 65535 as fallback for normalization." << std::endl;
            max_val = 65535; 
        }

        cv::Mat raw_image(height, width, CV_32FC1);

        for (int i = 0; i < height; ++i)
        {
            for (int j = 0; j < width; ++j)
            {
                ushort pixel_value = processor.imgdata.rawdata.raw_image[i * width + j];
                raw_image.at<float>(i, j) = static_cast<float>(pixel_value) / max_val;
            }
        }
        return raw_image;
    }

    std::string GetBayerPattern(LibRaw& processor)
    {
        char* desc = processor.imgdata.idata.cdesc;
        std::string pattern(desc, 4); // RGBG / RGBE / GMCY / GRTG
        if (pattern == "RGBG" || pattern == "RGBE" || pattern == "GMCY" || pattern == "GRTG")
        {
            if(pattern == "RGBG")
                pattern = "RGGB";
            return pattern;
        }
            
        return "UNKNOWN";
    }

    cv::Mat AssignInitialChannels(const cv::Mat& gray, const cv::Mat& maskR, const cv::Mat& maskG, const cv::Mat& maskB)
    {
        CV_Assert(gray.type() == CV_32FC1);
        CV_Assert(maskR.type() == CV_32F);
        CV_Assert(maskG.type() == CV_32F);
        CV_Assert(maskB.type() == CV_32F);


        std::vector<cv::Mat> channels(3);
        channels[0] = gray.mul(maskB / 1.0f);
        channels[1] = gray.mul(maskG / 1.0f);
        channels[2] = gray.mul(maskR / 1.0f);

        cv::Mat bgr;
        cv::merge(channels, bgr);
        return bgr; // CV_32FC3
    }

    void GenerateBayerMasks(int height, int width, cv::Mat& maskR, cv::Mat& maskG, cv::Mat& maskB, std::string& pattern)
    {
        maskR = cv::Mat::zeros(height, width, CV_32F);
        maskG = cv::Mat::zeros(height, width, CV_32F);
        maskB = cv::Mat::zeros(height, width, CV_32F);
        

        std::cout<<"pattern: "<<pattern<<"\n";

        for (int i = 0; i < height; ++i)
        {
            for (int j = 0; j < width; ++j)
            {
                if (pattern == "RGGB")
                {
                    if (i % 2 == 0 && j % 2 == 0)      maskR.at<float>(i, j) = 1.0f;
                    else if (i % 2 == 0 && j % 2 == 1) maskG.at<float>(i, j) = 1.0f;
                    else if (i % 2 == 1 && j % 2 == 0) maskG.at<float>(i, j) = 1.0f;
                    else                               maskB.at<float>(i, j) = 1.0f;
                }
                else if (pattern == "BGGR")
                {
                    if (i % 2 == 0 && j % 2 == 0)      maskB.at<float>(i, j) = 1.0f;
                    else if (i % 2 == 0 && j % 2 == 1) maskG.at<float>(i, j) = 1.0f;
                    else if (i % 2 == 1 && j % 2 == 0) maskG.at<float>(i, j) = 1.0f;
                    else                               maskR.at<float>(i, j) = 1.0f;
                }
                else if (pattern == "GRBG")
                {
                    if (i % 2 == 0 && j % 2 == 0)      maskG.at<float>(i, j) = 1.0f;
                    else if (i % 2 == 0 && j % 2 == 1) maskR.at<float>(i, j) = 1.0f;
                    else if (i % 2 == 1 && j % 2 == 0) maskB.at<float>(i, j) = 1.0f;
                    else                               maskG.at<float>(i, j) = 1.0f;
                }
                else if (pattern == "GBRG")
                {
                    if (i % 2 == 0 && j % 2 == 0)      maskG.at<float>(i, j) = 1.0f;
                    else if (i % 2 == 0 && j % 2 == 1) maskB.at<float>(i, j) = 1.0f;
                    else if (i % 2 == 1 && j % 2 == 0) maskR.at<float>(i, j) = 1.0f;
                    else                               maskG.at<float>(i, j) = 1.0f;
                }
                else
                {
                    std::cerr << "[Error] Unknown Bayer Pattern: " << pattern << std::endl;
                    return;
                }
            }
        }
    }


    cv::Mat ApplyCCM(const cv::Mat& raw_img, const cv::Mat& ccm)
    {
        CV_Assert(raw_img.type() == CV_32FC3);
        CV_Assert(ccm.rows == 3 && ccm.cols == 3);

        cv::Mat corrected(raw_img.size(), CV_32FC3);

        for (int i = 0; i < raw_img.rows; ++i)
        {
            for (int j = 0; j < raw_img.cols; ++j)
            {
                // pixel[0] = B (Blue)
                // pixel[1] = G (Green)
                // pixel[2] = R (Red)
                cv::Vec3f pixel = raw_img.at<cv::Vec3f>(i, j);

                // Trans BGR to RGB
                float original_R = pixel[2];
                float original_G = pixel[1];
                float original_B = pixel[0];

                // CCM Matrix：
                // [ RR RG RB ]   [ R_in ]   [ R_out ]
                // [ GR GG GB ] * [ G_in ] = [ G_out ]
                // [ BR BG BB ]   [ B_in ]   [ B_out ]
                float R_out = ccm.at<float>(0, 0) * original_R + ccm.at<float>(0, 1) * original_G + ccm.at<float>(0, 2) * original_B;
                float G_out = ccm.at<float>(1, 0) * original_R + ccm.at<float>(1, 1) * original_G + ccm.at<float>(1, 2) * original_B;
                float B_out = ccm.at<float>(2, 0) * original_R + ccm.at<float>(2, 1) * original_G + ccm.at<float>(2, 2) * original_B;

                cv::Vec3f corrected_pixel;
                // Trans RGB to RBGR
                corrected_pixel[0] = B_out;
                corrected_pixel[1] = G_out;
                corrected_pixel[2] = R_out;

                for (int k = 0; k < 3; ++k)
                    corrected_pixel[k] = std::min(std::max(corrected_pixel[k], 0.0f), 1.0f);

                corrected.at<cv::Vec3f>(i, j) = corrected_pixel;
            }
        }
        return corrected;
    }


    cv::Mat ApplyWhiteBalance(const cv::Mat& input_bgr, const float cam_mul[4])
    {
        if (input_bgr.channels() != 3 || input_bgr.depth() != CV_32F)
        {
            std::cerr << "Error: ApplyWhiteBalance expects 3-channel CV_32F input." << std::endl;
            return input_bgr.clone();
        }

        cv::Mat output_bgr = input_bgr.clone();
        float r_mul = cam_mul[0];
        float g_mul = cam_mul[1];
        float b_mul = cam_mul[3];

        if (r_mul == 0) r_mul = 1.0f;
        if (g_mul == 0) g_mul = 1.0f;
        if (b_mul == 0) b_mul = 1.0f;
        
        for (int i = 0; i < output_bgr.rows; ++i)
        {
            for (int j = 0; j < output_bgr.cols; ++j)
            {
                cv::Vec3f& pixel = output_bgr.at<cv::Vec3f>(i, j);
                pixel[0] *= b_mul;
                pixel[1] *= g_mul;
                pixel[2] *= r_mul;
            }
        }

        cv::min(output_bgr, 1.0f, output_bgr);
        cv::max(output_bgr, 0.0f, output_bgr);
        
        return output_bgr;
    }


    cv::Mat ApplyGammaCorrection(const cv::Mat& input_bgr, double gamma)
    {
        if (input_bgr.channels() != 3 || input_bgr.depth() != CV_32F)
        {
            std::cerr << "Error: ApplyGammaCorrection expects 3-channel CV_32F input." << std::endl;
            return input_bgr.clone();
        }

        cv::Mat output_bgr;

        cv::pow(input_bgr, 1.0 / gamma, output_bgr);

        cv::min(output_bgr, 1.0f, output_bgr);
        cv::max(output_bgr, 0.0f, output_bgr);
        
        return output_bgr;
    }


    void ShowImage(const std::string& winName, const cv::Mat& img)
    {
        cv::Mat disp_8u;
        if (img.empty()) 
        {
            std::cerr << "Warning: Attempted to show empty image: " << winName << std::endl;
            return;
        }
        
        if (img.depth() == CV_32F || img.depth() == CV_64F)
        {
            cv::Mat normalized_float_img;
            cv::normalize(img, normalized_float_img, 0.0, 1.0, cv::NORM_MINMAX, img.type());
            normalized_float_img.convertTo(disp_8u, CV_8U, 255.0);
        }
        else if (img.depth() == CV_16U)
        {
            cv::Mat normalized_16u_img;
            cv::normalize(img, normalized_16u_img, 0, 255, cv::NORM_MINMAX, CV_8U);
            disp_8u = normalized_16u_img;
        }
        else if (img.depth() == CV_8U)
        {
            disp_8u = img.clone();
        }
        else
        {
            std::cerr << "Error: Utils::ShowImage - Unsupported image depth: " << img.depth() << " for window " << winName << std::endl;
            return;
        }

        cv::namedWindow(winName, cv::WINDOW_NORMAL); 
        
        cv::imshow(winName, disp_8u);
        
        cv::waitKey(0);
        
        cv::destroyWindow(winName); 
    }

    void SaveImage(const std::string& filepath, const cv::Mat& img)
    {
        if (img.empty()) 
        {
            std::cerr << "Error: Attempted to save empty image to: " << filepath << std::endl;
            return;
        }

        bool success = cv::imwrite(filepath, img); 
        // ---------------------------
        
        if (!success) 
        {
            std::cerr << "Error: Could not save image to " << filepath << std::endl;
        }
    }
}
