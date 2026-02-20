#include <iostream>
#include "vision/webcam.hpp"
#include "utils/logger.hpp"

using namespace std;
using namespace vision;

int main(int argc, char** argv) {
    // 출력 버퍼 즉시 플러시 설정
    cout.setf(ios::unitbuf);
    cerr.setf(ios::unitbuf);
    
    plog::logInfo("==========================================");
    plog::logInfo("   Physical AI - Webcam Module           ");
    plog::logInfo("==========================================");
    plog::logInfo("OpenCV: " + string(CV_VERSION));

    int cameraIndex = 0;
    if (argc > 1) {
        cameraIndex = atoi(argv[1]);
        plog::logInfo("카메라 인덱스: " + to_string(cameraIndex));
    }

    WebcamCapture webcam(cameraIndex, "Physical AI - Live");

    WebcamError result = webcam.initialize();
    if (result != WebcamError::SUCCESS) {
        plog::logFatal(getErrorMessage(result));
        plog::logError("해결 방법:");
        plog::logError("1. 카메라 연결 확인");
        plog::logError("2. 권한 설정: sudo chmod 666 /dev/video*");
        plog::logError("3. 다른 인덱스 시도: ./main 1");
        return static_cast<int>(result);
    }

    result = webcam.run();
    if (result != WebcamError::SUCCESS) {
        plog::logError(getErrorMessage(result));
        return static_cast<int>(result);
    }

    plog::logInfo("정상 종료");
    return 0;
}
