// CristalCubeShaders.Build.cs — 셰이더 경로 매핑 전용 모듈 (rev.6 · golden-rev6-b4-provide §2)
// 위치: Source/CristalCubeShaders/CristalCubeShaders.Build.cs

using UnrealBuildTool;

public class CristalCubeShaders : ModuleRules
{
	public CristalCubeShaders(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// AddShaderSourceDirectoryMapping = RenderCore (ShaderCore.h)
		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "RenderCore" });
	}
}
