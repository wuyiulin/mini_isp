import rawpy
import imageio.v2 as imageio
import numpy as np
import cv2
from AWB import AutoWhiteBalance_PRA_FLOAT as AWBP
from gamma import apply_gamma_correction as AGC
import sys
import pdb


def show_image(img, img_name):
    cv2.namedWindow(img_name, 0)
    cv2.imshow(img_name, img)
    while True:
        if (cv2.getWindowProperty(img_name, cv2.WND_PROP_VISIBLE) <= 0 or cv2.waitKey(1) > 0):
            cv2.destroyWindow(img_name)
            break


def generate_bayer_masks(height, width, pattern='RGGB'):
    R = np.zeros((height, width), dtype=np.uint8)
    G = np.zeros((height, width), dtype=np.uint8)
    B = np.zeros((height, width), dtype=np.uint8)

    if pattern == 'RGGB':
        R[0::2, 0::2] = 1
        G[0::2, 1::2] = 1
        G[1::2, 0::2] = 1
        B[1::2, 1::2] = 1
    elif pattern == 'BGGR':
        B[0::2, 0::2] = 1
        G[0::2, 1::2] = 1
        G[1::2, 0::2] = 1
        R[1::2, 1::2] = 1

    return R, G, B


def assign_initial_channels(gray, B_mask, G_mask, R_mask):
    H, W = gray.shape
    bgr = np.zeros((H, W, 3), dtype=np.float32)
    bgr[:, :, 0] = gray * B_mask
    bgr[:, :, 1] = gray * G_mask
    bgr[:, :, 2] = gray * R_mask
    return bgr


def nearest_neighbor_interpolation(rgb):
    for c in range(3):
        mask = (rgb[:, :, c] == 0).astype(np.uint8)
        rgb[:, :, c] = cv2.inpaint(rgb[:, :, c], mask, 1, cv2.INPAINT_NS)
    return rgb


def apply_color_correction(img, CCM):
    H, W, _ = img.shape
    img_flat = img.reshape(-1, 3).astype(np.float32)
    corrected = np.dot(img_flat, CCM[:, :3].T)
    corrected = np.clip(corrected, 0.0, 1.0)
    return corrected.reshape(H, W, 3)


def read_raw16(raw_path, width, height):
    data = np.fromfile(raw_path, dtype=np.uint16).reshape(height, width)
    return data.astype(np.float32) / 1023.0


def extract_raw_and_ccm_from_dng(dng_path, raw_output_path='raw16.raw', ccm_output_path='ccm.txt'):
    with rawpy.imread(dng_path) as raw:
        raw_bayer = raw.raw_image_visible.astype(np.uint16)
        raw_bayer.tofile(raw_output_path)
        print(f"[Saved] Raw saved to {raw_output_path}, shape={raw_bayer.shape}")

        ccm = raw.color_matrix.copy()
        np.savetxt(ccm_output_path, ccm, fmt='%.6f')
        print(f"[Saved] CCM saved to {ccm_output_path}")


if __name__ == '__main__':
    dng_path = "../data/input.dng"
    raw_path = "../data/input.raw"
    ccm_path = "../data/ccm.txt"
    width, height = 640, 480

    extract_raw_and_ccm_from_dng(dng_path, raw_path, ccm_path)
    gray = read_raw16(raw_path, width, height)
    mask_R, mask_G, mask_B = generate_bayer_masks(height, width, pattern='RGGB')
    init_image = assign_initial_channels(gray, mask_R, mask_G, mask_B)
    demosaiced = nearest_neighbor_interpolation(init_image)
    show_image((demosaiced * 255).astype(np.uint8), "Demosaic")
    pdb.set_trace()
    # 到解馬賽克都是好的 
    awb = AWBP(demosaiced)
    show_image((awb * 255).astype(np.uint8), "AWB")

    CCM = np.loadtxt(ccm_path)
    corrected = apply_color_correction(awb, CCM)
    show_image((corrected * 255).astype(np.uint8), "CCM")

    gamma = AGC((corrected * 255).astype(np.uint8))
    show_image(gamma, "Gamma Corrected")