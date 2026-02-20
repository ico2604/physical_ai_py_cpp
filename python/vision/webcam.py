"""
C++ 대응: src/vision/webcam.hpp, webcam.cpp
"""

import cv2
import numpy as np
from enum import Enum
from typing import Optional
from datetime import datetime
import threading


class WebcamError(Enum):
    """C++ 대응: vision::WebcamError"""
    SUCCESS = 0
    CAMERA_NOT_FOUND = 1
    CAMERA_OPEN_FAILED = 2
    FRAME_CAPTURE_FAILED = 3
    INVALID_FRAME = 4
    DISPLAY_ERROR = 5


def get_error_message(error: WebcamError) -> str:
    """C++ 대응: vision::getErrorMessage()"""
    messages = {
        WebcamError.SUCCESS: "성공",
        WebcamError.CAMERA_NOT_FOUND: "카메라를 찾을 수 없습니다.",
        WebcamError.CAMERA_OPEN_FAILED: "카메라를 열 수 없습니다.",
        WebcamError.FRAME_CAPTURE_FAILED: "프레임 캡처 실패.",
        WebcamError.INVALID_FRAME: "유효하지 않은 프레임.",
        WebcamError.DISPLAY_ERROR: "영상 표시 오류."
    }
    return messages.get(error, "알 수 없는 오류")


class WebcamCapture:
    """C++ 대응: vision::WebcamCapture 클래스"""
    
    def __init__(self, cam_index: int = 0, win_name: str = "Webcam Live"):
        self._cap: Optional[cv2.VideoCapture] = None
        self._camera_index: int = cam_index
        self._frame_width: int = 640
        self._frame_height: int = 480
        self._is_running: bool = False
        self._window_name: str = win_name
        self._mirror_mode: bool = True
        self._contour_mode: bool = True
        
        # 멀티스레드용
        self._latest_frame: Optional[np.ndarray] = None
        self._frame_mutex: threading.Lock = threading.Lock()
        self._frame_ready: bool = False
        self._capture_thread: Optional[threading.Thread] = None
    
    def __del__(self):
        self.release()
    
    def initialize(self) -> WebcamError:
        print(f"[INFO] 카메라 {self._camera_index} 초기화 중...")
        
        # V4L2 백엔드로 시도
        print("[INFO] V4L2 백엔드로 시도...")
        self._cap = cv2.VideoCapture(self._camera_index, cv2.CAP_V4L2)
        
        if not self._cap.isOpened():
            print("[WARN] V4L2 실패, 기본 백엔드로 시도...")
            self._cap = cv2.VideoCapture(self._camera_index)
        
        if not self._cap.isOpened():
            print("[INFO] 다른 카메라 검색 중...")
            for i in range(10):
                if i == self._camera_index:
                    continue
                print(f"[INFO] /dev/video{i} 시도...")
                self._cap = cv2.VideoCapture(i, cv2.CAP_V4L2)
                if self._cap.isOpened():
                    self._camera_index = i
                    print(f"[INFO] 카메라 {i} 발견!")
                    break
            
            if not self._cap.isOpened():
                print("[ERROR] 카메라를 찾을 수 없습니다.")
                return WebcamError.CAMERA_NOT_FOUND
        
        # 카메라 설정
        self._cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
        self._cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc('M', 'J', 'P', 'G'))
        self._cap.set(cv2.CAP_PROP_FRAME_WIDTH, self._frame_width)
        self._cap.set(cv2.CAP_PROP_FRAME_HEIGHT, self._frame_height)
        self._cap.set(cv2.CAP_PROP_FPS, 30)
        self._cap.set(cv2.CAP_PROP_CONVERT_RGB, 1)
        self._cap.set(cv2.CAP_PROP_AUTOFOCUS, 0)
        self._cap.set(cv2.CAP_PROP_AUTO_EXPOSURE, 1)
        
        actual_width = int(self._cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        actual_height = int(self._cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
        actual_fps = int(self._cap.get(cv2.CAP_PROP_FPS))
        
        print(f"[INFO] 해상도: {actual_width}x{actual_height}")
        print(f"[INFO] FPS 설정: {actual_fps}")
        print(f"[INFO] 거울 모드: {'ON' if self._mirror_mode else 'OFF'}")
        print(f"[INFO] 윤곽선 모드: {'ON' if self._contour_mode else 'OFF'}")
        print("[INFO] 초기화 완료")
        
        return WebcamError.SUCCESS
    
    def _capture_loop(self) -> None:
        """캡처 스레드 루프"""
        while self._is_running:
            if self._cap is None:
                break
            
            ret, frame = self._cap.read()
            
            if ret and frame is not None and frame.size > 0:
                with self._frame_mutex:
                    self._latest_frame = frame.copy()
                    self._frame_ready = True
    
    def _convert_to_color(self, frame: np.ndarray) -> np.ndarray:
        """컬러 변환"""
        if len(frame.shape) == 2:
            return cv2.cvtColor(frame, cv2.COLOR_GRAY2BGR)
        elif frame.shape[2] == 4:
            return cv2.cvtColor(frame, cv2.COLOR_BGRA2BGR)
        return frame.copy()
    
    def _apply_mirror(self, frame: np.ndarray) -> np.ndarray:
        """좌우 반전"""
        return cv2.flip(frame, 1)
    
    def _detect_edges(self, frame: np.ndarray) -> np.ndarray:
        """엣지 검출"""
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        blurred = cv2.GaussianBlur(gray, (5, 5), 0)
        edges = cv2.Canny(blurred, 50, 150)
        return edges
    
    def _draw_contours(self, frame: np.ndarray, edges: np.ndarray) -> None:
        """모든 윤곽선 검출 및 그리기"""
        # RETR_TREE: 모든 윤곽선 (내부 포함)
        contours, hierarchy = cv2.findContours(
            edges, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE
        )
        
        # 면적 기준으로 분류
        large_contours = []   # 큰 윤곽선 (1000 이상)
        medium_contours = []  # 중간 윤곽선 (100~1000)
        small_contours = []   # 작은 윤곽선 (30~100)
        
        for i, contour in enumerate(contours):
            area = cv2.contourArea(contour)
            if area >= 1000:
                large_contours.append((i, contour, area))
            elif area >= 100:
                medium_contours.append((i, contour, area))
            elif area >= 30:
                small_contours.append((i, contour, area))
        
        # 작은 윤곽선 (파란색) - 눈코입 등
        for idx, contour, area in small_contours:
            cv2.drawContours(frame, [contour], -1, (255, 100, 0), 1)
        
        # 중간 윤곽선 (노란색)
        for idx, contour, area in medium_contours:
            cv2.drawContours(frame, [contour], -1, (0, 255, 255), 1)
        
        # 큰 윤곽선 (초록색) - 가장 큰 것에 추가 정보 표시
        for idx, contour, area in large_contours:
            cv2.drawContours(frame, [contour], -1, (0, 255, 0), 2)
        
        # 가장 큰 윤곽선에 바운딩박스 + 중심점
        if large_contours:
            largest = max(large_contours, key=lambda x: x[2])
            idx, contour, area = largest
            
            # 바운딩 박스
            x, y, w, h = cv2.boundingRect(contour)
            cv2.rectangle(frame, (x, y), (x + w, y + h), (255, 0, 255), 2)
            
            # 중심점
            m = cv2.moments(contour)
            if m["m00"] != 0:
                cx = int(m["m10"] / m["m00"])
                cy = int(m["m01"] / m["m00"])
                cv2.circle(frame, (cx, cy), 5, (0, 0, 255), -1)
                cv2.putText(frame, f"Area: {int(area)}", (cx - 50, cy - 15),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 0, 0), 2)
        
        # 통계 정보
        cv2.putText(frame, f"Large: {len(large_contours)} | Med: {len(medium_contours)} | Small: {len(small_contours)}",
                (10, frame.shape[0] - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
    
    def _save_screenshot(self, frame: np.ndarray) -> None:
        """스크린샷 저장"""
        filename = datetime.now().strftime("screenshot_%Y%m%d_%H%M%S.png")
        if cv2.imwrite(filename, frame):
            print(f"[INFO] 스크린샷 저장: {filename}")
        else:
            print("[ERROR] 스크린샷 저장 실패")
    
    def run(self) -> WebcamError:
        """메인 실행 루프"""
        if self._cap is None or not self._cap.isOpened():
            return WebcamError.CAMERA_OPEN_FAILED
        
        self._is_running = True
        self._capture_thread = threading.Thread(target=self._capture_loop)
        self._capture_thread.start()
        
        consecutive_failures = 0
        max_failures = 100
        
        print("[INFO] 영상 스트리밍 시작")
        print("[INFO] 단축키: 'q'/ESC=종료 | 's'=스크린샷 | 'm'=거울 | 'c'=윤곽선")
        
        cv2.namedWindow(self._window_name, cv2.WINDOW_AUTOSIZE)
        cv2.namedWindow(f"{self._window_name} - Edges", cv2.WINDOW_AUTOSIZE)
        
        # FPS 측정
        start_time = cv2.getTickCount()
        frame_count = 0
        current_fps = 0.0
        
        while self._is_running:
            frame = None
            
            with self._frame_mutex:
                if self._frame_ready:
                    frame = self._latest_frame.copy() if self._latest_frame is not None else None
                    self._frame_ready = False
            
            if frame is None:
                consecutive_failures += 1
                if consecutive_failures >= max_failures:
                    print("[ERROR] 프레임 수신 실패")
                    break
                cv2.waitKey(1)
                continue
            
            consecutive_failures = 0
            
            # 컬러 변환
            display_frame = self._convert_to_color(frame)
            
            # 거울 모드
            if self._mirror_mode:
                display_frame = self._apply_mirror(display_frame)
            
            # 윤곽선 모드
            if self._contour_mode:
                edges_frame = self._detect_edges(display_frame)
                self._draw_contours(display_frame, edges_frame)
                
                edges_color = cv2.cvtColor(edges_frame, cv2.COLOR_GRAY2BGR)
                cv2.putText(edges_color, "Edge Detection (Canny)",
                           (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
                cv2.imshow(f"{self._window_name} - Edges", edges_color)
            else:
                black_frame = np.zeros(display_frame.shape, dtype=np.uint8)
                cv2.putText(black_frame, "Contour Mode: OFF (Press 'c' to enable)",
                           (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (100, 100, 100), 1)
                cv2.imshow(f"{self._window_name} - Edges", black_frame)
            
            # FPS 계산
            frame_count += 1
            elapsed = (cv2.getTickCount() - start_time) / cv2.getTickFrequency()
            if elapsed >= 1.0:
                current_fps = frame_count / elapsed
                frame_count = 0
                start_time = cv2.getTickCount()
            
            # 정보 표시
            cv2.putText(display_frame, "'q'=quit | 's'=screenshot | 'm'=mirror | 'c'=contour",
                       (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
            
            mirror_str = "ON" if self._mirror_mode else "OFF"
            contour_str = "ON" if self._contour_mode else "OFF"
            cv2.putText(display_frame, f"FPS: {current_fps:.1f} | Mirror: {mirror_str} | Contour: {contour_str}",
                       (10, 50), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 255), 1)
            
            cv2.imshow(self._window_name, display_frame)
            
            # 키 입력
            key = cv2.waitKey(1) & 0xFF
            
            if key == ord('q') or key == ord('Q') or key == 27:
                print("[INFO] 종료 요청")
                self._is_running = False
            elif key == ord('s') or key == ord('S'):
                self._save_screenshot(display_frame)
            elif key == ord('m') or key == ord('M'):
                self._mirror_mode = not self._mirror_mode
                print(f"[INFO] 거울 모드: {'ON' if self._mirror_mode else 'OFF'}")
            elif key == ord('c') or key == ord('C'):
                self._contour_mode = not self._contour_mode
                print(f"[INFO] 윤곽선 모드: {'ON' if self._contour_mode else 'OFF'}")
            
            # 윈도우 닫힘 감지
            if cv2.getWindowProperty(self._window_name, cv2.WND_PROP_VISIBLE) < 1:
                self._is_running = False
        
        return WebcamError.SUCCESS
    
    def release(self) -> None:
        """리소스 해제"""
        self._is_running = False
        
        if self._capture_thread is not None and self._capture_thread.is_alive():
            self._capture_thread.join()
        
        if self._cap is not None and self._cap.isOpened():
            self._cap.release()
            print("[INFO] 카메라 해제됨")
        
        cv2.destroyAllWindows()
    
    # Setter / Getter
    def set_resolution(self, width: int, height: int) -> None:
        self._frame_width = width
        self._frame_height = height
        if self._cap is not None and self._cap.isOpened():
            self._cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
            self._cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
    
    def set_mirror_mode(self, enable: bool) -> None:
        self._mirror_mode = enable
        print(f"[INFO] 거울 모드: {'ON' if enable else 'OFF'}")
    
    def set_contour_mode(self, enable: bool) -> None:
        self._contour_mode = enable
        print(f"[INFO] 윤곽선 모드: {'ON' if enable else 'OFF'}")
    
    def is_opened(self) -> bool:
        return self._cap is not None and self._cap.isOpened()
    
    def get_camera_index(self) -> int:
        return self._camera_index
    
    def is_mirror_mode(self) -> bool:
        return self._mirror_mode
    
    def is_contour_mode(self) -> bool:
        return self._contour_mode
