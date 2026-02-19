# WSL2 에서 USB 장치 인식 가이드

## 1단계: Windows에 usbipd 설치 (Windows 터미널에서 진행)

1. 관리자 권한으로 PowerShell을 엽니다.

2. 아래 명령어로 usbipd를 설치합니다:
```PowerShell
winget install dorssel.usbipd-win
```

3. 설치 후 PowerShell 창을 닫고 다시 관리자 권한으로 엽니다.

# 2단계: 웹캠을 WSL2에 연결하기 (Windows 터미널에서 진행)

1. 장치 목록 확인:
```PowerShell
usbipd list # <확인한_BUSID>
wsl --list # <확인한_이름>
```

2. 장치 공유 설정 (최초 1회):
```PowerShell
usbipd bind --busid <확인한_BUSID>
```

3. WSL로 카메라 넘기기:
```PowerShell
usbipd attach --busid <확인한_BUSID> --wsl <확인한_이름>
```

4. WSL(Ubuntu) 터미널에서 장치 확인:
```Bash
# 장치 확인
ls -la /dev/video*

# 권한 설정
sudo chmod 666 /dev/video*

# v4l2 유틸리티 설치 (카메라 정보 확인용)
sudo apt install v4l-utils

# 카메라 지원 포맷 확인
v4l2-ctl --list-formats-ext -d /dev/video0
```
(여기서 /dev/video0 같은게 보이면 성공!)


5. 윈도우에 웹캠이 점유가 되어 오류가 발생 시, 윈도우의 웹캠 사용 브라우저 등을 종료 후 연결 해제 후 다시 연결
```PowerShell
# 1. 기존 연결 해제 (에러 나도 무시하고 진행)
usbipd detach --busid <확인한_BUSID>

# 2. 다시 연결
usbipd attach --busid <확인한_BUSID> --wsl <확인한_이름>
```