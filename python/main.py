"""
C++ 대응: src/main.cpp
"""

import sys
import cv2
from vision.webcam import WebcamCapture, WebcamError, get_error_message


def main() -> int:
    print("==========================================")
    print("   Physical AI - Webcam Module (Python)  ")
    print("==========================================")
    print(f"OpenCV: {cv2.__version__}")
    
    # 카메라 인덱스 (인자로 전달 가능)
    camera_index = 0
    if len(sys.argv) > 1:
        try:
            camera_index = int(sys.argv[1])
            print(f"[INFO] 카메라 인덱스: {camera_index}")
        except ValueError:
            print("[ERROR] 잘못된 카메라 인덱스")
    
    # 웹캠 객체 생성
    webcam = WebcamCapture(camera_index, "Physical AI - Live (Python)")
    
    # 초기화
    result = webcam.initialize()
    if result != WebcamError.SUCCESS:
        print(f"\n[FATAL] {get_error_message(result)}")
        print("\n[해결 방법]")
        print("1. 카메라 연결 확인")
        print("2. 권한 설정: sudo chmod 666 /dev/video*")
        print("3. 다른 인덱스 시도: python main.py 1")
        return result.value
    
    # 실행
    result = webcam.run()
    if result != WebcamError.SUCCESS:
        print(f"[ERROR] {get_error_message(result)}")
        return result.value
    
    print("[INFO] 정상 종료")
    return 0


if __name__ == "__main__":
    sys.exit(main())
