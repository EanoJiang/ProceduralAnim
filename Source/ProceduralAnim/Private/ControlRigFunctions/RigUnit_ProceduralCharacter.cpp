#include "ControlRigFunctions/RigUnit_ProceduralCharacter.h"
#include "CoreMinimal.h"
#include "ControlRig/Public/Rigs/RigHierarchy.h"
#include "AnimationCoreLibrary.h"
#include "RigVMFunctions/Animation/RigVMFunction_GetDeltaTime.h"
#include "RigVMFunctions/Math/RigVMFunction_MathQuaternion.h"
#include "RigVMFunctions/Math/RigVMFunction_MathVector.h"
#include "RigVMFunctions/Math/RigVMMathLibrary.h"
#include "Transform/TransformableHandleUtils.h"

#pragma region SetupArray
FRigUnit_SetupArray_Execute()
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
	HandArray.Reset();

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
		
		if (BoneNameStr.Contains(TEXT("foot"), ESearchCase::IgnoreCase) && !BoneNameStr.Contains(TEXT("ik"), ESearchCase::IgnoreCase))
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
		else if (BoneNameStr.Contains(TEXT("hand"), ESearchCase::IgnoreCase) && !BoneNameStr.Contains(TEXT("ik"), ESearchCase::IgnoreCase))
		{
			HandArray.Add(ChildKey);
		}
	}
}
#pragma endregion

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
	
		FVector LerpedVector = VectorLerpIndependentOnFrameRate(
			AnimationCore::EulerFromQuat(MovementAngleOffset),
			FVector(0,0,FootTargetZAngle),
			MaxDelVectorLengthPerSecond,
			ExecuteContext.GetDeltaTime<float>()
			);
	
		MovementAngleOffset = AnimationCore::QuatFromEuler(FVector(0,0,LerpedVector.Z));
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

#pragma region 消除帧率差异的用于Vector的Lerp函数
FRigUnit_VectorLerpIndependentOnFrameRate_Execute()
{
	LerpedVector = VectorLerpIndependentOnFrameRate(
		InVector,
		TargetVector,
		MaxDelVectorLengthPerSecond,
		ExecuteContext.GetDeltaTime<float>());
}

FVector VectorLerpIndependentOnFrameRate(FVector InVector, FVector TargetVector, float MaxDelVectorLengthPerSecond, float DeltaTime)
{
	FVector DeltaVector = MathVectorClampLength(TargetVector - InVector, 0,MaxDelVectorLengthPerSecond * DeltaTime);
	return InVector + DeltaVector;
}

FVector MathVectorClampLength(FVector Value, float MinimumLength, float MaximumLength)
{
	if (Value.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}
	float Length = static_cast<float>(Value.Size());
	return Value * FMath::Clamp<float>(Length, MinimumLength, MaximumLength) / Length;
}
#pragma endregion

#pragma region 计算ArmMotion的主次轴朝向数据
	FRigUnit_GetArmMotionAxisData_Execute()
	{
		//右骨骼朝向是反的，因此Index不为0时需要反向
		PrimaryAxis   = (ArmIndex == 0) ? FVector(1, 0, 0) : FVector(-1, 0, 0);
		SecondaryAxis = (ArmIndex == 0) ? FVector(0, -1, 0) : FVector(0, 1, 0);
	}
#pragma endregion

#pragma region 计算ArmMotion的Effector的RotationAmount值
	FRigUnit_GetArmMotionEffectorRotationAmount_Execute()
	{
		const float PerFootCyclePercent = PerFootCyclePercentArray[ArmIndex];
		//摆动的正负号(向后摆动时需要乘-1)
		const float ArmSwingSign = FVector::DotProduct(
			RigSpaceVelocity.GetSafeNormal(),
			MovementAngleOffset.RotateVector(FVector(0,1,0) )
			);
		//向后摆动时的幅度小一些
		const float ArmSwingSignClamp = FMath::Clamp(ArmSwingSign,-0.5,1);
		//摆动的幅度
		const float ArmSwingAmplitude = MathFloatRemap(
										RigSpaceVelocity.Length(),
										0,
										500,
										0,
										20,
										true
										);
		//摆动的中轴向前偏移量
		const float ArmSwingAxisOffset = MathFloatRemap(
										RigSpaceVelocity.Length(),
										0,
										500,
										0,
										15,
										true
										);
		//Sign * ArmSwingAmplitude * sin( 2Π * (PerFootCyclePercent+0.15)%1 ) + ArmSwingAxisOffset
		const float ArmSwingCurve = sin(2 * UE_PI * FMath::Fmod(PerFootCyclePercent + 0.15f, 1.0f))
									* ArmSwingSignClamp
									* ArmSwingAmplitude
									+ ArmSwingAxisOffset;
		
		RotateAmount = AnimationCore::QuatFromEuler(FVector(ArmSwingCurve, 0, 0));
	}

	float MathFloatRemap(float Value, float SourceMinimum, float SourceMaximum, float TargetMinimum, float TargetMaximum, bool bClamp)
	{
		float Result = 0.f;
		float Ratio = 0.f;
		if (FMath::IsNearlyEqual(SourceMinimum, SourceMaximum))
		{
			Ratio = 0.f;
		}
		else
		{
			Ratio = (Value - SourceMinimum) / (SourceMaximum - SourceMinimum);
		}
		if (bClamp)
		{
			Ratio = FMath::Clamp<float>(Ratio, 0.f, 1.f);
		}
		Result = FMath::Lerp<float>(TargetMinimum, TargetMaximum, Ratio);
		return Result;
	}
#pragma endregion

#pragma region ArmMotion时给Hand加一个基于移动速度的Z轴偏移量
	FRigUnit_AddHandZOffset_Execute()
	{
		const float ZOffset = MathFloatRemap(
			RigSpaceVelocity.Length(),
			200,
			500,
			0,
			12,
			true
			);
		const FVector Offset = FVector(0, 0, ZOffset);
		OutTranslation = InTranslation + Offset;
	}
#pragma endregion


#pragma region 计算肩膀的晃动偏移
FRigUnit_GetClavicleOffset_Execute()
{
	const float ClavicleZOffset = sin(2 * PI * 2 * (MasterCyclePercent-0.25) )
									* MathFloatRemap(
										RigSpaceVelocity.Length(),
										0,
										500,
										0,
										2,
										true
										) ;
	ClavicleOffset = FVector(0, 0, ClavicleZOffset);
}
#pragma endregion


#pragma region 绕着旋转点旋转
	FRigUnit_RotateAroundPoint_Execute()
	{
		ModifiedTransform = RotateAroundPoint(TransformToRotate, PointToRotateAround, RotateAmount);
	}

	FTransform RotateAroundPoint(FTransform TransformToRotate, FVector PointToRotateAround, FQuat RotateAmount)
	{
		FTransform ModifiedTransform;
		
		FVector OutTranslation = RotateAmount.RotateVector(TransformToRotate.GetTranslation()-PointToRotateAround) + PointToRotateAround;
		//旋转量 * 待旋转的Transform的当前Rotation
		FQuat OutRotation = RotateAmount * TransformToRotate.GetRotation();
		
		ModifiedTransform.SetTranslation(OutTranslation);
		ModifiedTransform.SetRotation(OutRotation);
		ModifiedTransform.SetScale3D(TransformToRotate.GetScale3D());
		
		return ModifiedTransform;
	}
#pragma endregion


#pragma region 身体倾斜
FRigUnit_PelvisLean_Execute()
{
	URigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if(!Hierarchy)
	{
		return;
	}

	FRigElementKey PelvisRig = FRigElementKey(TEXT("pelvis"), ERigElementType::Bone);
	FTransform TransformToRotate = Hierarchy->GetGlobalTransform(PelvisRig);
	FVector PointToRotateAround = TransformToRotate.GetTranslation();

	float LeanRotateAmount = MathFloatRemap(
		RigSpaceVelocity.Length(),
		0,
		500,
		0,
		-10,
		true
		);
	float RigSpaceVelocityYProjection = RigSpaceVelocity.GetSafeNormal().Dot(FVector::UnitY());
	float LeanRotateAmountAroundX = LeanRotateAmount * RigSpaceVelocityYProjection;
	//基于速度的前后旋转量(绕x轴)
	FQuat RotateAmount = AnimationCore::QuatFromEuler(FVector(LeanRotateAmountAroundX, 0, 0));
	//Pelvis自旋转后的Transform
	FTransform ModifiedTransform = RotateAroundPoint(TransformToRotate, PointToRotateAround, RotateAmount);

	
	float LeanOffsetAmount = MathFloatRemap(
		RigSpaceVelocity.Length(),
		0,
		500,
		0,
		10,
		true
		);
	// 基于速度的前后位置偏移量(y轴)
	float LeanOffsetAmountOnY =	LeanOffsetAmount * RigSpaceVelocityYProjection;
	ModifiedTransform.AddToTranslation(FVector(0, LeanOffsetAmountOnY, 0));
	
	//最终倾斜后的Pelvis
	FTransform FinalPelvis;
	FinalPelvis.SetRotation(ModifiedTransform.GetRotation());
	FinalPelvis.SetTranslation(ModifiedTransform.GetTranslation());
	FinalPelvis.SetScale3D(ModifiedTransform.GetScale3D());
	
	Hierarchy->SetGlobalTransform(PelvisRig, FinalPelvis);
}
#pragma endregion
