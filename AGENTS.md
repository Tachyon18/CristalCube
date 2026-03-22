# AGENTS.md

CristalCube는 Unreal Engine 5.6 기반의 단일 런타임 모듈이다.

## 저장소 구조

- 주 게임플레이 코드는 `Source/CristalCube/` 아래에 있다.
- 공용 enum, delegate, DataTable 구조체는 `Source/CristalCube/CristalCubeStruct.h`에 모여 있다.
- 공통 게임 흐름은 `CC_GameModeBase`, 메인 사이클과 큐브 클리어 흐름은 `CC_MainGameMode`가 담당한다.
- 무기 조회와 DataTable 접근 경계는 `UCC_WeaponManagerSubsystem`이다.
- 디렉터리 역할은 다음과 같다.
- `Characters/`: 플레이어와 적 액터
- `Gameplay/`: 큐브, 타일, 젬 등 월드 게임플레이 로직
- `SkillSystem/`: 모듈형 스킬 로직
- `WeaponSystems/`: 무기와 발사체 로직
- `Widgets/`: UI 위젯
- `Animation/`: 애님 노티파이
- `Test/`: 런타임 테스트 액터와 보조 코드

## 유지할 규칙

- 기존 프로젝트 접두어 규칙 `ACC_`, `UCC_`, `FCC_`, `ECC_`를 유지한다.
- 리플렉션에 연결된 타입을 수정할 때는 `UCLASS`, `USTRUCT`, `UENUM`, `UPROPERTY`, `UFUNCTION` 사용을 보존한다.
- `Source/CristalCube/Test/`는 자동화 테스트가 아니라 런타임 테스트 코드로 취급한다.

## 인코딩 규칙

- 이 저장소는 한글 주석 인코딩 이슈가 있으니, 비-UTF8 파일은 변환 승인 없이 수정하지 말 것.

## 확인된 사실

- `Source/` 아래에서 Unreal Automation Test 매크로는 발견되지 않았다.
- 저장소 안에는 전용 빌드 스크립트나 테스트 스크립트가 없다.
