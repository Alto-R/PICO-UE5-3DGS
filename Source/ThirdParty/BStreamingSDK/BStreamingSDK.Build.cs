// External module wrapping the PICO Business Streaming SDK (PC-side, Win64 only).
// Eye-tracking data is pulled over the streaming link via BStreamingSDK_MsgJson.
// The .dll is delay-loaded so a missing SDK / streaming runtime degrades to a
// fallback backend instead of crashing at startup.

using System.IO;
using UnrealBuildTool;

public class BStreamingSDK : ModuleRules
{
	public BStreamingSDK(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicSystemIncludePaths.Add(Path.Combine(ModuleDirectory, "include"));

			string LibDir = Path.Combine(ModuleDirectory, "lib", "Win64");
			PublicAdditionalLibraries.Add(Path.Combine(LibDir, "BStreamingSDK.lib"));

			const string Dll = "BStreamingSDK.dll";
			PublicDelayLoadDLLs.Add(Dll);
			RuntimeDependencies.Add(
				Path.Combine("$(BinaryOutputDir)", Dll),
				Path.Combine(LibDir, Dll));
		}
	}
}
