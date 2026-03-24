# 白话ControlRig完全程序化角色基础移动

## 初始化要用到的数组

![1774252443658](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260323175904125-300876890.png)

```cpp
#pragma region 初始化Array
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
	TArray<FVector> DefaultFeetPoleVectorArray;
};
#pragma endregion
```

```cpp
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

```

## 骨骼控制链全流程

![1774252747815](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260323175905041-1986037003.png)

### 参数准备

#### CalculateVelocity

##### 世界空间速度WorldSpaceVelocity：

WorldSpaceVelocity = 根骨骼世界位置的前后帧差 / DeltaTime

##### 世界空间前后帧相对变换 WorldDeltaTransform：

WorldDeltaTransform =  `T_now * T_last^(-1)`

也就是先通过逆变换撤销上一帧的世界变换，再与当前帧的世界变换相乘，最终得到 “从上一帧到当前帧，根骨骼在世界空间中发生的相对变换”。

##### 骨骼空间速度RigSpaceVelocity：

RigSpaceVelocity的目标速度 = (**世界空间速度**+根骨骼的世界位置)的局部位置 - 根骨骼的局部位置

再通过 Lerp 平滑后得到RigSpaceVelocity

更慢的RigSpaceVelocity_SlowLerp 就是把BlendSpeed设置更小的值

> VectorLerp函数
>
> ```cpp
> #pragma region 消除帧率差异的用于Vector的Lerp函数
> 	//消除帧率差异的用于Vector的Lerp函数
> 	USTRUCT(meta = (DisplayName = "VectorLerp"), Category = "CalculateVelocity")
> 	struct PROCEDURALANIM_API FRigUnit_VectorLerpIndependentOnFrameRate : public FRigUnit
> 	{
> 		GENERATED_BODY()
>
> 		FRigUnit_VectorLerpIndependentOnFrameRate()
> 		{
> 			LerpedVector = TargetVector = InVector = FVector(1.f, 0.f, 0.f);
> 			BlendSpeed = 0.f;
> 		}
> 		RIGVM_METHOD()
> 		virtual void Execute() override;
>
> 		UPROPERTY(meta = (Input))
> 		FVector InVector;
>
> 		UPROPERTY(meta = (Input))
> 		FVector TargetVector;
>
> 		UPROPERTY(meta = (Input))
> 		float BlendSpeed = 0;
>
> 		UPROPERTY(meta = (Output))
> 		FVector LerpedVector;
> 	};
>
> 	FVector VectorLerpIndependentOnFrameRate(FVector InVector, FVector TargetVector, float BlendSpeed = 0, float DeltaTime = 0);
>
> #pragma endregion
>
> ```
>
> ```cpp
> #pragma region 消除帧率差异的用于Vector的Lerp函数
> FRigUnit_VectorLerpIndependentOnFrameRate_Execute()
> {
> 	LerpedVector = VectorLerpIndependentOnFrameRate(
> 		InVector,
> 		TargetVector,
> 		BlendSpeed,
> 		ExecuteContext.GetDeltaTime<float>());
> }
>
> FVector VectorLerpIndependentOnFrameRate(FVector InVector, FVector TargetVector, float BlendSpeed, float DeltaTime)
> {
> 	const float LerpFactor = FMath::Clamp<float>(BlendSpeed * DeltaTime, 0, 1);
> 	FVector DeltaVector = (TargetVector - InVector) * LerpFactor;
> 	return InVector + DeltaVector;
> }
> #pragma endregion
> ```

##### 最大步幅MaxFootStrideLength

MaxFootStrideLength = `速度[200.0, 300.0]`Remap到 `[65.0, 45.0]`的步长范围

速度越快，最大步长越短；速度越慢，最大步长越长。

##### 移动角度偏移MovementAngleOffset

MovementAngleOffset = GetMovementAngleOffset(移动速度RigSpaceVelocity，步态周期进度MasterCyclePercent)

> GetMovementAngleOffset函数
>
> ```cpp
> #pragma region 计算移动角度偏移
> 	//计算移动角度偏移
> 	USTRUCT(meta = (DisplayName = "GetMovementAngleOffset"), Category = "CalculateVelocity")
> 	struct PROCEDURALANIM_API FRigUnit_GetMovementAngleOffset : public FRigUnit
> 	{
> 		GENERATED_BODY()
>
> 		RIGVM_METHOD()
> 		virtual void Execute() override;
>
> 		UPROPERTY(meta = (Input))
> 		FVector RigSpaceVelocity;
>
> 		UPROPERTY(meta = (Input))
> 		float MasterCyclePercent;
>
> 		UPROPERTY(meta = (Output))
> 		FQuat MovementAngleOffset;
> 	};
>
> 	FQuat FromTwoVectors(const FVector& A, const FVector& B);
>
> #pragma endregion
> ```
>
> ```cpp
> #pragma region 计算移动角度偏移
> 	FRigUnit_GetMovementAngleOffset_Execute()
> 	{
> 		const float OriginalZAngle = AnimationCore::EulerFromQuat(
> 			FromTwoVectors(FVector(0,1,0),RigSpaceVelocity)
> 			).Z;
>
> 		//TargetZAngle
> 		float TargetZAngle = OriginalZAngle;
> 		if (OriginalZAngle > 110)
> 		{
> 			TargetZAngle = OriginalZAngle - 180;
> 		}
> 		else if (OriginalZAngle < -110)
> 		{
> 			TargetZAngle = OriginalZAngle + 180;
> 		}
>
> 		//Lerp速度
> 		float LerpSpeed;
> 		bool IsBigAngleOffset = abs(TargetZAngle - AnimationCore::EulerFromQuat(MovementAngleOffset).Z) > 90;
> 		bool IsInStartMoment = FMath::Modulo(MasterCyclePercent * 2, 1) < 0.4;
> 		if (IsBigAngleOffset && !IsInStartMoment)
> 		{
> 			LerpSpeed = 0.2;
> 		}
> 		else
> 		{
> 			LerpSpeed = 6;
> 		}
>
> 		FVector LerpedVector = VectorLerpIndependentOnFrameRate(
> 			AnimationCore::EulerFromQuat(MovementAngleOffset),
> 			FVector(0,0,TargetZAngle),
> 			LerpSpeed,
> 			ExecuteContext.GetDeltaTime<float>()
> 			);
>
> 		MovementAngleOffset = AnimationCore::QuatFromEuler(FVector(0,0,LerpedVector.Z));
> 	}
>
> 	FQuat FromTwoVectors(const FVector& A, const FVector& B)
> 	{
> 		if (A.IsNearlyZero() || B.IsNearlyZero())
> 		{
> 			return FQuat::Identity;
> 		}
> 		return FRigVMMathLibrary::FindQuatBetweenVectors(A, B);
> 	}
> #pragma endregion
>
> ```

#### CalculateCycle

##### 步态周期长度CycleLength

CycleLength = 空中摆动时间SwingTime + 地面停留时间

地面停留时间 = **最大步幅MaxFootStrideLength** / **Max(速度，速度从[0,200]Remap到[600,200])**

##### 摆动时间占据步态周期长度的百分比SwingTimeAsAPercent

SwingTimeAsAPercent = 空中摆动时间SwingTime / 步态周期长度CycleLength

##### 步态周期百分比MasterCyclePercent

MasterCyclePercent \= ( MasterCyclePercent + DeltaTime/步态周期长度CycleLength ) % 1

最后除模1，把MasterCyclePercent限制在0到1

##### 每只脚的周期进度数组PerFootCyclePercentArray

PerFootCyclePercentArray[FootIndex] = (步态周期百分比**MasterCyclePercent + FootIndex × 0.5**) % 1

循环写入，FootIndex × 0.5是因为双脚之间刚好差半个步态周期

#### 循环遍历

##### 每只脚的FootRig和FootIndex

##### 每只脚的朝向因子FootRotationFactor

FootRotationFactor = CalculatePerFootRotationFactor(MovementAngleOffset,FootIndex)

> CalculatePerFootRotationFactor函数
>
> ```cpp
> #pragma region 计算每个脚的RotationFactor
> //计算每个脚的RotationFactor
> USTRUCT(meta = (DisplayName = "CalculatePerFootRotationFactor"), Category = "FootRotation")
> struct PROCEDURALANIM_API FRigUnit_CalculatePerFootRotationFactor : public FRigUnit
> {
> 	GENERATED_BODY()
>
> 	RIGVM_METHOD()
> 	virtual void Execute() override;
>
> 	UPROPERTY(meta = (Input))
> 	FQuat MovementAngleOffset;
>
> 	UPROPERTY(meta = (Input))
> 	int FootIndex;
>
> 	UPROPERTY(meta = (Output))
> 	float FootRotationFactor;
> };
> #pragma endregion
> ```
>
> ```cpp
> #pragma region 计算每个脚的RotationFactor
> FRigUnit_CalculatePerFootRotationFactor_Execute()
> {
> 	const float ZAngle = AnimationCore::EulerFromQuat(MovementAngleOffset).Z;
> 	if (FootIndex == 0)
> 	{
> 		//每当左脚向右转：说明这时候是左脚在前的右向移动，让此时的FootRotationFactor = 0，也就是前腿不旋转
> 		FootRotationFactor = (ZAngle > 0) ? 0.5 : 0.9;
> 	}
> 	else if (FootIndex == 1)
> 	{
> 		//每当右脚向左转：说明这时候是右脚在前的左向移动，让此时的FootRotationFactor = 0
> 		FootRotationFactor = (ZAngle > 0) ? 0.5 : 0.9;
> 	}
> 	else
> 	{
> 		FootRotationFactor = 0.9;
> 	}
> }
> #pragma endregion
> ```

##### 保存脚默认的极坐标矢量数组DefaultFeetPoleVectorArray

DefaultFeetPoleVectorArray[FootIndex] = 膝盖位置

膝盖位置 = 小腿Calf - 大腿Thigh与脚部Foot的中点

##### 最主要的功能函数节点——RotateAroundPoint

> Transform绕着Transform旋转RotateAmount

```cpp
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
```

```cpp
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
```

##### 控制身体部位的实际功能节点

#### 预测脚部落点PredictFootLandingSpot

##### 预测前进距离PredictFwdDistance

PredictFwdDistance = CalculatePredictFwdDistance()

> CalculatePredictFwdDistance函数：
>
> PredictFwdDistance = RigSpaceVelocity_SlowLerp × 预测脚部即将落地的时间
>
> 预测脚部即将落地的时间 = ( SwingTimeAsAPercent - PerFootCyclePercentArray[FootIndex] ) × CycleLength

##### 基于步幅的预测前进距离PredictFwdDistanceBasedOnStride

> PredictFwdDistanceBasedOnStride函数：
>
> PredictFwdDistanceBasedOnStride = PredictFwdDistance × 移动方向上的步幅
>
> 移动方向上的步幅 = RigSpaceVelocity_SlowLerp× CycleLength/SwingTime

##### 预测脚部位置PredictFeetLocation

PredictFeetLocation = 脚部位置(适配移动角度偏移) + PredictFwdDistanceBasedOnStride

脚部位置(适配移动角度偏移) = RotateAroundPoint(Foot, Pelvis, MovementAngleOffset)

也就是FootRig 绕着 Pelvis 旋转 MovementAngleOffset值

> 最后为了确保脚在地面上，需要SphereTrace：
>
> SphereTrace击中的HitLocation就是最终的PredictFeetLocation

##### 落地点的法向量FootLandHitPointNormal

FootLandHitPointNormal = SphereTrace击中的HitNormal

##### 落地点朝向FootLandingRotation

FootLandingRotation = MovementAngleOffset × FootRotationFactor

##### 多点矩阵Trace检测得到的预测脚部落点PredictFeetLocationAfterTrace

> 防止脚部靠近障碍物时穿模

PredictFeetLocationAfterTrace = RectangleFootLandTraces(FootLandingRotation, PredictFeetLocation, FootIndex)

> RectangleFootLandTraces函数

###### 区分左右脚的偏移方向OffsetDirection

OffsetDirection = (FootIndex?) 1 : -1

###### TraceIndex

两层循环，外循环TraceIndex1，内循环TraceIndex2

###### 每次偏移前后方向(Y轴)需要抵消的量DistanceCenterToBoundary

DistanceCenterToBoundary = 偏移量总长 / 2

偏移量总长 = 每一次循环偏移量OffsetPerLoop × (循环次数-1)

###### 每个点的偏移量长宽XOffset、YOffset

左右XOffset = OffsetDirection × TraceIndex2 × OffsetPerLoop

前后YOffset = TraceIndex1 × OffsetPerLoop - DistanceCenterToBoundary

###### 一次Trace

每个Trace点PerDetectedPoint = FootLandingSpot + FVector(XOffset, YOffset, 0)

PerDetectedPoint经过SphereTrace之后得到 IsFirstTraceHit 和 FirstTraceResult

###### 二次Trace

一次Trace的起止点沿着FootLandingSpot的方向偏移得到二次Trace的起止点，结果得到SecondTraceResult

###### Trace检测后的落点影响因素

高度因素HeightFactor = 


##### 最终的预测脚部落点FinalFootLandingLocation

> 需要抵消预测前进距离

FinalFootLandingLocation = PredictFeetLocationAfterTrace - PredictFwdDistance

##### 适应斜面角度的脚部落点朝向FootLandingRotationBySlope

> 只需要让FootLandingRotation绕x、y轴的旋转量调用AimMath以瞄准落地点的法向量

FootLandingRotationBySlope.XY = AimMath(InputTransform.Rotation:FootLandingRotation,  Secondary.Target:FootLandHitPointNormal).XY

FootLandingRotationBySlope.Z = FootLandingRotation.Z

##### 将Rotation和Location设置为PredictFeetLocationArray数组的目标值

Location设置的时候需要Lerp平滑数值，BlendSpeed = 6
