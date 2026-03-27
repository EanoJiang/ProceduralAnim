#pragma once

#include "CoreMinimal.h"
#include "Units/Execution/RigUnit_DynamicHierarchy.h"
#include "ControlRig.h"
#include "Rigs/RigHierarchy.h"
#include "RigUnit_ProceduralCharacter.generated.h"

#pragma region 初始化Array
	//初始化Array
	USTRUCT(meta = (DisplayName = "SetupArray"), Category = "ConstructionEvent")
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

		UPROPERTY(meta=(Output))
		TArray<FVector> DefaultKneeVectorArray;
	};
#pragma endregion

#pragma region 盆骨朝向
	//盆骨朝向：左脚在前顺时针旋转，右脚在前逆时针旋转
	USTRUCT(meta = (DisplayName = "CalculatePelvisRotation"), Category = "OffsetPelvis")
	struct PROCEDURALANIM_API FRigUnit_CalculatePelvisRotation : public FRigUnit
	{
		GENERATED_BODY()
		
		RIGVM_METHOD()
		virtual void Execute() override;

		UPROPERTY(meta = (Input))
		FVector RigSpaceVelocity;
		
		UPROPERTY(meta = (Input))
		TArray<FTransform> SavedFootPlatformArray;

		UPROPERTY(meta = (Input))
		FQuat MovementAngleOffset;
		
		UPROPERTY(meta = (Output))
		FQuat Result;
		
	};
#pragma endregion

#pragma region 盆骨上下起伏偏移量
	//盆骨上下起伏偏移量
	USTRUCT(meta = (DisplayName = "AddPelvisZOffset"), Category = "OffsetPelvis")
	struct PROCEDURALANIM_API FRigUnit_AddPelvisZOffset : public FRigUnit
	{
		GENERATED_BODY()
		
		RIGVM_METHOD()
		virtual void Execute() override;

		UPROPERTY(meta = (Input, Output))
		FVector Translation;
		
		UPROPERTY(meta = (Input))
		float MasterCyclePercent;

		UPROPERTY(meta = (Input))
		FVector RigSpaceVelocity;

		UPROPERTY(meta = (Input))
		float PreviousZTraceOffset;
	};
#pragma endregion

#pragma region 身体前后倾斜
	//身体前后倾斜
	USTRUCT(meta = (DisplayName = "PelvisLean"), Category = "OffsetPelvis")
	struct PROCEDURALANIM_API FRigUnit_PelvisLean : public FRigUnit_DynamicHierarchyBaseMutable
	{
		GENERATED_BODY()

		RIGVM_METHOD()
		virtual void Execute() override;

		UPROPERTY(meta = (Input))
		FVector RigSpaceVelocity;
				
	};
#pragma endregion

#pragma region 盆骨侧倾
	//盆骨侧倾
	USTRUCT(meta = (DisplayName = "PelvisSideLean"), Category = "OffsetPelvis")
	struct PROCEDURALANIM_API FRigUnit_PelvisSideLean : public FRigUnit_DynamicHierarchyBaseMutable
	{
		GENERATED_BODY()

		RIGVM_METHOD()
		virtual void Execute() override;

		UPROPERTY(meta = (Input))
		TArray<FTransform> SavedFootPlatformArray;

		UPROPERTY(meta = (Output))
		FQuat OutPelvisTiltRotateAmount;
	};
#pragma endregion

#pragma region 身体绕着Z轴旋转：跟随脚部的旋转而自旋转
	//身体绕着Z轴旋转：跟随脚部的旋转而自旋转
	USTRUCT(meta = (DisplayName = "PelvisRotateAroundZAxis"), Category = "OffsetPelvis")
	struct PROCEDURALANIM_API FRigUnit_PelvisRotateAroundZAxis : public FRigUnit_DynamicHierarchyBaseMutable
	{
		GENERATED_BODY()

		RIGVM_METHOD()
		virtual void Execute() override;

		UPROPERTY(meta = (Input))
		TArray<FTransform> SavedFootPlatformArray;

		UPROPERTY(meta = (Output))
		FQuat OutPelvisRotationOffset;
	};
#pragma endregion

#pragma region 计算FinalLegIK的主次轴朝向数据
	//计算FinalLegIK的主次轴朝向数据
	USTRUCT(meta = (DisplayName = "CalculateFinalLegIKAxisData"), Category = "FinalLegIK")
	struct PROCEDURALANIM_API FRigUnit_CalculateFinalLegIKAxisData : public FRigUnit
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
#pragma endregion

#pragma region 计算移动角度偏移
	//计算移动角度偏移
	USTRUCT(meta = (DisplayName = "GetMovementAngleOffset"), Category = "CalculateVelocity")
	struct PROCEDURALANIM_API FRigUnit_GetMovementAngleOffset : public FRigUnit
	{
		GENERATED_BODY()

		RIGVM_METHOD()
		virtual void Execute() override;

		UPROPERTY(meta = (Input))
		FVector RigSpaceVelocity;

		UPROPERTY(meta = (Input))
		float MasterCyclePercent;

		UPROPERTY(meta = (Output))
		FQuat MovementAngleOffset;
	};

	FQuat FromTwoVectors(const FVector& A, const FVector& B);

#pragma endregion

#pragma region 消除帧率差异的用于Vector的Lerp函数
	//消除帧率差异的用于Vector的Lerp函数
	USTRUCT(meta = (DisplayName = "VectorLerp"), Category = "CalculateVelocity")
	struct PROCEDURALANIM_API FRigUnit_VectorLerpIndependentOnFrameRate : public FRigUnit
	{
		GENERATED_BODY()

		FRigUnit_VectorLerpIndependentOnFrameRate()
		{
			LerpedVector = TargetVector = InVector = FVector(1.f, 0.f, 0.f);
			BlendSpeed = 0.f;
		}
		RIGVM_METHOD()
		virtual void Execute() override;

		UPROPERTY(meta = (Input))
		FVector InVector;
		
		UPROPERTY(meta = (Input))
		FVector TargetVector;
		
		UPROPERTY(meta = (Input))
		float BlendSpeed = 0;
		
		UPROPERTY(meta = (Output))
		FVector LerpedVector;
	};

	FVector VectorLerpIndependentOnFrameRate(FVector InVector, FVector TargetVector, float BlendSpeed = 0, float DeltaTime = 0);

#pragma endregion

#pragma region 计算FootEffector
	//计算FootEffector
	USTRUCT(meta = (DisplayName = "CalculateFootEffector"), Category = "ArmMotion")
	struct PROCEDURALANIM_API FRigUnit_CalculateFootEffector : public FRigUnit
	{
		GENERATED_BODY()

		RIGVM_METHOD()
		virtual void Execute() override;

		UPROPERTY(meta = (Input))
		FRigElementKey FootRig;

		UPROPERTY(meta = (Input))
		FRigElementKey CalfRig;

		UPROPERTY(meta = (Input))
		FRigElementKey ThighRig;

		UPROPERTY(meta = (Output))
		FTransform OutFootEffector;
	};
#pragma endregion

#pragma region 计算基于移动角度偏移的膝盖朝向向量KneeVector
	//计算基于移动角度偏移的膝盖朝向向量KneeVector
	USTRUCT(meta = (DisplayName = "CalculateKneeVector"), Category = "ArmMotion")
	struct PROCEDURALANIM_API FRigUnit_CalculateKneeVector : public FRigUnit
	{
		GENERATED_BODY()

		RIGVM_METHOD()
		virtual void Execute() override;

		UPROPERTY(meta = (Input))
		TArray<FTransform> SavedFootPlatformArray;
		
		UPROPERTY(meta = (Input))
		int FootIndex;

		UPROPERTY(meta = (Input))
		FQuat MovementAngleOffset;

		UPROPERTY(meta = (Input))
		TArray<FVector> DefaultKneeVectorArray;

		UPROPERTY(meta = (Output))
		FVector OutKneeVector;
	};
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

	FQuat MathQuaternionScale(FQuat Value, float Scale);

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
USTRUCT(meta = (DisplayName = "GetClavicleZOffset"), Category = "ArmMotion")
struct PROCEDURALANIM_API FRigUnit_GetClavicleZOffset : public FRigUnit
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;
		
	UPROPERTY(meta = (Input))
	FVector RigSpaceVelocity;

	UPROPERTY(meta = (Input))
	float MasterCyclePercent;

	UPROPERTY(meta = (Output))
	float ClavicleZOffset;
};
#pragma endregion


#pragma region 绕着旋转点旋转
	//绕着旋转点旋转
	USTRUCT(meta = (DisplayName = "RotateAroundPoint"), Category = "RotationTools")
	struct PROCEDURALANIM_API FRigUnit_RotateAroundPoint : public FRigUnit
	{
		GENERATED_BODY()

		RIGVM_METHOD()
		virtual void Execute() override;
			
		UPROPERTY(meta = (Input))
		FTransform TransformToRotate;

		UPROPERTY(meta = (Input))
		FVector PointToRotateAround;

		UPROPERTY(meta = (Input))
		FQuat RotateAmount;

		UPROPERTY(meta = (Output))
		FTransform ModifiedTransform;
		
	};
	//绕着旋转点旋转
	FTransform RotateAroundPoint(FTransform TransformToRotate, FVector PointToRotateAround, FQuat RotateAmount);
#pragma endregion


#pragma region 计算每个脚的RotationFactor
//计算每个脚的RotationFactor
USTRUCT(meta = (DisplayName = "CalculatePerFootRotationFactor"), Category = "FootRotation")
struct PROCEDURALANIM_API FRigUnit_CalculatePerFootRotationFactor : public FRigUnit
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FQuat MovementAngleOffset;

	UPROPERTY(meta = (Input))
	int FootIndex;

	UPROPERTY(meta = (Output))
	float FootRotationFactor;
};
#pragma endregion


#pragma region 避免脚部交叉
	//避免脚部交叉
	USTRUCT(meta = (DisplayName = "FootAvoidance"), Category = "CalculateFootTargetTransform")
	struct PROCEDURALANIM_API FRigUnit_FootAvoidance : public FRigUnit
	{
		GENERATED_BODY()

		RIGVM_METHOD()
		virtual void Execute() override;
		
		UPROPERTY(meta = (Input))
		FVector IdeaLocation;
		UPROPERTY(meta = (Input))
		int FootIndex;
		UPROPERTY(meta = (Input))
		FQuat MovementAngleOffset;
		UPROPERTY(meta = (Input))
		TArray<FTransform> SavedFootPlatformArray;

		UPROPERTY(meta = (Output))
		FVector ModifiedLocation;
	};
#pragma endregion

#pragma region 脚部的Z轴旋转受移动角度偏移限制，也就是左右旋转限制
//脚部的Z轴旋转受移动角度偏移限制，也就是左右旋转限制
USTRUCT(meta = (DisplayName = "LimitRotationAroundZ"), Category = "CalculateFootTargetTransform")
struct PROCEDURALANIM_API FRigUnit_LimitRotationAroundZ : public FRigUnit
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;
		
	UPROPERTY(meta = (Input))
	FQuat InRotation;
	UPROPERTY(meta = (Input))
	FQuat MovementAngleOffset;
	UPROPERTY(meta = (Input))
	float FootRotationFactor;

	UPROPERTY(meta = (Output))
	FQuat LimitedRotation;
};
#pragma endregion
