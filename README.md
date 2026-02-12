# 🚀 WSL2 + Ubuntu + CUDA + PyTorch + LibTorch 개발 환경 구축 가이드

본 문서는 Windows + WSL2 + Ubuntu 22.04 환경에서  
CUDA, PyTorch, LibTorch(C++) 개발 환경을 구축하는 전체 과정을 정리한 가이드입니다.

---

# 1️⃣ 1단계: WSL2 설치 및 Ubuntu 설정

## ✅ WSL2 기능 활성화

Windows 10 (버전 2004 이상) 또는 Windows 11에서 가능합니다.  
**관리자 권한 PowerShell** 실행 후:

```bash
wsl --install
```

---

## ✅ WSL 버전 확인

```bash
wsl --list --verbose
```

Ubuntu가 **VERSION 2**로 실행되고 있어야 합니다.

버전이 1이라면:

```bash
wsl --set-version Ubuntu-22.04 2
```

---

# 2️⃣ 2단계: Visual Studio Code 설정

## ✅ 필수 확장 프로그램 설치

VSCode → Extensions에서 설치:

- WSL (Microsoft)
- C/C++ (Microsoft)
- C/C++ Extension Pack (Microsoft)
- CMake Tools (Microsoft)
- Python (Microsoft)

---

## ✅ WSL과 VSCode 연결

Ubuntu 터미널에서:

```bash
code .
```

---

# 3️⃣ 3단계: Ubuntu 기본 개발 환경 구축

## ✅ 시스템 업데이트

```bash
sudo apt update && sudo apt upgrade -y
```

---

## ✅ 기본 개발 도구 설치

```bash
sudo apt install -y build-essential cmake git wget curl unzip
```

---

## ✅ 설치 확인

```bash
gcc --version
g++ --version
cmake --version
```

---

# 4️⃣ 4단계: NVIDIA GPU 및 CUDA 설정 (GPU가 있는 경우)

## ✅ CUDA Toolkit 설치

```bash
wget https://developer.download.nvidia.com/compute/cuda/repos/wsl-ubuntu/x86_64/cuda-keyring_1.1-1_all.deb
sudo dpkg -i cuda-keyring_1.1-1_all.deb
sudo apt update
sudo apt install -y cuda-toolkit-12-3
```

---

## ❗ libtinfo5 오류 발생 시 해결

### 🔎 문제 원인

- Ubuntu 22.04는 `libtinfo6`만 기본 제공
- CUDA 12.3의 Nsight Systems가 `libtinfo5`에 의존

### ✅ 해결 방법

```bash
wget http://archive.ubuntu.com/ubuntu/pool/universe/n/ncurses/libtinfo5_6.3-2ubuntu0.1_amd64.deb
sudo dpkg -i libtinfo5_6.3-2ubuntu0.1_amd64.deb
sudo apt install -y cuda-toolkit-12-3
```

---

## ✅ 설치 확인

```bash
/usr/local/cuda-12.3/bin/nvcc --version
nvidia-smi
```

---

## ✅ 환경 변수 설정

```bash
echo 'export PATH=/usr/local/cuda-12.3/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/local/cuda-12.3/lib64:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc
```

확인:

```bash
nvcc --version
nvidia-smi
```

---

# 5️⃣ 5단계: Python 환경 및 PyTorch 설치

## ✅ Python 및 가상환경 도구 설치

```bash
sudo apt install -y python3 python3-pip python3-venv
```

---

## ✅ 프로젝트 디렉토리 생성

```bash
mkdir -p ~/projects/physical_ai
cd ~/projects/physical_ai
```

---

## ✅ 가상환경 생성 및 활성화

```bash
python3 -m venv venv
source venv/bin/activate
```

---

## ✅ PyTorch 설치

### GPU 버전

```bash
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu121
```

### CPU 버전

```bash
# pip install torch torchvision torchaudio
```

---

## ✅ 설치 확인

```bash
python3 -c "import torch; print(f'PyTorch version: {torch.__version__}'); print(f'CUDA available: {torch.cuda.is_available()}')"
```

---

# 6️⃣ 6단계: LibTorch 설치 (C++ 환경)

## ✅ 다운로드

```bash
cd ~/projects

wget https://download.pytorch.org/libtorch/cu121/libtorch-cxx11-abi-shared-with-deps-2.1.2%2Bcu121.zip
unzip libtorch-cxx11-abi-shared-with-deps-2.1.2+cu121.zip
```

---

## ✅ 환경 변수 설정

```bash
echo 'export LIBTORCH_PATH=~/projects/libtorch' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=$LIBTORCH_PATH/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc
```

---

# 7️⃣ 7단계: 테스트 프로젝트 생성

```bash
cd ~/projects/physical_ai
mkdir -p src build
```

---

## ✅ CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.18)
project(physical_ai_test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_PREFIX_PATH "$ENV{LIBTORCH_PATH}")

find_package(Torch REQUIRED)

add_executable(test_app src/main.cpp)
target_link_libraries(test_app "${TORCH_LIBRARIES}")

set_target_properties(test_app PROPERTIES
    BUILD_RPATH "${TORCH_INSTALL_PREFIX}/lib"
)
```

---

## ✅ src/main.cpp

```cpp
#include <torch/torch.h>
#include <iostream>

int main() {
    std::cout << "=== LibTorch 환경 테스트 ===" << std::endl;
    std::cout << "LibTorch 버전: " << TORCH_VERSION << std::endl;

    torch::Tensor tensor = torch::rand({3, 4});
    std::cout << "\n랜덤 텐서:" << std::endl;
    std::cout << tensor << std::endl;

    if (torch::cuda::is_available()) {
        std::cout << "\n✅ CUDA 사용 가능!" << std::endl;
        std::cout << "CUDA 디바이스 수: " << torch::cuda::device_count() << std::endl;

        torch::Tensor cuda_tensor = torch::rand({2, 3}).cuda();
        std::cout << "CUDA 텐서:" << std::endl;
        std::cout << cuda_tensor << std::endl;
    } else {
        std::cout << "\n⚠️ CUDA 사용 불가. CPU 모드로 동작합니다." << std::endl;
    }

    std::cout << "\n🎉 환경 설정이 완료되었습니다!" << std::endl;
    return 0;
}
```

---

## ✅ 빌드 및 실행

```bash
cd ~/projects/physical_ai
mkdir -p build
cd build
cmake ..
cmake --build .
./test_app
```

---

# ❗ GCC 버전 오류 해결

에러 메시지:

```
#error -- unsupported GNU version! gcc versions later than 12 are not supported!
```

CUDA 12.3은 GCC 12까지만 지원합니다.

---

## ✅ GCC 12 설치

```bash
sudo apt update
sudo apt install -y gcc-12 g++-12
```

---

## ✅ 빌드 폴더 초기화

```bash
cd ~/projects/physical_ai
rm -rf build
mkdir build
cd build
```

---

## ✅ 컴파일러 명시 지정

```bash
cmake -DCMAKE_C_COMPILER=gcc-12 \
      -DCMAKE_CXX_COMPILER=g++-12 \
      -DCMAKE_CUDA_HOST_COMPILER=g++-12 \
      ..
```

```bash
cmake --build .
./test_app
```

---

# 8️⃣ VSCode 프로젝트 관리 설정

## 📂 프로젝트 열기

```
/home/사용자이름/projects/physical_ai
```

---

## ⚙ CMake 설정

1. Ctrl + Shift + P
2. `CMake: Configure`
3. GCC 선택
4. Debug 또는 Release 선택

---

## ⌨ 단축키

- F7 → 빌드
- F5 → 디버깅 실행
- Ctrl + F5 → 일반 실행

---

# 🧩 추가 유용한 도구

## OpenCV

```bash
sudo apt install -y libopencv-dev
```

## Eigen

```bash
sudo apt install -y libeigen3-dev
```

## Python 라이브러리

```bash
source ~/projects/physical_ai/venv/bin/activate
pip install numpy matplotlib jupyter opencv-python
```

---

# 🗂 Git 설정

```bash
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"

cd ~/projects/physical_ai
git init
```

`.gitignore` 파일 생성 권장

---

# 🔧 문제 해결

## CUDA 문제 발생 시

```bash
wsl --shutdown
```

Windows NVIDIA 드라이버 재설치 후 재시작

---

## 빌드 에러 발생 시

```bash
cd ~/projects/physical_ai
rm -rf build
mkdir build
cd build
cmake ..
cmake --build .
```

---

# 🎉 완료

이제 WSL2 + CUDA + PyTorch + LibTorch C++ 개발 환경이 완성되었습니다.
