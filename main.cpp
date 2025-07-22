#include "Utils.hpp"
#include "ImageIO.hpp"
#include "Demosaic.hpp"
#include <iostream>
#include <libraw/libraw.h>

int main(int argc, char** argv)
{

    // Setup Config
    // ===============================================================
    std::string input_path;
    std::string output_path;
    std::string pattern;
    double gamma_value;
    bool simulatedMode;

    LibRaw processor; // LibRaw 處理器實例

    cv::Mat raw;
    cv::Mat ccm_mat(3, 3, CV_32F);
    int width = 0, height = 0;
    float cam_mul_coeffs[4] = {1.0f, 1.0f, 1.0f, 1.0f}; 
    
    // ===============================================================

    // CLI Parameter
    // ===============================================================
        if (argc < 2)
        {
            std::cerr << "Usage: ./mini_isp <input.dng> [output.png] [bayer.pattern] [gamma.value] [simulatedMode(assume 0 == False)]" << std::endl;
            return -1;
        }
        input_path = argv[1];
        output_path = (argc >= 3) ? argv[2] : "../data/output.png";
        pattern = (argc >= 4) ? argv[3] : "";
        gamma_value = (argc >= 5) ? (std::stod(argv[4])) : 2.2;
        simulatedMode = (argc >= 6) ? (std::atoi(argv[5]) != 0)  : false;
        
        if (processor.open_file(input_path.c_str()) != LIBRAW_SUCCESS)
        {
            std::cerr << "Failed to open DNG file: " << input_path << std::endl;
            return -1;
        }
        if (processor.unpack() != LIBRAW_SUCCESS)
        {
            std::cerr << "Failed to unpack DNG file: " << input_path << std::endl;
            return -1;
        }
    // ===============================================================
    if (simulatedMode)
    {
        width = 4;
        height = 4;
        
        raw = cv::Mat(height, width, CV_32FC1);
        for (int i = 0; i < height; ++i)
        {
            for (int j = 0; j < width; ++j)
            {
                if (i % 2 == 0 && j % 2 == 0)      raw.at<float>(i, j) = 0.8f; // R
                else if (i % 2 == 0 && j % 2 == 1) raw.at<float>(i, j) = 0.5f; // G
                else if (i % 2 == 1 && j % 2 == 0) raw.at<float>(i, j) = 0.5f; // G
                else                               raw.at<float>(i, j) = 0.2f; // B
            }
        }
        ImageIO::ShowImage("Raw Simulated Data", raw);
        ccm_mat = cv::Mat::eye(3, 3, CV_32F); 
        cam_mul_coeffs[0] = 1.0f;
        cam_mul_coeffs[1] = 1.0f;
        cam_mul_coeffs[2] = 1.0f;
        cam_mul_coeffs[3] = 1.0f;
    }
    else
    {


        raw = ImageIO::ReadDNG(processor);
        
        auto ccm = processor.imgdata.color.rgb_cam;
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                ccm_mat.at<float>(i, j) = ccm[i][j];
            }
        }
        
        width = processor.imgdata.sizes.raw_width;
        height = processor.imgdata.sizes.raw_height;

        if (pattern.empty())
            pattern = Utils::GetBayerPattern(processor);
        else if(pattern == "UNKNOWN")
        {
            std::cerr << "Error: Failed to determine Bayer Pattern. Exiting." << std::endl;
            return -1;
        }
        else if(pattern != "RGGB")
        {
            std::cerr << "Error: " << pattern << " pattern Unsupported now." << std::endl;
            return -1;
        }

        for (int i = 0; i < 4; ++i) 
        {
            cam_mul_coeffs[i] = processor.imgdata.color.cam_mul[i];
            if (cam_mul_coeffs[i] <= 0) cam_mul_coeffs[i] = 1.0f; 
        }
        gamma_value = 2.2; 
    }
    
    if (raw.empty())
    {
        std::cerr << "Error: Raw image data is empty." << std::endl;
        return -1;
    }

    std::cout << "--- Processing Image ---" << std::endl;
    std::cout << (simulatedMode ? "Mode: Simulated Data Test" : "Mode: Real DNG File Processing") << std::endl;
    std::cout << "Image Width: " << width << ", Height: " << height << std::endl;
    std::cout << "Bayer Pattern used: " << pattern << std::endl;

    double raw_min, raw_max;
    cv::minMaxLoc(raw, &raw_min, &raw_max);
    std::cout << "Raw image min: " << raw_min << ", max: " << raw_max << std::endl;
    

    cv::Mat maskR, maskG, maskB;
    Utils::GenerateBayerMasks(height, width, maskR, maskG, maskB, pattern);

    // Verify Mask 
    // ===============================================================
    // ImageIO::ShowImage("Mask R", maskR);
    // ImageIO::ShowImage("Mask G", maskG);
    // ImageIO::ShowImage("Mask B", maskB);
    // ===============================================================
    double r_min, r_max, g_min, g_max, b_min, b_max;
    cv::minMaxLoc(maskR, &r_min, &r_max);
    cv::minMaxLoc(maskG, &g_min, &g_max);
    cv::minMaxLoc(maskB, &b_min, &b_max);
    std::cout << "Mask R min/max: " << r_min << "/" << r_max << std::endl;
    std::cout << "Mask G min/max: " << g_min << "/" << g_max << std::endl;
    std::cout << "Mask B min/max: " << b_min << "/" << b_max << std::endl;
    
    auto init_bgr = Utils::AssignInitialChannels(raw, maskR, maskG, maskB);
    cv::Mat init_bgr_display;
    init_bgr.convertTo(init_bgr_display, CV_8UC3, 255.0);
    ImageIO::ShowImage("Initial BGR Channels", init_bgr_display);

    
    double minVal, maxVal;
    cv::minMaxLoc(init_bgr, &minVal, &maxVal);
    std::cout << "Initial BGR min: " << minVal << ", max: " << maxVal << std::endl;

    auto demosaiced = Demosaic::NearestNeighborInterpolation(init_bgr);
    
    cv::minMaxLoc(demosaiced, &minVal, &maxVal);
    std::cout << "Demosaiced min: " << minVal << ", max: " << maxVal << std::endl;
    
    auto white_balanced = Utils::ApplyWhiteBalance(demosaiced, cam_mul_coeffs);
    cv::minMaxLoc(white_balanced, &minVal, &maxVal);
    std::cout << "White-Balanced min: " << minVal << ", max: " << maxVal << std::endl;

    auto color_corrected = Utils::ApplyCCM(white_balanced, ccm_mat);
    

    auto gamma_corrected = Utils::ApplyGammaCorrection(color_corrected, gamma_value);
    cv::minMaxLoc(gamma_corrected, &minVal, &maxVal);
    std::cout << "Gamma-Corrected min: " << minVal << ", max: " << maxVal << std::endl;

    cv::Mat image_display;
    gamma_corrected.convertTo(image_display, CV_8UC3, 255.0);
    
    ImageIO::ShowImage("image_display", image_display); 
    
    if (!output_path.empty()) 
    {
        ImageIO::SaveImage(output_path, image_display);
        std::cout << "Saved output image to: " << output_path << std::endl;
    } 
    else 
    {
        std::cout << "No output path specified. Image not saved." << std::endl;
    }

    std::cout << "--- Processing Finished ---" << std::endl;
    return 0;
}