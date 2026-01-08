import cv2
import numpy as np

def make_map_pretty(input_file, output_file, scale_factor=10):
    # 1. 이미지 로드 (흑백 모드)
    img = cv2.imread(input_file, cv2.IMREAD_GRAYSCALE)
    if img is None:
        print("오류: 파일을 찾을 수 없습니다. 파일명을 확인해주세요.")
        return

    print(f"원본 크기: {img.shape}")

    # 2. 이미지 전처리 (회색조 제거 -> 흑/백으로 명확히 구분)
    # SLAM 맵에서 벽은 보통 어두운 색, 빈 공간은 밝은 색, 미탐사 영역은 회색(205)입니다.
    # 210보다 어두우면 벽(검정)으로 간주하고 나머지는 모두 흰색으로 날립니다.
    _, binary_map = cv2.threshold(img, 210, 255, cv2.THRESH_BINARY)

    # 3. 해상도 확대 (Upscaling)
    # 픽셀이 깨지는 것을 막고 부드러운 처리를 위해 이미지를 키웁니다.
    h, w = binary_map.shape
    new_dim = (w * scale_factor, h * scale_factor)
    upscaled = cv2.resize(binary_map, new_dim, interpolation=cv2.INTER_NEAREST)

    # 4. 형태학적 변환 (Morphology) - 여기가 핵심입니다!
    # 벽(검은색)을 기준으로 노이즈를 제거하기 위해 색을 반전시킵니다.
    inverted_map = cv2.bitwise_not(upscaled)

    # 커널 생성 (붓의 크기라고 생각하시면 됩니다)
    kernel_size = scale_factor  # 확대 배율에 맞춰 커널 크기 조정
    kernel = np.ones((kernel_size, kernel_size), np.uint8)

    # (1) Closing: 벽 사이의 작은 구멍이나 끊어진 부분을 메웁니다.
    closed_map = cv2.morphologyEx(inverted_map, cv2.MORPH_CLOSE, kernel)
    
    # (2) Opening: 벽 주변의 자잘한 노이즈(점)를 제거합니다.
    opened_map = cv2.morphologyEx(closed_map, cv2.MORPH_OPEN, kernel)

    # (3) Gaussian Blur: 각진 모서리를 살짝 둥글게 깎습니다.
    # 부드러운 느낌을 원하면 홀수 값을 키우세요 (예: 15, 15)
    smooth_map = cv2.GaussianBlur(opened_map, (11, 11), 0)

    # (4) 다시 선명하게 (Threshold): 블러로 흐려진 경계를 다시 뚜렷한 선으로 만듭니다.
    _, sharp_map = cv2.threshold(smooth_map, 127, 255, cv2.THRESH_BINARY)

    # 5. 색상 복구 및 저장
    # 다시 반전시켜서 흰 배경에 검은 벽으로 만듭니다.
    final_map = cv2.bitwise_not(sharp_map)
    
    cv2.imwrite(output_file, final_map)
    print(f"변환 완료! 저장된 파일: {output_file}")

# 실행
if __name__ == "__main__":
    make_map_pretty('map_Hospital.pgm', 'map_Hospital_pretty.png', scale_factor=10)