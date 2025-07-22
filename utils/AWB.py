import pdb
import time
import cv2
import numpy as np
from matplotlib import pyplot as plt
import matplotlib.image as im
from glob import glob
from tqdm import tqdm
import os
import sys
import pdb


def AutoWhiteBalance_PRA_FLOAT(BGRimg, ratio=0.2):
    # BGRimg : float32 0~1
    BGRimg = BGRimg.astype(np.float32)
    pixelSum = BGRimg[:, :, 0] + BGRimg[:, :, 1] + BGRimg[:, :, 2]
    pexelMax = np.max(BGRimg[:, :, :])    # 0~1 這裡應該就是 1.0

    row, col, chan = BGRimg.shape
    pixelSum = pixelSum.flatten()
    imgFlatten = BGRimg.reshape(row * col, chan)

    thresholdNum = int(ratio * row * col)
    thresholdList = np.argpartition(pixelSum, kth=-thresholdNum, axis=None)
    thresholdList = thresholdList[-thresholdNum:]

    blueMean, greenMean, redMean = 0, 0, 0

    for i in thresholdList:
        blueMean  += imgFlatten[i][0]
        greenMean += imgFlatten[i][1]
        redMean   += imgFlatten[i][2]

    blueMean  /= thresholdNum
    greenMean /= thresholdNum
    redMean   /= thresholdNum

    blueGain  = pexelMax / blueMean
    greenGain = pexelMax / greenMean
    redGain   = pexelMax / redMean

    BGRimg[:, :, 0] *= blueGain
    BGRimg[:, :, 1] *= greenGain
    BGRimg[:, :, 2] *= redGain

    BGRimg = np.clip(BGRimg, 0, 1.0)

    return BGRimg


def AutoWhiteBalance_PRA(BGRimg, ratio=0.2): # 自動白平衡演算法_完美反射核心, 0 < ratio <1.

    BGRimg = BGRimg.astype(np.float32)    # 將資料格式轉為 float32，避免 OpenCV 讀圖進來為 np.uint8 而造成後續截斷問題。
    pixelSum = BGRimg[:,:,0] + BGRimg[:,:,1] + BGRimg[:,:,2]     
    pexelMax = np.max(BGRimg[:,:,:])    # 求三通道加總最大值作為像素值拉伸指標
    
    row , col, chan = BGRimg.shape[0], BGRimg.shape[1], BGRimg.shape[2]
    
    pixelSum = pixelSum.flatten()
    imgFlatten = BGRimg.reshape(row*col, chan)
    
    thresholdNum =  int(ratio * row * col)
    thresholdList = np.argpartition(pixelSum, kth=-thresholdNum, axis=None)    # 由右至左設定閾值位置，並得到粗略排序後的索引表。
    thresholdList = thresholdList[-thresholdNum:]    # 取得索引表中比閾值大的索引號
    

    blueMean, greenMean, redMean, i = 0, 0, 0, 0
    
    for i in thresholdList:
        blueMean  += imgFlatten[i][0]
        greenMean += imgFlatten[i][1]
        redMean   += imgFlatten[i][2]
    
    blueMean  /= thresholdNum
    greenMean /= thresholdNum
    redMean   /= thresholdNum

    blueGain  = pexelMax / blueMean
    greenGain = pexelMax / greenMean
    redGain   = pexelMax / redMean
    
    BGRimg[:,:,0] = BGRimg[:,:,0] * blueGain
    BGRimg[:,:,1] = BGRimg[:,:,1] * greenGain
    BGRimg[:,:,2] = BGRimg[:,:,2] * redGain

    BGRimg = np.clip(BGRimg, 0, 255)
    BGRimg = BGRimg.astype(np.uint8)

    return BGRimg





def AutoWhiteBalance_greyWorld(imgPath): # 自動白平衡演算法_灰色世界核心
    
    BGRimg = cv2.imread(imgPath)
    Mean_blue, Mean_green, Mean_red = np.mean(BGRimg[:,:,0]), np.mean(BGRimg[:,:,1]), np.mean(BGRimg[:,:,2])
    Mean_grey = (Mean_blue + Mean_green + Mean_red) / 3

    gainMat =  Mean_grey / np.array([Mean_blue, Mean_green, Mean_red])
    BGRimg = np.multiply(BGRimg, gainMat)
    BGRimg = np.clip(BGRimg, 0, 255)
    BGRimg = BGRimg.astype(np.uint8)

    return BGRimg



if __name__=='__main__':
    # imgPath = "/Data/Project/feature_optimal/Our_Dataset/破碎化資料集/日本四車/202503141126.jpg"
    
    # img = cv2.imread(imgPath)
    # # imgG = AutoWhiteBalance_greyWorld(imgPath)
    # imgP = AutoWhiteBalance_PRA(imgPath, 0.05)

    # # concatenated_img = cv2.hconcat([img, imgG, imgP])
    # concatenated_img = cv2.hconcat([img, imgP])
    # # print((imgG == imgP).all())
    # cv2.namedWindow("AWB", 0)
    # cv2.resizeWindow("AWB", int(img.shape[1]/2), int(img.shape[0]/2))
    # cv2.imshow('AWB', concatenated_img)

    # while True:
    #     if (cv2.getWindowProperty('AWB', cv2.WND_PROP_VISIBLE) <= 0 or cv2.waitKey(1) > 0):
    #         cv2.destroyWindow('AWB')
    #         break

    ## 搜尋一個資料夾裡面所有圖片並計算白平衡後存起來

    input_folder = "/Data/Project/feature_optimal/Our_Dataset/破碎化資料集/日本四車_resize_DHL"
    output_folder = input_folder + "_AWB"

    # 確保輸出資料夾存在
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)

    # 讀取所有 .jpg 和 .png 檔案
    image_paths = sorted(glob(os.path.join(input_folder, "*.jpg")) + glob(os.path.join(input_folder, "*.png")))

    if not image_paths:
        print("資料夾內沒有找到圖片")
        exit()

    # 對所有圖片應用相同的透視變換
    for img_path in image_paths:
        img = AutoWhiteBalance_PRA(img_path, 0.05)
        output_filename = os.path.join(output_folder, os.path.basename(img_path))
        cv2.imwrite(output_filename, img)
        print(f"已儲存: {output_filename}")

    print("所有圖片已處理完成！")



