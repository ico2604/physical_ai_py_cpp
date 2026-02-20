#include "vision/webcam.hpp"

namespace vision {

using namespace cv;
using namespace std;

string getErrorMessage(WebcamError error) {
    switch (error) {
        case WebcamError::SUCCESS:              return "성공";
        case WebcamError::CAMERA_NOT_FOUND:     return "카메라를 찾을 수 없습니다.";
        case WebcamError::CAMERA_OPEN_FAILED:   return "카메라를 열 수 없습니다.";
        case WebcamError::FRAME_CAPTURE_FAILED: return "프레임 캡처 실패.";
        case WebcamError::INVALID_FRAME:        return "유효하지 않은 프레임.";
        case WebcamError::DISPLAY_ERROR:        return "영상 표시 오류.";
        default:                                return "알 수 없는 오류";
    }
}

WebcamCapture::WebcamCapture(int camIndex, const string& winName)
    : cameraIndex(camIndex)
    , frameWidth(640)
    , frameHeight(480)
    , isRunning(false)
    , windowName(winName)
    , mirrorMode(true)
    , contourMode(true)  // 기본값: 윤곽선 모드 ON
    , frameReady(false)
    , firstFrameLogged(false) 
    , logger("Webcam") {
}

WebcamCapture::~WebcamCapture() {
    release();
}

WebcamError WebcamCapture::initialize() {
    logger.info("카메라 " + to_string(cameraIndex) + " 초기화 중...");

    logger.info("V4L2 백엔드로 시도...");
    cap.open(cameraIndex, CAP_V4L2);

    if (!cap.isOpened()) {
        logger.warning("V4L2 실패, 기본 백엔드로 시도...");
        cap.open(cameraIndex);
    }

    if (!cap.isOpened()) {
        logger.info("다른 카메라 검색 중...");
        for (int i = 0; i < 10; i++) {
            if (i == cameraIndex) continue;
            logger.info("/dev/video" + to_string(i) + " 시도...");
            cap.open(i, CAP_V4L2);
            if (cap.isOpened()) {
                cameraIndex = i;
                logger.info("카메라 " + to_string(i) + " 발견!");
                break;
            }
        }
        if (!cap.isOpened()) {
            logger.error("카메라를 찾을 수 없습니다.");
            return WebcamError::CAMERA_NOT_FOUND;
        }
    }

    cap.set(CAP_PROP_BUFFERSIZE, 1);
    cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J', 'P', 'G'));
    cap.set(CAP_PROP_FRAME_WIDTH, frameWidth);
    cap.set(CAP_PROP_FRAME_HEIGHT, frameHeight);
    cap.set(CAP_PROP_FPS, 30);
    cap.set(CAP_PROP_CONVERT_RGB, 1);
    cap.set(CAP_PROP_AUTOFOCUS, 0);
    cap.set(CAP_PROP_AUTO_EXPOSURE, 1);

    logger.info("해상도: " + to_string((int)cap.get(CAP_PROP_FRAME_WIDTH)) + "x" 
                + to_string((int)cap.get(CAP_PROP_FRAME_HEIGHT)));
    logger.info("FPS 설정: " + to_string((int)cap.get(CAP_PROP_FPS)));
    logger.info("거울 모드: " + string(mirrorMode ? "ON" : "OFF"));
    logger.info("윤곽선 모드: " + string(contourMode ? "ON" : "OFF"));
    logger.info("초기화 완료");

    return WebcamError::SUCCESS;
}

void WebcamCapture::captureLoop() {
    Mat frame;
    while (isRunning) {
        if (cap.read(frame) && !frame.empty()) {
            lock_guard<mutex> lock(frameMutex);
            frame.copyTo(latestFrame);
            frameReady = true;
        }
    }
}

Mat WebcamCapture::convertToColor(const Mat& frame) {
    Mat colorFrame;
    
    if (frame.channels() == 1) {
        if (!firstFrameLogged) {
            logger.info("흑백 프레임 감지, 컬러로 변환");
        }
        cvtColor(frame, colorFrame, COLOR_GRAY2BGR);
    } else if (frame.channels() == 4) {
        if (!firstFrameLogged) {
            logger.info("BGRA 프레임 감지, BGR로 변환");
        }
        cvtColor(frame, colorFrame, COLOR_BGRA2BGR);
    } else if (frame.channels() == 2) {
        if (!firstFrameLogged) {
            logger.info("YUYV 프레임 감지, BGR로 변환");
        }
        cvtColor(frame, colorFrame, COLOR_YUV2BGR_YUYV);
    } else {
        if (!firstFrameLogged) {
            logger.info("BGR 컬러 프레임 (3채널), 변환 없음");
        }
        colorFrame = frame.clone();
    }
    
    // 첫 프레임 로그 완료 표시
    if (!firstFrameLogged) {
        firstFrameLogged = true;
    }
    
    
    return colorFrame;
}

Mat WebcamCapture::applyMirror(const Mat& frame) {
    Mat mirrored;
    flip(frame, mirrored, 1);
    return mirrored;
}

// ★ 엣지 검출 함수
Mat WebcamCapture::detectEdges(const Mat& frame) {
    Mat gray, blurred, edges;
    
    // 1. 흑백 변환
    cvtColor(frame, gray, COLOR_BGR2GRAY);
    
    // 2. 가우시안 블러 (노이즈 제거)
    GaussianBlur(gray, blurred, Size(5, 5), 0);
    
    // 3. Canny 엣지 검출
    Canny(blurred, edges, 50, 150);
    
    return edges;
}

// 윤곽선 검출 및 그리기 함수
void WebcamCapture::drawContours(Mat& frame, const Mat& edges) {
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    
    // RETR_TREE: 모든 윤곽선 (내부 포함)
    findContours(edges, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
    
    // 면적별 분류
    vector<tuple<int, double>> large_contours;   // 1000 이상
    vector<tuple<int, double>> medium_contours;  // 100~1000
    vector<tuple<int, double>> small_contours;   // 30~100
    
    for (size_t i = 0; i < contours.size(); i++) {
        double area = contourArea(contours[i]);
        if (area >= 1000) {
            large_contours.push_back({i, area});
        } else if (area >= 100) {
            medium_contours.push_back({i, area});
        } else if (area >= 30) {
            small_contours.push_back({i, area});
        }
    }
    
    // 작은 윤곽선 (파란색)
    for (auto& [idx, area] : small_contours) {
        cv::drawContours(frame, contours, idx, Scalar(255, 100, 0), 1);
    }
    
    // 중간 윤곽선 (노란색)
    for (auto& [idx, area] : medium_contours) {
        cv::drawContours(frame, contours, idx, Scalar(0, 255, 255), 1);
    }
    
    // 큰 윤곽선 (초록색)
    for (auto& [idx, area] : large_contours) {
        cv::drawContours(frame, contours, idx, Scalar(0, 255, 0), 2);
    }
    
    // 가장 큰 윤곽선에 바운딩박스 + 중심점
    if (!large_contours.empty()) {
        auto largest = *max_element(large_contours.begin(), large_contours.end(),
            [](auto& a, auto& b) { return get<1>(a) < get<1>(b); });
        
        int idx = get<0>(largest);
        double area = get<1>(largest);
        
        Rect bbox = boundingRect(contours[idx]);
        rectangle(frame, bbox, Scalar(255, 0, 255), 2);
        
        Moments m = moments(contours[idx]);
        if (m.m00 != 0) {
            int cx = static_cast<int>(m.m10 / m.m00);
            int cy = static_cast<int>(m.m01 / m.m00);
            circle(frame, Point(cx, cy), 5, Scalar(0, 0, 255), -1);
            putText(frame, format("Area: %d", (int)area), Point(cx - 50, cy - 15),
                   FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 0, 0), 2);
        }
    }
    
    // 통계 정보
    putText(frame, format("Large: %zu | Med: %zu | Small: %zu", 
            large_contours.size(), medium_contours.size(), small_contours.size()),
           Point(10, frame.rows - 10), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255), 1);
}

void WebcamCapture::saveScreenshot(const Mat& frame) {
    time_t now = time(nullptr);
    tm* t = localtime(&now);
    char filename[64];
    strftime(filename, sizeof(filename), "screenshot_%Y%m%d_%H%M%S.png", t);
    
    if (imwrite(filename, frame)) {
        logger.info("스크린샷 저장: " + string(filename));
    } else {
        logger.error("스크린샷 저장 실패");
    }
}

WebcamError WebcamCapture::run() {
    if (!cap.isOpened()) {
        return WebcamError::CAMERA_OPEN_FAILED;
    }

    isRunning = true;
    captureThread = thread(&WebcamCapture::captureLoop, this);

    Mat displayFrame, edgesFrame;
    int consecutiveFailures = 0;
    const int maxFailures = 100;
    bool firstFrame = true;

    logger.info("영상 스트리밍 시작");
    logger.info("단축키: 'q'/ESC=종료 | 's'=스크린샷 | 'm'=거울 | 'c'=윤곽선");

    // ★ 두 개의 윈도우 생성
    namedWindow(windowName, WINDOW_AUTOSIZE);
    namedWindow(windowName + " - Edges", WINDOW_AUTOSIZE);

    int64 startTime = getTickCount();
    int frameCount = 0;
    double currentFps = 0;

    while (isRunning) {
        Mat frame;
        {
            lock_guard<mutex> lock(frameMutex);
            if (frameReady) {
                latestFrame.copyTo(frame);
                frameReady = false;
            }
        }

        if (frame.empty()) {
            consecutiveFailures++;
            if (consecutiveFailures >= maxFailures) {
                logger.error("프레임 수신 실패");
                break;
            }
            waitKey(1);
            continue;
        }
        consecutiveFailures = 0;

        if (firstFrame) {
            logger.info("첫 프레임 수신 성공");
            logger.info("프레임 크기: " + to_string(frame.cols) + "x" + to_string(frame.rows));
            logger.info("프레임 채널: " + to_string(frame.channels()));
            firstFrame = false;
        }

        // 컬러 변환
        displayFrame = convertToColor(frame);
        //displayFrame = frame.clone();
        
        // 거울 모드
        if (mirrorMode) {
            displayFrame = applyMirror(displayFrame);
        }

        // 윤곽선 모드
        if (contourMode) {
            // 엣지 검출
            edgesFrame = detectEdges(displayFrame);
            
            // 원본에 윤곽선 그리기
            drawContours(displayFrame, edgesFrame);
            
            // 엣지 이미지를 컬러로 변환 (표시용)
            Mat edgesColor;
            cvtColor(edgesFrame, edgesColor, COLOR_GRAY2BGR);
            
            // 엣지 창에 정보 표시
            putText(edgesColor, "Edge Detection (Canny)", 
                    Point(10, 25), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 1);
            
            imshow(windowName + " - Edges", edgesColor);
        } else {
            // 윤곽선 모드 OFF면 엣지 창 숨기기
            Mat blackFrame = Mat::zeros(displayFrame.size(), CV_8UC3);
            putText(blackFrame, "Contour Mode: OFF (Press 'c' to enable)", 
                    Point(10, 25), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(100, 100, 100), 1);
            imshow(windowName + " - Edges", blackFrame);
        }

        // FPS 계산
        frameCount++;
        double elapsed = (getTickCount() - startTime) / getTickFrequency();
        if (elapsed >= 1.0) {
            currentFps = frameCount / elapsed;
            frameCount = 0;
            startTime = getTickCount();
        }

        // 정보 표시
        putText(displayFrame, "'q'=quit | 's'=screenshot | 'm'=mirror | 'c'=contour", 
                Point(10, 25), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 1);
        putText(displayFrame, format("FPS: %.1f | Mirror: %s | Contour: %s", 
                currentFps, mirrorMode ? "ON" : "OFF", contourMode ? "ON" : "OFF"), 
                Point(10, 50), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255), 1);

        imshow(windowName, displayFrame);

        // 키 입력 처리
        int key = waitKey(1) & 0xFF;
        if (key == 'q' || key == 'Q' || key == 27) {
            logger.info("종료 요청");
            isRunning = false;
        } else if (key == 's' || key == 'S') {
            saveScreenshot(displayFrame);
        } else if (key == 'm' || key == 'M') {
            mirrorMode = !mirrorMode;
            logger.info("거울 모드: " + string(mirrorMode ? "ON" : "OFF"));
        } else if (key == 'c' || key == 'C') {
            contourMode = !contourMode;
            logger.info("윤곽선 모드: " + string(contourMode ? "ON" : "OFF"));
        }

        // 윈도우 닫힘 감지
        if (getWindowProperty(windowName, WND_PROP_VISIBLE) < 1) {
            isRunning = false;
        }
    }

    return WebcamError::SUCCESS;
}

void WebcamCapture::release() {
    isRunning = false;
    
    if (captureThread.joinable()) {
        captureThread.join();
    }
    
    if (cap.isOpened()) {
        cap.release();
        logger.info("카메라 해제됨");
    }
    destroyAllWindows();
}

void WebcamCapture::setResolution(int width, int height) {
    frameWidth = width;
    frameHeight = height;
    if (cap.isOpened()) {
        cap.set(CAP_PROP_FRAME_WIDTH, frameWidth);
        cap.set(CAP_PROP_FRAME_HEIGHT, frameHeight);
    }
}

void WebcamCapture::setWindowName(const string& name) {
    windowName = name;
}

void WebcamCapture::setMirrorMode(bool enable) {
    mirrorMode = enable;
    logger.info("거울 모드: " + string(mirrorMode ? "ON" : "OFF"));
}

void WebcamCapture::setContourMode(bool enable) {
    contourMode = enable;
    logger.info("윤곽선 모드: " + string(contourMode ? "ON" : "OFF"));
}

bool WebcamCapture::isOpened() const {
    return cap.isOpened();
}

int WebcamCapture::getCameraIndex() const {
    return cameraIndex;
}

bool WebcamCapture::isMirrorMode() const {
    return mirrorMode;
}

bool WebcamCapture::isContourMode() const {
    return contourMode;
}

} // namespace vision
