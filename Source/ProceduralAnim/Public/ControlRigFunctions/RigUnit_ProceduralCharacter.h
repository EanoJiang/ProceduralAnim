#pragma once

#include "CoreMinimal.h"
#include "Units/Execution/RigUnit_DynamicHierarchy.h"
#include "ControlRig.h"
#include "Rigs/RigHierarchy.h"
#include "RigUnit_ProceduralCharacter.generated.h"

#pragma region 初始化Array
USTRUCT(meta = (DisplayName = "SetupArray"))
struct PROCEDURALANIM_API FRigUnit_SetupArray : public FRigUnit_DynamicHierarchyBaseMutable
{
	GENERATED_BODY()
	
	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta=(Output))
	TArray<FRigElementKey> FootArray;
	
	UPROPERTY(meta=(Output))
	TArray<FTransform> LockedFootLocationArray;

	UPROPERTY(meta=(Output))
	TArray<bool> IsFootLockedArray;

	UPROPERTY(meta=(Output))
	TArray<FTransform> PredictFeetLocationArray;

	UPROPERTY(meta=(Output))
	TArray<float> PerFootCyclePercentArray;

	UPROPERTY(meta=(Output))
	TArray<FTransform> SavedFootPlatformArray;

	UPROPERTY(meta=(Output))
	TArray<FRigElementKey> HandArray;
};
#pragma endregion

//Pelvis偏移
USTRUCT(meta = (DisplayName = "OffsetPelvis"))
struct PROCEDURALANIM_API FRigUnit_OffsetPelvis : public FRigUnit_DynamicHierarchyBaseMutable
{
	GENERATED_BODY()
	
	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	TArray<FRigElementKey> FootArray;

	//OffsetPelvis前的脚部位置数组
	UPROPERTY(Transient)
	TArray<FTransform> OriginalFootLocationArray;

	UPROPERTY(meta = (Input))
	float MasterCyclePercent;
	
	UPROPERTY(meta = (Input))
	FVector RigSpaceVelocity;
	
};

//计算FinalLegIK的主次轴朝向数据
USTRUCT(meta = (DisplayName = "GetFinalLegIKAxisData"), Category = "FinalLegIK")
struct PROCEDURALANIM_API FRigUnit_GetFinalLegIKAxisData : public FRigUnit
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	int LegIndex = 0;
	UPROPERTY(meta = (Output))
	FVector PrimaryAxis = FVector(-1, 0, 0) ;
	UPROPERTY(meta = (Output))
	FVector SecondaryAxis = FVector(0, 1, 0);
};


//手动封装的用于Transform的Lerp函数
static FTransform InterpolateTransform(const FTransform& A, const FTransform& B, float Alpha)
{
	FVector InterpLocation = FMath::Lerp(A.GetLocation(), B.GetLocation(), Alpha);
	FQuat InterpRotation = FQuat::Slerp(A.GetRotation(), B.GetRotation(), Alpha);
	FVector InterpScale = FMath::Lerp(A.GetScale3D(), B.GetScale3D(), Alpha);
 
	return FTransform(InterpRotation, InterpLocation, InterpScale);
}

#pragma region 计算移动角度偏移
	//计算移动角度偏移
	USTRUCT(meta = (DisplayName = "GetMovementAngleOffset"), Category = "Calculate")
	struct PROCEDURALANIM_API FRigUnit_GetMovementAngleOffset : public FRigUnit
	{
		GENERATED_BODY()

		RIGVM_METHOD()
		virtual void Execute() override;

		UPROPERTY(meta = (Input))
		FVector RigSpaceVelocity;
		
		UPROPERTY(Transient)
		float FootTargetZAngle;

		UPROPERTY(meta = (Output))
		FQuat MovementAngleOffset;

		UPROPERTY(meta = (Input))
		float MaxDelVectorLengthPerSecond = 360.0f;
	};

	FQuat FromTwoVectors(const FVector& A, const FVector& B);

#pragma endregion

#pragma region 消除帧率差异的用于Vector的Lerp函数
	//消除帧率差异的用于Vector的Lerp函数
	USTRUCT(meta = (DisplayName = "VectorLerp"), Category = "Lerp")
	struct PROCEDURALANIM_API FRigUnit_VectorLerpIndependentOnFrameRate : public FRigUnit
	{
		GENERATED_BODY()

		FRigUnit_VectorLerpIndependentOnFrameRate()
		{
			LerpedVector = TargetVector = InVector = FVector(1.f, 0.f, 0.f);
			MaxDelVectorLengthPerSecond = 0.f;
		}
		RIGVM_METHOD()
		virtual void Execute() override;

		UPROPERTY(meta = (Input))
		FVector InVector;
		
		UPROPERTY(meta = (Input))
		FVector TargetVector;
		
		UPROPERTY(meta = (Input))
		float MaxDelVectorLengthPerSecond = 0;
		
		UPROPERTY(meta = (Output))
		FVector LerpedVector;
	};

	FVector VectorLerpIndependentOnFrameRate(FVector InVector, FVector TargetVector, float MaxDelVectorLengthPerSecond = 0, float DeltaTime = 0);

	FVector MathVectorClampLength(FVector Value = FVector(1.f, 0.f, 0.f), float MinimumLength = 0, float MaximumLength = 1);
#pragma endregion


#pragma region 计算ArmMotion的主次轴朝向数据
	//计算ArmMotion的主次轴朝向数据
	USTRUCT(meta = (DisplayName = "GetArmMotionAxisData"), Category = "ArmMotion")
	struct PROCEDURALANIM_API FRigUnit_GetArmMotionAxisData : public FRigUnit
	{
		GENERATED_BODY()

		RIGVM_METHOD()
		virtual void Execute() override;

		UPROPERTY(meta = (Input))
		int ArmIndex = 0;
		
		UPROPERTY(meta = (Output))
		FVector PrimaryAxis = FVector(1, 0, 0) ;
		
		UPROPERTY(meta = (Output))
		FVector SecondaryAxis = FVector(0, -1, 0);
	};
#pragma endregion


#pragma region 计算ArmMotion的Effector的RotationAmount值
	//计算ArmMotion的Effector的RotationAmount值
	USTRUCT(meta = (DisplayName = "GetArmMotionEffectorRotationAmount"), Category = "ArmMotion")
	struct PROCEDURALANIM_API FRigUnit_GetArmMotionEffectorRotationAmount : public FRigUnit
	{
		GENERATED_BODY()

		RIGVM_METHOD()
		virtual void Execute() override;

		UPROPERTY(meta = (Input))
		TArray<float> PerFootCyclePercentArray;

		UPROPERTY(meta = (Input))
		int ArmIndex;

		UPROPERTY(meta = (Input))
		FVector RigSpaceVelocity;

		UPROPERTY(meta = (Input))
		FQuat MovementAngleOffset;

		UPROPERTY(meta = (Output))
		FQuat RotateAmount;
	};

	float MathFloatRemap(float Value, float SourceMinimum, float SourceMaximum, float TargetMinimum, float TargetMaximum, bool bClamp);

#pragma endregion

#pragma region ArmMotion时给Hand加一个基于移动速度的Z轴偏移量
	//ArmMotion时给Hand加一个基于移动速度的Z轴偏移量
	USTRUCT(meta = (DisplayName = "AddHandZOFfset"), Category = "ArmMotion")
	struct PROCEDURALANIM_API FRigUnit_AddHandZOffset : public FRigUnit
	{
		GENERATED_BODY()

		RIGVM_METHOD()
		virtual void Execute() override;
		
		UPROPERTY(meta = (Input))
		FVector InTranslation;

		UPROPERTY(meta = (Input))
		FVector RigSpaceVelocity;

		UPROPERTY(meta = (Output))
		FVector OutTranslation;
	};
#pragma endregion


#pragma region 计算肩膀的晃动偏移
//计算肩膀的晃动偏移
USTRUCT(meta = (DisplayName = "GetClavicleOffset"), Category = "ArmMotion")
struct PROCEDURALANIM_API FRigUnit_GetClavicleOffset : public FRigUnit
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;
		
	UPROPERTY(meta = (Input))
	FVector RigSpaceVelocity;

	UPROPERTY(meta = (Input))
	float MasterCyclePercent;

	UPROPERTY(meta = (Output))
	FVector ClavicleOffset;
};
#pragma endregion
