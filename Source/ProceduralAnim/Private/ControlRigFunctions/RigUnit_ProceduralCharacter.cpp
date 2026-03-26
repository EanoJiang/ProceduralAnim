#include "ControlRigFunctions/RigUnit_ProceduralCharacter.h"
#include "CoreMinimal.h"
#include "ControlRig/Public/Rigs/RigHierarchy.h"
#include "AnimationCoreLibrary.h"
#include "VectorTypes.h"
#include "Dataflow/DataflowMathNodes.h"
#include "RigVMFunctions/Animation/RigVMFunction_GetDeltaTime.h"
#include "RigVMFunctions/Math/RigVMFunction_MathQuaternion.h"
#include "RigVMFunctions/Math/RigVMFunction_MathVector.h"
#include "RigVMFunctions/Math/RigVMMathLibrary.h"
#include "Transform/TransformableHandleUtils.h"
#include "Units/Deprecated/Math/RigUnit_Float.h"

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
	DefaultFeetPoleVectorArray.Reset();

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

			DefaultFeetPoleVectorArray.Add(FVector());
		}
		else if (BoneNameStr.Contains(TEXT("hand"), ESearchCase::IgnoreCase) && !BoneNameStr.Contains(TEXT("ik"), ESearchCase::IgnoreCase))
		{
			HandArray.Add(ChildKey);
		}
	}
}
#pragma endregion

#pragma region 盆骨朝向
	FRigUnit_CalculatePelvisRotation_Execute()
	{
		//比较哪个脚在前：脚的位置在移动方向上的投影值
		float FootProjectionOnMoveDir = FVector::DotProduct(
			SavedFootPlatformArray[0].GetTranslation() - SavedFootPlatformArray[1].GetTranslation(),
			MovementAngleOffset.RotateVector(FVector::UnitY())
			);
		//绕z轴的旋转量 = 速度映射 × 脚的位置在移动方向上的投影值
		float RotationAroundZAxis = MathFloatRemap(
			RigSpaceVelocity.Length(),
			100,
			300,
			0,
			0.2,
			true
			) * FootProjectionOnMoveDir;
		Result = AnimationCore::QuatFromEuler(FVector(0,0,RotationAroundZAxis));
	}
#pragma endregion

#pragma region 盆骨上下起伏偏移量
	FRigUnit_AddPelvisZOffset_Execute()
	{
		//ZOffset = 速度映射 * sin(2Π * 2 * MasterCyclePercent)
		float ZOffset = MathFloatRemap(
			RigSpaceVelocity.Length(),
			0,
			300,
			0,
			5,
			true)
			* sin(2 * PI * MasterCyclePercent * 2)
			+ PreviousZTraceOffset;
		Translation += FVector(0,0,ZOffset);
	}
#pragma endregion

#pragma region 身体前后倾斜
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
			300,
			0,
			-15,
			true
			);
		float RigSpaceVelocityYProjection = RigSpaceVelocity.GetSafeNormal().Dot(FVector::UnitY());
		float LeanRotateAmountAroundXAxis = LeanRotateAmount * RigSpaceVelocityYProjection;
		//基于速度的前后旋转量(绕x轴)
		FQuat RotateAmount = AnimationCore::QuatFromEuler(FVector(LeanRotateAmountAroundXAxis, 0, 0));
		//Pelvis自旋转后的Transform
		FTransform ModifiedTransform = RotateAroundPoint(TransformToRotate, PointToRotateAround, RotateAmount);

		
		float LeanOffsetAmount = MathFloatRemap(
			RigSpaceVelocity.Length(),
			0,
			300,
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

#pragma region 盆骨侧倾
	FRigUnit_PelvisSideLean_Execute()
	{
		URigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
		if(!Hierarchy)
		{
			return;
		}
		
		//脚踩高度 = 脚部位置在Z轴的投影值
		float FootPlatformHeight = FVector::DotProduct(
			SavedFootPlatformArray[0].GetTranslation() - SavedFootPlatformArray[1].GetTranslation(),
			FVector::UnitZ()
			);
		//基于不同脚部高度的盆骨侧倾旋转量：绕着y轴旋转
		float PelvisSideLeanRotateValue= MathFloatRemap(
			FootPlatformHeight,
			-60,
			60,
			-15,
			15,
			true
			);
		FQuat PelvisSideLeanRotateAmount = AnimationCore::QuatFromEuler( FVector(0, PelvisSideLeanRotateValue, 0) );

		//盆骨绕着y轴自旋转
		FRigElementKey PelvisRig = FRigElementKey(TEXT("pelvis"), ERigElementType::Bone);
		FTransform TransformToRotate = Hierarchy->GetGlobalTransform(PelvisRig);
		FVector PointToRotateAround = TransformToRotate.GetTranslation();
		FTransform ModifiedTransform = RotateAroundPoint(TransformToRotate, PointToRotateAround, PelvisSideLeanRotateAmount);
		Hierarchy->SetGlobalTransform(PelvisRig, ModifiedTransform);

		OutPelvisTiltRotateAmount = PelvisSideLeanRotateAmount;
	}
#pragma endregion

#pragma region 身体绕着Z轴旋转：跟随脚部的旋转而自旋转
FRigUnit_PelvisRotateAroundZAxis_Execute()
{
	URigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if(!Hierarchy)
	{
		return;
	}
	
	//双脚平均旋转量：绕z轴
	FQuat FootAverageRotation = FQuat::Slerp(
		SavedFootPlatformArray[0].GetRotation(),
		SavedFootPlatformArray[1].GetRotation(),
		0.5);
	float FootAverageRotationAroundZAxis = AnimationCore::EulerFromQuat(FootAverageRotation).Z;
	//盆骨绕着Z轴的旋转量 = 双脚的平均旋转
	FQuat PelvisRotateAmount = AnimationCore::QuatFromEuler( FVector(0,0,FootAverageRotationAroundZAxis) );
	
	//盆骨绕着Z轴自旋转
	FRigElementKey PelvisRig = FRigElementKey(TEXT("pelvis"), ERigElementType::Bone);
	FTransform TransformToRotate = Hierarchy->GetGlobalTransform(PelvisRig);
	FVector PointToRotateAround = TransformToRotate.GetTranslation();
	FTransform ModifiedTransform = RotateAroundPoint(TransformToRotate, PointToRotateAround, PelvisRotateAmount);
	Hierarchy->SetGlobalTransform(PelvisRig, ModifiedTransform);

	OutPelvisRotationOffset = PelvisRotateAmount;
}
#pragma endregion

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
	
		//TargetZAngle
		float TargetZAngle = OriginalZAngle;
		if (OriginalZAngle > 110)
		{
			TargetZAngle = OriginalZAngle - 180;
		}
		else if (OriginalZAngle < -110)
		{
			TargetZAngle = OriginalZAngle + 180;
		}
	
		//Lerp速度
		float LerpSpeed;
		bool IsBigAngleOffset = abs(TargetZAngle - AnimationCore::EulerFromQuat(MovementAngleOffset).Z) > 90;
		bool IsInStartMoment = FMath::Modulo(MasterCyclePercent * 2, 1) < 0.4;
		if (IsBigAngleOffset && !IsInStartMoment)
		{
			LerpSpeed = 0.2;
		}
		else
		{
			LerpSpeed = 6;
		}
	
		FVector LerpedVector = VectorLerpIndependentOnFrameRate(
			AnimationCore::EulerFromQuat(MovementAngleOffset),
			FVector(0,0,TargetZAngle),
			LerpSpeed,
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
		BlendSpeed,
		ExecuteContext.GetDeltaTime<float>());
}

FVector VectorLerpIndependentOnFrameRate(FVector InVector, FVector TargetVector, float BlendSpeed, float DeltaTime)
{
	const float LerpFactor = FMath::Clamp<float>(BlendSpeed * DeltaTime, 0, 1);
	FVector DeltaVector = (TargetVector - InVector) * LerpFactor;
	return InVector + DeltaVector;
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
			MathQuaternionScale(MovementAngleOffset, 0.4).RotateVector(FVector(0,1,0) )
			);
		//向后摆动时的幅度小一些
		const float ArmSwingSignClamp = FMath::Clamp(ArmSwingSign,-0.5,1);
		//摆动的幅度
		const float ArmSwingAmplitude = MathFloatRemap(
										RigSpaceVelocity.Length(),
										0,
										300,
										0,
										50,
										true
										)
										* FMath::Clamp(FMath::Abs(RigSpaceVelocity.Dot(FVector(0,0.4,0))), 0.4, 1);
		//摆动的中轴向前偏移量
		const float ArmSwingAxisOffset = MathFloatRemap(
										RigSpaceVelocity.Length(),
										0,
										300,
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

	FQuat MathQuaternionScale(FQuat Value, float Scale)
	{
		FVector Axis = FVector::ZeroVector;
		float Angle = 0.f;
		Value.ToAxisAndAngle(Axis, Angle);
		Value = FQuat(Axis, Angle * Scale);
		return Value;
	}
#pragma endregion

#pragma region ArmMotion时给Hand加一个基于移动速度的Z轴偏移量
	FRigUnit_AddHandZOffset_Execute()
	{
		const float ZOffset = MathFloatRemap(
			RigSpaceVelocity.Length(),
			0,
			300,
			0,
			12,
			true
			);
		const FVector Offset = FVector(0, 0, ZOffset);
		OutTranslation = InTranslation + Offset;
	}
#pragma endregion


#pragma region 计算肩膀的晃动偏移量
FRigUnit_GetClavicleZOffset_Execute()
{
	ClavicleZOffset = sin(2 * PI * 2 * (MasterCyclePercent-0.25) )
									* MathFloatRemap(
										RigSpaceVelocity.Length(),
										0,
										300,
										0,
										1,
										true
										) ;
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


#pragma region 计算每个脚的RotationFactor
FRigUnit_CalculatePerFootRotationFactor_Execute()
{
	const float ZAngle = AnimationCore::EulerFromQuat(MovementAngleOffset).Z;
	if (FootIndex == 0)
	{
		//每当左脚向右转：说明这时候是左脚在前的右向移动，让此时的FootRotationFactor = 0，也就是前腿不旋转
		FootRotationFactor = (ZAngle > 0) ? 0.5 : 0.9;
	}
	else if (FootIndex == 1)
	{
		//每当右脚向左转：说明这时候是右脚在前的左向移动，让此时的FootRotationFactor = 0
		FootRotationFactor = (ZAngle > 0) ? 0.5 : 0.9;
	}
	else
	{
		FootRotationFactor = 0.9;
	}
}
#pragma endregion


#pragma region 避免脚部交叉
FRigUnit_FootAvoidance_Execute()
{
	//前后移动方向矢量
	FVector MoveAngleVector = MovementAngleOffset.RotateVector(FVector::UnitY());
	
	//指向身体两侧的矢量
	FVector BodySideVector = AnimationCore::QuatFromEuler(FVector(0,0,90)).RotateVector(MoveAngleVector);


	//对侧脚踩位置相距身体两侧偏移多少
	float OppositeFootBodySideOffset = SavedFootPlatformArray[(FootIndex==0)? 1: 0].GetTranslation().Dot(BodySideVector);
	//左脚需要偏移的量
	float LeftFootOffset = FMath::Min(IdeaLocation.Dot(BodySideVector), OppositeFootBodySideOffset-15);
	//右脚需要偏移的量
	float RightFootOffset = FMath::Max(IdeaLocation.Dot(BodySideVector), OppositeFootBodySideOffset+15);
	//身体两侧方向上需要避开多少
	FVector BodySideAvoidOffset = BodySideVector*( (FootIndex==0)? LeftFootOffset: RightFootOffset );

	//移动方向上需要避开多少
	FVector MoveAngleAvoidOffset = MoveAngleVector * ( IdeaLocation.Dot(MoveAngleVector) );

	//最终输出的位置
	ModifiedLocation.X = (BodySideAvoidOffset+MoveAngleAvoidOffset).X;
	ModifiedLocation.Y = (BodySideAvoidOffset+MoveAngleAvoidOffset).Y;
	ModifiedLocation.Z = IdeaLocation.Z;
}	
#pragma endregion


#pragma region 脚部的Z轴旋转受移动角度偏移限制，也就是左右旋转限制
//脚部的Z轴旋转受移动角度偏移限制，也就是左右旋转限制
FRigUnit_LimitRotationAroundZ_Execute()
{
	float MovementAngleAroundZAxis = AnimationCore::EulerFromQuat(MovementAngleOffset * FootRotationFactor).Z;
	FVector LimitedRotationVector;
	LimitedRotationVector.X = AnimationCore::EulerFromQuat(InRotation).X;
	LimitedRotationVector.Y = AnimationCore::EulerFromQuat(InRotation).Y;
	LimitedRotationVector.Z = FMath::Clamp(
										AnimationCore::EulerFromQuat(InRotation).Z,
										MovementAngleAroundZAxis - 25,
										MovementAngleAroundZAxis + 25
										);
	LimitedRotation = AnimationCore::QuatFromEuler(LimitedRotationVector);
}
#pragma endregion
