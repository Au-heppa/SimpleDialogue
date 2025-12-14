// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SimpleDialogueEditor : ModuleRules
{
	public SimpleDialogueEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"SimpleDialogue",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UMG",
				"UnrealEd",
				"EditorStyle", 
				"PropertyEditor",
				"InputCore",
				"GameplayTags",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		//We wan't access level editor in
		if (Target.Type == TargetType.Editor)
        { 
            //Only need this when we generate wall meshes
            PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
        }

		
       DynamicallyLoadedModuleNames.AddRange(
            new string[] { 
                "WorkspaceMenuStructure",
				"AssetTools",
				"AssetRegistry"
            }
            );
	}
}
