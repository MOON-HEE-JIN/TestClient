# TestClient LoginServer HTTP 통신

`Http/CHttpClient.h`와 `Http/CHttpClient.cpp`는 GameServer HTTP 클라이언트와 같은 libcurl + JSON POST 방식으로 `POST /api/clientconnection/login`을 호출한다.

## 실행

PowerShell에서 실제 LoginServer 계정에 맞게 설정한다. 비밀번호와 계정은 소스에 저장하지 않는다.

```powershell
$env:LOGIN_SERVER_URL = 'http://127.0.0.1:5080'
$env:LOGIN_ACCOUNT = '설정된 계정 ID'
$env:LOGIN_PASSWORD = '설정된 계정 비밀번호'
& 'D:\TestClient\x64\Debug\TestClient.exe' --login-only
```

`--login-only`는 main.cpp에서 로그인 요청과 응답 처리까지 수행하고 종료한다. 인자 없이 실행하면 API 시험 후 기존 UI와 게임·채팅 TCP 연결 흐름을 계속 실행한다. API 시험의 성공·실패는 기존 TCP 흐름을 제어하지 않는다. GameServer·ChatServer 인증은 추가하지 않았다. 기존 `--aoi-auto` 실행 흐름도 유지한다. LOGIN_SERVER_URL 기본값은 `http://127.0.0.1:5080`이다. Windows 환경 변수는 UTF-8로 변환해서 JSON에 넣는다.

성공 시 `HTTP 200`, `SUCCESS`, 만료 시각과 `token received`를 출력한다. Base64 토큰은 `loginResult.token`에 있으며 원문은 출력하지 않는다. 실패 시 HTTP 상태·서버 오류 코드·메시지를 출력한다. `--login-only`에서는 실패 시 종료 코드 1, 필수 계정 설정 누락 시 종료 코드 2다. 일반 실행은 API 시험 결과와 관계없이 기존 흐름으로 계속 진행한다.

LoginServer에는 유효한 계정 인증 공급자와 GameServer의 활성 상태·현재 Connect 보고와 LoginServer에 설정된 MaxConnect가 필요하다. 기본 DisabledAuthenticator에서는 성공하지 않는다. 운영자 로그인 계정과 게임 로그인 계정은 별개다. HTTPS 사용 시 인증서 검증을 유지한다.

## 빌드와 검증

Debug/Release x64에 현재 ThirdParty의 curl·JSON 경로, MSVC import library 생성 및 DLL 자동 복사를 연결했다.

```powershell
node D:\TestClient\TestClient\Verification\login.cjs
```

격리된 LoginServer와 테스트용 서버 상태를 준비한 뒤 실제 Debug·Release 실행 파일을 `--login-only`로 호출한다. 성공, 잘못된 계정, 서버 상태 미수신, 정원 초과, 네트워크 장애, 잘못된 JSON 응답을 검사한다. 서버 설정과 임시 데이터는 Verification/output 아래에 생성하며 실제 운영 서버 프로세스는 사용하지 않는다. LoginServer의 현재 소스를 빌드한 DLL이 필요하다.
