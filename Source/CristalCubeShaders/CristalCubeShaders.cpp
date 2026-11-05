// CristalCubeShaders.cpp — <Project>/Shaders 를 가상 경로 /CristalCube 로 매핑 (rev.6 · golden-rev6-b4-provide §2)
// 위치: Source/CristalCubeShaders/CristalCubeShaders.cpp
//
// - .uproject 에서 LoadingPhase = PostConfigInit 으로 등록해야 함 (셰이더 컴파일·머티리얼 로드보다 먼저).
//   게임 모듈(CristalCube, Default 단계)에서 매핑하면 에디터 시작 시 머티리얼이 먼저 컴파일되어
//   "Can't map virtual shader source path /CristalCube/UISurface.ush" 오류가 날 수 있음 → 별도 모듈로 분리.
// - UObject 없음 (PostConfigInit 단계는 UObject 시스템 초기화 전).
// - 패키징(쿡) 빌드는 셰이더가 이미 컴파일돼 있어 폴더가 필요 없음 → 엔진 함수가 자체적으로 건너뜀.

#include "Modules/ModuleManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProperties.h"
#include "ShaderCore.h"

class FCristalCubeShadersModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		if (FPlatformProperties::RequiresCookedData())
		{
			return;                                  // 패키징 빌드 — 셰이더는 쿡 단계에서 이미 컴파일됨
		}

		static const TCHAR* VirtualDir = TEXT("/CristalCube");
		const FString RealDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders")));

		if (AllShaderSourceDirectoryMappings().Contains(VirtualDir))
		{
			return;                                  // 이미 매핑됨 (모듈 재로드 등)
		}
		if (!FPaths::DirectoryExists(RealDir))
		{
			// 엔진 함수는 폴더가 없으면 check()로 멈춤 → 경고만 남기고 건너뜀
			UE_LOG(LogTemp, Warning, TEXT("[CristalCubeShaders] Shader folder not found: %s (/CristalCube 매핑 생략)"), *RealDir);
			return;
		}
		AddShaderSourceDirectoryMapping(VirtualDir, RealDir);
		UE_LOG(LogTemp, Log, TEXT("[CristalCubeShaders] /CristalCube -> %s"), *RealDir);
	}

	virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FCristalCubeShadersModule, CristalCubeShaders)
