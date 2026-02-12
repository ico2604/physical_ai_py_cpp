#include <torch/torch.h>
#include <iostream>

using namespace std;
int main() {
    cout << "=== LibTorch 환경 테스트 ===" << endl;
    cout << "LibTorch 버전: " << TORCH_VERSION << endl;
    
    // 간단한 텐서 생성 및 출력
    torch::Tensor tensor = torch::rand({3, 4});
    cout << "\n랜덤 텐서:" << endl;
    cout << tensor << endl;
    
    // CUDA 사용 가능 여부 확인
    if (torch::cuda::is_available()) {
        cout << "\n✅ CUDA 사용 가능!" << endl;
        cout << "CUDA 디바이스 수: " << torch::cuda::device_count() << endl;
        
        // GPU에 텐서 생성
        torch::Tensor cuda_tensor = torch::rand({2, 3}).cuda();
        cout << "CUDA 텐서:" << endl;
        cout << cuda_tensor << endl;
    } else {
        cout << "\n⚠️  CUDA 사용 불가. CPU 모드로 동작합니다." << endl;
    }
    
    cout << "\n🎉 환경 설정이 완료되었습니다!" << endl;
    return 0;
}