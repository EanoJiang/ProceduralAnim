#pragma once

#include "CoreMinimal.h"
#include "Units/Execution/RigUnit_DynamicHierarchy.h"
#include "ControlRig.h"
#include "Rigs/RigHierarchy.h"
#include "RigUnit_ProceduralCharacter.generated.h"

//建立FootArray
USTRUCT(meta = (DisplayName = "SetupFootArray"))
struct PROCEDURALANIM_API FRigUnit_SetupFootArray : public FRigUnit_DynamicHierarchyBaseMutable
{
	GENERATED_BODY()
	
	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta=(Input,Output))
	TArray<FRigElementKey> FootArray;


	UPROPERTY()
	FName RootName = TEXT("root");
	
	UPROPERTY(meta = (Input))
	FString IncludeNameContains = TEXT("foot");

	UPROPERTY(meta = (Input))
	FString ExcludeNameContains = TEXT("ik");
	
	UPROPERTY(meta=(Input,Output))
	TArray<FTransform> LockedFootLocationArray;

	UPROPERTY(meta=(Input,Output))
	TArray<bool> IsFootLockedArray;
};

//Pelvis偏移
USTRUCT(meta = (DisplayName = "OffsetPelvis"))
struct PROCEDURALANIM_API FRigUnit_OffsetPelvis : public FRigUnit_DynamicHierarchyBaseMutable
{
	GENERATED_BODY()
	
	RIGVM_METHOD()
	virtual void Execute() override;
	
	UPROPERTY(Transient)
	ERigElementType PelvisType = ERigElementType::Bone;
	
	UPROPERTY(Transient)
	FName PelvisName = TEXT("pelvis");

	//Z轴偏移量
	UPROPERTY(meta = (Input))
	float ZOffset = -30.0f;
	
};

//计算FinalLegIK的主次轴朝向数据
USTRUCT(meta = (DisplayName = "GetFinalLegIKAxisData"))
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


