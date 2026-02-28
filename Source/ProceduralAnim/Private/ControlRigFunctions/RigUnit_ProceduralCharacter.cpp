#include "ControlRigFunctions/RigUnit_ProceduralCharacter.h"
#include "CoreMinimal.h"
#include "Units/RigUnitContext.h"
#include "RigVMCore/RigVMDrawInterface.h"
#include "ControlRig/Public/Rigs/RigHierarchy.h"
#include "Transform/TransformableHandleUtils.h"

FRigUnit_SetupFootArray_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	
	URigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	
	if(!Hierarchy)
	{
		return;
	}
	
	FootArray.Reset();
	LockedFootLocationArray.Reset();
	IsFootLockedArray.Reset();

	const FRigElementKey RootBoneKey(RootName, ERigElementType::Bone);
	if (!Hierarchy->Contains(RootBoneKey))
	{
		return;
	}

	for (const FRigElementKey& ChildKey : Hierarchy->GetChildren(RootBoneKey, true))
	{
		if (ChildKey.Type != ERigElementType::Bone)
		{
			continue;
		}
		const FString BoneNameStr = ChildKey.Name.ToString();
		
		if (BoneNameStr.Contains(IncludeNameContains, ESearchCase::IgnoreCase) && !BoneNameStr.Contains(ExcludeNameContains, ESearchCase::IgnoreCase))
		{
			FootArray.Add(ChildKey);
			
			FVector LockedFootLocationElementTranslation = Hierarchy->GetGlobalTransform(ChildKey).GetTranslation() + FVector(0.0f, 0.0f, -13.5f);
			FTransform LockedFootLocationElement;
			LockedFootLocationElement.SetTranslation(LockedFootLocationElementTranslation);
			LockedFootLocationArray.Add(LockedFootLocationElement);

			IsFootLockedArray.Add(false);
			
		}
		
	}

}

FRigUnit_OffsetPelvis_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	URigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (!Hierarchy)
	{
		return;
	}
	
	FRigElementKey Item = FRigElementKey(PelvisName, PelvisType);
	FTransform PelvisTransform = Hierarchy->GetGlobalTransform(Item);
	// // Z轴偏移：Sin(Time * Speed) * Amplitude
	// AccumulatedTime += ExecuteContext.GetDeltaTime();
	// ZOffset = FMath::Sin(AccumulatedTime * Speed) * Amplitude;

	// 应用位置偏移到PelvisTransform
	PelvisTransform.AddToTranslation(FVector(0,0,ZOffset));

	// 设置回骨骼
	Hierarchy->SetGlobalTransform(Item, PelvisTransform);
}


FRigUnit_GetFinalLegIKAxisData_Execute()
{
	//右脚的骨骼朝向是反的，因此Index不为0时需要乘以的是-1
	const float Sign = (LegIndex == 0) ? 1.0f : -1.0f;
	PrimaryAxis = FVector(-1, 0, 0) * Sign;
	SecondaryAxis = FVector(0, 1, 0) * Sign;
}


