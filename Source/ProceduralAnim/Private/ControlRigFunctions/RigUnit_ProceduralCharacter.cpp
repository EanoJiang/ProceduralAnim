#include "ControlRigFunctions/RigUnit_ProceduralCharacter.h"
#include "CoreMinimal.h"
#include "ControlRig/Public/Rigs/RigHierarchy.h"
#include "AnimationCoreLibrary.h"
#include "RigVMFunctions/Math/RigVMMathLibrary.h"

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
	PredictFeetLocationArray.Reset();
	PerFootCyclePercentArray.Reset();
	SavedFootPlatformArray.Reset();

	const FRigElementKey RootBoneKey(TEXT("root"), ERigElementType::Bone);
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

			PredictFeetLocationArray.Add(FTransform());
			
			PerFootCyclePercentArray.Add(0);

			SavedFootPlatformArray.Add(FTransform());
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

	/*保存OffsetPelvis之前的脚部Transform*/
	OriginalFootLocationArray.Reset();
	for (const FRigElementKey& FootRig : FootArray)
	{
		OriginalFootLocationArray.Add(Hierarchy->GetGlobalTransform(FootRig));
	}
	
	/*Pelvis偏移*/
	FRigElementKey PelvisRig = FRigElementKey(TEXT("pelvis"), ERigElementType::Bone);
	FTransform PelvisTransform = Hierarchy->GetGlobalTransform(PelvisRig);
	float ZOffset = FMath::GetMappedRangeValueClamped(
		FVector2D(100,500),
		FVector2D(0,5),
		RigSpaceVelocity.Length()
		)* sin(2 * PI * 2 * MasterCyclePercent) - 7;
	// 应用位置偏移到PelvisTransform
	PelvisTransform.AddToTranslation(FVector(0,0,ZOffset));
	// 设置回骨骼
	Hierarchy->SetGlobalTransform(PelvisRig, PelvisTransform);

	/*恢复脚部Transform*/
	for (int i = 0; i < FootArray.Num(); i++)
	{
		Hierarchy->SetGlobalTransform(FootArray[i], OriginalFootLocationArray[i]);
		
	}
}


FRigUnit_GetFinalLegIKAxisData_Execute()
{
	//右脚的骨骼朝向是反的，因此Index不为0时需要乘以的是-1
	const float Sign = (LegIndex == 0) ? 1.0f : -1.0f;
	PrimaryAxis = FVector(-1, 0, 0) * Sign;
	SecondaryAxis = FVector(0, 1, 0) * Sign;
}



#pragma region 计算移动角度偏移
	FRigUnit_GetMovementAngleOffset_Execute()
	{
		const float OriginalZAngle = AnimationCore::EulerFromQuat(
			FromTwoVectors(FVector(0,1,0),RigSpaceVelocity)
			).Z;
		if (OriginalZAngle > 100)
		{
			FootTargetZAngle = OriginalZAngle - 180;
		}
		else if (OriginalZAngle < -100)
		{
			FootTargetZAngle = OriginalZAngle + 180;
		}
		else
		{
			FootTargetZAngle = OriginalZAngle;
		}
		MovementAngleOffset = AnimationCore::QuatFromEuler(FVector(0,0,FootTargetZAngle));
	}

	FQuat FromTwoVectors(const FVector& A, const FVector& B)
	{
		if (A.IsNearlyZero() || B.IsNearlyZero())
		{
			return FQuat::Identity;
		}
		return FRigVMMathLibrary::FindQuatBetweenVectors(A, B);
	}
#pragma endregion




