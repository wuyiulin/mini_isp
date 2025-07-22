#include "Utils.hpp"
#include <fstream>
#include <iostream>
#include <libraw/libraw.h>

namespace ImageIO
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
        
        // 從 LibRaw 獲取原始 RAW 數據的最大可能值 (白點)
        // 根據你查找的頭文件，嘗試 processor.imgdata.color.data_maximum
        // ushort max_val = processor.imgdata.color.data_maximum; 
        ushort max_val = processor.imgdata.color.maximum; 
        
        // 如果 data_maximum 為 0，或者異常，提供一個備用值
        // DNG 可能是 12-bit, 14-bit, 16-bit
        // 65535 是 16-bit 的最大值，作為一個相對安全的通用備用值
        if (max_val == 0) {
            std::cerr << "Warning: LibRaw data_maximum is 0, using 65535 as fallback for normalization." << std::endl;
            max_val = 65535; 
        }

        cv::Mat raw_image(height, width, CV_32FC1); // 創建浮點單通道 Mat

        // 遍歷 RAW 數據，並進行正規化
        for (int i = 0; i < height; ++i)
        {
            for (int j = 0; j < width; ++j)
            {
                // 從 LibRaw 獲取原始像素值
                ushort pixel_value = processor.imgdata.rawdata.raw_image[i * width + j];
                
                // 將 ushort 值轉換為 float，並除以 max_val 進行正規化
                raw_image.at<float>(i, j) = static_cast<float>(pixel_value) / max_val;
            }
        }
        return raw_image;
    }

    void ShowImage(const std::string& winName, const cv::Mat& img)
    {
        cv::Mat disp_8u; // 最終用於顯示的 8 位元圖像

        if (img.empty()) {
            std::cerr << "Warning: Attempted to show empty image: " << winName << std::endl;
            return;
        }

        // --- 核心邏輯：將所有輸入圖像轉換為 8 位元，無論其原始類型或通道數 ---
        // 這樣可以保證 imshow 總是接收到它期望的 8 位元圖像
        
        if (img.depth() == CV_32F || img.depth() == CV_64F)
        {
            // 對於浮點圖像（單通道或多通道），先正規化到 0-1 範圍，再轉換為 0-255
            cv::Mat normalized_float_img;
            cv::normalize(img, normalized_float_img, 0.0, 1.0, cv::NORM_MINMAX, img.type()); // 注意：normalize 後保持原類型
            normalized_float_img.convertTo(disp_8u, CV_8U, 255.0);
        }
        else if (img.depth() == CV_16U) // 如果是 16 位元無符號整數（例如 DNG 原始數據），也要正規化
        {
            cv::Mat normalized_16u_img;
            cv::normalize(img, normalized_16u_img, 0, 255, cv::NORM_MINMAX, CV_8U);
            disp_8u = normalized_16u_img;
        }
        else if (img.depth() == CV_8U) // 如果已經是 8 位元無符號整數，直接複製
        {
            disp_8u = img.clone();
        }
        else
        {
            std::cerr << "Error: Utils::ShowImage - Unsupported image depth: " << img.depth() << " for window " << winName << std::endl;
            return;
        }
        // --- 核心邏輯結束 ---

        // 設定視窗為可調整大小
        cv::namedWindow(winName, cv::WINDOW_NORMAL); 
        
        // 顯示圖片
        cv::imshow(winName, disp_8u); // 顯示轉換後的 8 位元圖像
        
        // 等待用戶按鍵
        cv::waitKey(0);
        
        // 關閉視窗
        cv::destroyWindow(winName); 
    }

    void SaveImage(const std::string& filepath, const cv::Mat& img)
    {
        if (img.empty()) {
            std::cerr << "Error: Attempted to save empty image to: " << filepath << std::endl;
            return;
        }

        // --- 移除重複的轉換邏輯 ---
        // 傳入 img 應該已經是 CV_8U 類型 (0-255 範圍)
        // 直接使用 img 進行保存
        bool success = cv::imwrite(filepath, img); 
        // ---------------------------
        
        if (!success) {
            std::cerr << "Error: Could not save image to " << filepath << std::endl;
        }
    }

}
