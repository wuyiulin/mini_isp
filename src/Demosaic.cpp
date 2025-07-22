#include "Demosaic.hpp"
#include <iostream>

namespace Demosaic
{
    cv::Mat fill(const cv::Mat& channel_input)
    {
        CV_Assert(channel_input.type() == CV_32FC1);

        cv::Mat filled_channel = channel_input.clone();
        int height = channel_input.rows, width = channel_input.cols;
        for(int r=0; r<height; r++)
        {
            for(int c=0; c<width; c++)
            {
                if(filled_channel.at<float>(r, c) == 0.0f)
                {
                    float sum = 0.0f;
                    int count = 0;
                    for(int sr=-1; sr<2; sr++)
                    {
                        for(int sc=-1; sc<2; sc++)
                        {
                            if(!sr && !sc)
                                continue;
                            int nr = r + sr, nc = c + sc;
                            if(0 <= nr && nr < height && 0 <= nc && nc < width)
                            {
                                float val = channel_input.at<float>(nr, nc);
                                if(val != 0.0f)
                                {
                                    sum += val;
                                    count++;
                                }
                            }
                            
                        }
                    }

                    if(count > 0)
                    {
                        filled_channel.at<float>(r, c) = sum / count;
                    }
                }
            }
        }

        return filled_channel;
    }

    cv::Mat NearestNeighborInterpolation(const cv::Mat& input)
    {
        CV_Assert(input.type() == CV_32FC3);

        std::vector<cv::Mat> initial_channels(3);
        cv::split(input, initial_channels); // input 來自 AssignInitialChannels

        std::vector<cv::Mat> demosaiced_channels(3);

        for (int i = 0; i < 3; ++i)
        {
            demosaiced_channels[i] = Demosaic::fill(initial_channels[i]);
        }

        cv::Mat output_bgr;
        cv::merge(demosaiced_channels, output_bgr);
        return output_bgr;
    }
}