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
    , frameReady(false)
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

    logger.info("해상도: " + to_string((int)cap.get(CAP_PROP_FRAME_WIDTH)) + "x" 
                + to_string((int)cap.get(CAP_PROP_FRAME_HEIGHT)));
    logger.info("FPS: " + to_string((int)cap.get(CAP_PROP_FPS)));
    logger.info("거울 모드: " + string(mirrorMode ? "ON" : "OFF"));
    logger.info("초기화 완료");

    return WebcamError::SUCCESS;
}

void WebcamCapture::captureLoop() {
    Mat frame;
    while (isRunning) {
        for (int i = 0; i < 2; i++) {
            cap.grab();
        }
        
        if (cap.retrieve(frame) && !frame.empty()) {
            lock_guard<mutex> lock(frameMutex);
            frame.copyTo(latestFrame);
            frameReady = true;
        }
        
        this_thread::sleep_for(chrono::milliseconds(1));
    }
}

Mat WebcamCapture::convertToColor(const Mat& frame) {
    Mat colorFrame;
    
    if (frame.channels() == 1) {
        cvtColor(frame, colorFrame, COLOR_GRAY2BGR);
    } else if (frame.channels() == 4) {
        cvtColor(frame, colorFrame, COLOR_BGRA2BGR);
    } else if (frame.channels() == 2) {
        cvtColor(frame, colorFrame, COLOR_YUV2BGR_YUYV);
    } else {
        colorFrame = frame.clone();
    }
    
    return colorFrame;
}

Mat WebcamCapture::applyMirror(const Mat& frame) {
    Mat mirrored;
    flip(frame, mirrored, 1);
    return mirrored;
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

    Mat displayFrame;
    int consecutiveFailures = 0;
    const int maxFailures = 100;

    logger.info("영상 스트리밍 시작");
    logger.info("단축키: 'q'/ESC=종료 | 's'=스크린샷 | 'm'=거울모드 토글");

    namedWindow(windowName, WINDOW_AUTOSIZE);

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

        displayFrame = convertToColor(frame);
        
        if (mirrorMode) {
            displayFrame = applyMirror(displayFrame);
        }

        frameCount++;
        double elapsed = (getTickCount() - startTime) / getTickFrequency();
        if (elapsed >= 1.0) {
            currentFps = frameCount / elapsed;
            frameCount = 0;
            startTime = getTickCount();
        }

        putText(displayFrame, "'q'=quit | 's'=screenshot | 'm'=mirror toggle", 
                Point(10, 25), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 1);
        putText(displayFrame, format("FPS: %.1f | Mirror: %s", currentFps, mirrorMode ? "ON" : "OFF"), 
                Point(10, 50), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255), 1);

        imshow(windowName, displayFrame);

        int key = waitKey(1) & 0xFF;
        if (key == 'q' || key == 'Q' || key == 27) {
            logger.info("종료 요청");
            isRunning = false;
        } else if (key == 's' || key == 'S') {
            saveScreenshot(displayFrame);
        } else if (key == 'm' || key == 'M') {
            mirrorMode = !mirrorMode;
            logger.info("거울 모드: " + string(mirrorMode ? "ON" : "OFF"));
        }

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

bool WebcamCapture::isOpened() const {
    return cap.isOpened();
}

int WebcamCapture::getCameraIndex() const {
    return cameraIndex;
}

bool WebcamCapture::isMirrorMode() const {
    return mirrorMode;
}

} // namespace vision
