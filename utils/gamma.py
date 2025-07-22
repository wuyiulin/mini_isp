import numpy as np

def apply_gamma_correction(img, gamma=2.2, max_val=1023.0):
    """
    對經過白平衡的 linear RGB 圖進行 Gamma 校正，使其接近人眼視覺感受
    參數：
        img: np.ndarray, dtype=float32 or uint16
        gamma: float, gamma 值 (一般為 2.2)
        max_val: 最大原始值（10-bit 是 1023）
    回傳：
        Gamma 校正後的 uint8 圖像 (0~255)
    """
    # 轉為 float32 並正規化到 0~1
    img = img.astype(np.float32) / max_val

    # Clip 避免負值或過曝
    img = np.clip(img, 0, 1)

    # Gamma correction
    img = np.power(img, 1.0 / gamma)

    # 再轉回 0~255 顯示
    img = (img * 255.0).astype(np.uint8)

    return img