#ifndef WEBCAM_HPP
#define WEBCAM_HPP

#include <opencv2/opencv.hpp>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include "utils/logger.hpp"

namespace vision {

enum class WebcamError {
    SUCCESS = 0,
    CAMERA_NOT_FOUND,
    CAMERA_OPEN_FAILED,
    FRAME_CAPTURE_FAILED,
    INVALID_FRAME,
    DISPLAY_ERROR
};

std::string getErrorMessage(WebcamError error);

class WebcamCapture {
private:
    cv::VideoCapture cap;
    int cameraIndex;
    int frameWidth;
    int frameHeight;
    std::atomic<bool> isRunning;
    std::string windowName;
    bool mirrorMode;
    bool contourMode;  // ★ 윤곽선 모드 추가
    bool firstFrameLogged; // 첫 프레임 로그 여부
    
    // 멀티스레드용
    cv::Mat latestFrame;
    std::mutex frameMutex;
    std::atomic<bool> frameReady;
    std::thread captureThread;
    
    // 로거
    plog::Logger logger;

    // 내부 함수
    void captureLoop();
    void saveScreenshot(const cv::Mat& frame);
    cv::Mat convertToColor(const cv::Mat& frame);
    cv::Mat applyMirror(const cv::Mat& frame);
    
    // 윤곽선 처리 함수 추가
    cv::Mat detectEdges(const cv::Mat& frame);
    void drawContours(cv::Mat& frame, const cv::Mat& edges);

public:
    WebcamCapture(int camIndex = 0, const std::string& winName = "Webcam Live");
    ~WebcamCapture();

    WebcamCapture(const WebcamCapture&) = delete;
    WebcamCapture& operator=(const WebcamCapture&) = delete;

    WebcamError initialize();
    WebcamError run();
    void release();
    
    void setResolution(int width, int height);
    void setWindowName(const std::string& name);
    void setMirrorMode(bool enable);
    void setContourMode(bool enable); 
    
    bool isOpened() const;
    int getCameraIndex() const;
    bool isMirrorMode() const;
    bool isContourMode() const; 
};

} // namespace vision

#endif // WEBCAM_HPP
