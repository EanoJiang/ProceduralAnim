# 白话ControlRig完全程序化角色基础移动

> 视频演示：[【UE5】完全程序化的角色基础移动控制器_哔哩哔哩_bilibili](https://www.bilibili.com/video/BV11mQRB8Ez5/?vd_source=5eb26c403edb4b6de737a9c6fad9b1de#reply116277381238713)
>
> 详细教程：[[UE5]完全程序化的角色基础移动控制器 - EanoJiang - 博客园](https://www.cnblogs.com/eanojiang/p/19623752)
>
> [白话ControlRig完全程序化角色基础移动 - EanoJiang - 博客园](https://www.cnblogs.com/eanojiang/p/19759397)

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
	TArray<FVector> DefaultKneeVectorArray;
};
#pragma endregion
```

```cpp
#pragma region 初始化Array
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
	DefaultKneeVectorArray.Reset();

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

			DefaultKneeVectorArray.Add(FVector());
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

##### 保存默认的膝盖朝向向量(脚的极坐标向量)数组SaveDefaultKneeVectorArray

DefaultKneeVectorArray[FootIndex] = 膝盖朝向向量

膝盖朝向向量 = 小腿Calf - 大腿Thigh与脚部Foot的中点

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

### 控制身体部位的实际功能节点

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

> 高度因素HeightFactor：优先选取更高的，也就是数值越小的优先级越高

HeightFactor = Min(FirstTraceResult.Z,SecondTraceResult.Z).Remap(-10,20,1,0.5)

> 偏移量因素OffsetFactor：优先选取偏移量更小的

OffsetFactor = OffsetAmount.Remap(0,20,0,1)

OffsetAmount = √(x^2 + y^2)

> Trace是否击中因素FirstTraceHitFactor

FirstTraceHitFactor = (IsFirstTraceHit)? 0: 1

> 两次Trace结果的高度差因素TraceHeightDeltaFactor

TraceHeightDeltaFactor = Abs(FirstTraceResult.Z - SecondTraceResult.Z)

###### 最终权重如果小于，选择第一次Trace结果为落点

```cpp
if(TraceHeightDeltaFactor + FirstTraceHitFactor + OffsetFactor + HeightFactor < LowestResult)
{
	LowestResult = TraceHeightDeltaFactor + FirstTraceHitFactor + OffsetFactor + HeightFactor;
	TempLandLocation = (IsFirstTraceHit)? FirstTraceResult: SecondTraceResult;
}
```

###### 外循环结束后

Return Translation = TempLandLocation, Rotation = FootLandingSpot.Rotation

##### 最终的预测脚部落点FinalFootLandingLocation

> 需要抵消预测前进距离

FinalFootLandingLocation = PredictFeetLocationAfterTrace - PredictFwdDistance

##### 适应斜面角度的脚部落点朝向FootLandingRotationBySlope

> 只需要让FootLandingRotation绕x、y轴的旋转量调用AimMath以瞄准落地点的法向量

FootLandingRotationBySlope.XY = AimMath(InputTransform.Rotation:FootLandingRotation,  Secondary.Target:FootLandHitPointNormal).XY

FootLandingRotationBySlope.Z = FootLandingRotation.Z

##### 将Rotation和Location设置为PredictFeetLocationArray数组的目标值

Location设置的时候需要Lerp平滑数值，BlendSpeed = 6

#### 计算脚部目标平台CalculateFootTargetPlatform

SavedFootPlatformArray[FootIndex] = CalculateFootTargetPlatform()

##### 如果处于Locked，也就是IsFootLockedArray[FootIndex] == true

###### 抵消前后帧的世界变换，来实现脚部位置的锁定，这也是锁定状态下的目标脚踩位置

Clamp限制前的结果 = LockedFeetLocationArray[FootIndex] × WorldDeltaTransform.Inverse

LockedFeetLocationArray[FootIndex].Translation = Clamp限制前的结果.Translation.ClampSpatially(0,100)

LockedFeetLocationArray[FootIndex].Rotation = LimitRotationAroundZ( Clamp限制前的结果.Rotation )

> LimitRotationAroundZ节点：
>
> ```cpp
> #pragma region 脚部的Z轴旋转受移动角度偏移限制，也就是左右旋转限制
> //脚部的Z轴旋转受移动角度偏移限制，也就是左右旋转限制
> USTRUCT(meta = (DisplayName = "LimitRotationAroundZ"), Category = "CalculateFootTargetTransform")
> struct PROCEDURALANIM_API FRigUnit_LimitRotationAroundZ : public FRigUnit
> {
> 	GENERATED_BODY()
>
> 	RIGVM_METHOD()
> 	virtual void Execute() override;
>
> 	UPROPERTY(meta = (Input))
> 	FQuat InRotation;
> 	UPROPERTY(meta = (Input))
> 	FQuat MovementAngleOffset;
> 	UPROPERTY(meta = (Input))
> 	float FootRotationFactor;
>
> 	UPROPERTY(meta = (Output))
> 	FQuat LimitedRotation;
> };
> #pragma endregion
> ```
>
> ```cpp
> #pragma region 脚部的Z轴旋转受移动角度偏移限制，也就是左右旋转限制
> //脚部的Z轴旋转受移动角度偏移限制，也就是左右旋转限制
> FRigUnit_LimitRotationAroundZ_Execute()
> {
> 	// float MovementAngleAroundZAxis = AnimationCore::EulerFromQuat(MovementAngleOffset * FootRotationFactor).Z;
> 	// Rotation.Z = FMath::Clamp(
> 	// 	Rotation.Z,
> 	// 	MovementAngleAroundZAxis - 25,
> 	// 	MovementAngleAroundZAxis + 25
> 	// 	);
>
> 	float MovementAngleAroundZAxis = AnimationCore::EulerFromQuat(MovementAngleOffset * FootRotationFactor).Z;
> 	FVector LimitedRotationVector;
> 	LimitedRotationVector.X = AnimationCore::EulerFromQuat(InRotation).X;
> 	LimitedRotationVector.Y = AnimationCore::EulerFromQuat(InRotation).Y;
> 	LimitedRotationVector.Z = FMath::Clamp(
> 										AnimationCore::EulerFromQuat(InRotation).Z,
> 										MovementAngleAroundZAxis - 25,
> 										MovementAngleAroundZAxis + 25
> 										);
> 	LimitedRotation = AnimationCore::QuatFromEuler(LimitedRotationVector);
>
> }
> #pragma endregion
> ```

TempFootPlatform = LockedFeetLocationArray[FootIndex]

###### 更新Lock/UnLock状态

IsFootLockedArray[FootIndex] = (PerFootCyclePercentArray[FootIndex] > SwingTimeAsAPercent)

##### 如果处于UnLocked，也就是sFootLockedArray[FootIndex] == false

###### 对脚踩目标平台位置进行CalculateFootSpline自定义插值计算

TempFootPlatform = CalculateFootSpline(StartingTransform, EndTransform, Alpha)

StartingTransform = LockedFeetLocationArray[FootIndex]

EndTransform = PredictFeetLocationArray[FootIndex]

Alpha = SwingTimeAsAPercent.Remap(0, PerFootCyclePercentArray[FootIndex], 0, 1)

###### CalculateFootSpline函数节点

![1774409103493](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260325154345763-121422232.png)

返回值：OutputTransform

OutputTransform.Rotation = Interpolate(`StartingTransform`.Rotation, `EndTransform`.Rotation, 0.5)

OutputTransform.Translation = PositionFromSpline(SplineFromPoints,Alpha)

SplineFromPoints的7个点位

* `StartingTransform`
* `InsertPoint1` = CalculateOffsetPoint(StartPoint=`StartingTransform`, FwdOrBwd=-1, TargetMaximum=30)
* `InsertPoint2` = `InsertPoint1` + FVector( 0,0,RigSpaceVelocity.Length.Remap(0,300,0,50) )
* `FinalInsertPoint3` = (Distance(`StartingTransform`,`EndTransform`) < 5)? InsertPoint3_Idle: InsertPoint3_Move_ByFootAvoidance
* `InsertPoint4` = InsertPoint2 + FVector( 0,0,RigSpaceVelocity.Length.Remap(0,300,0,30) )
* `InsertPoint5` = CalculateOffsetPoint(StartPoint= `StartingTransform`, FwdOrBwd=1, TargetMaximum=30)
* `EndTransform`

> 其中，
>
> CalculateOffsetPoint的逻辑为：
>
> Return StartPoint + RigSpaceVelocity.Unit × FwdOrBwd × RigSpaceVelocity.Length.Remap(0,300,0,TargetMaximum)

InsertPoint3_Idle = Interpolate(`InsertPoint2`, `InsertPoint4`, 0.5)

InsertPoint3_Move_ByFootAvoidance = FootAvoidance( IdeaLocation=InsertPoint3_Move )

InsertPoint3_Move.XY = 脚部绕着盆骨旋转MovementAngleOffset

InsertPoint3_Move.Z = InsertPoint3_Idle.Z

###### FootAvoidance函数节点

```cpp
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
```

```cpp
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
```

###### 更新Lock/UnLock状态

IsFootLockedArray[FootIndex] = (PerFootCyclePercentArray[FootIndex] > SwingTimeAsAPercent)

###### 根据Lock/UnLock状态，更新脚部位置锁定数组

```cpp
if(IsFootLockedArray[FootIndex])
{
	LockedFeetLocationArray[FootIndex] = TempFootPlatform;
}
```

###### 返回值

TempFootPlatform.Tanslation.Z.Clamp(-30,40)

Return TempFootPlatform

#### 设置脚的前后偏移量SetFootTransforms

![1774426496050](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260325181634282-803179272.png)

##### Rig引用

![1774424709926](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260325181634923-1606539645.png)

##### 设置脚部向前偏移量FootForwardOffset

FootForwardOffset = (TargetFootPlatform.Translation - ThighRig.Translation).Dot( MovementAngleOffset.RotateVector(0,1,0) )

##### 脚部放置

向上偏移保证脚踩在地面

```cpp
FootRig.SetTransfrom( 
		FootRig.Rotation, 
		TargetFootPlatform.Translation + FVector(0,0,9), 
		FootRig.Scale3D 
		)
```

##### 脚前掌旋转偏移点(脚部局部坐标)BallRotationOffsetPoint

![1774428787264](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260325181635758-1193113677.png)

##### 脚尖旋转偏移点(脚掌局部坐标)TipRotationOffsetPoint

![1774429125820](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260325181636243-1754745384.png)

> Ball的世界坐标BallPosition

BallPosition = BallRotationOffsetPoint × FootRig

> 脚底向前移动的向量BallFwdVector

BallFwdVector = ( (BallRig.Translation - FootRig.Translation).XY.Unit ) × 5.8

##### 脚后跟旋转偏移点(脚部局部坐标)HeelRotationOffsetPoint

![1774429338037](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260325181636697-1027601821.png)

> 脚后跟的位置HeelPosition

HeelPosition = ( FootRig.Translation - FVector(0,0,9) ) + BallFwdVector×(-0.8)

##### 腿在身后时，脚先绕着脚前掌旋转，再绕脚尖旋转

> Remap输入范围(-40,-10)和(-70,-40)

###### 脚部绕着脚前掌旋转

![1774430872474](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260325181637221-1796305417.png)

> 绕着脚前掌旋转量RotationAroundBall

FQuat RotationAroundBall = FromEuler( FootForwardOffset.Remap(-40,-10,-40,0) );

###### 取消脚前掌Ball自身的旋转

![1774430886252](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260325181637741-916681290.png)

###### 脚部绕着脚尖旋转

![1774431056928](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260325181638252-1604980620.png)

> 绕着脚尖旋转量RotationAroundTip

FQuat RotationAroundTip = FromEuler( ForwardOffset.Remap(-70,-40,-50,0) );

##### 腿在前面时，脚部绕着脚后跟旋转

> Remap输入范围(15,70)

![1774431261426](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260325181638758-1300590331.png)

> 绕着脚后跟旋转量RotationAroundHeel

FQuat RotationAroundHeel = FromEuler( ForwardOffset.Remap(15,70,0,40) );

##### 最后脚部整体旋转：绕着脚踩处FootPlatform旋转脚部

![1774431636332](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260325181639244-2132324653.png)

#### 手部运动

##### Rig引用

![1774431797297](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260325181639747-124831521.png)

##### 保存Hand的局部变换

![1774432011118](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260325181640242-269363302.png)

##### HandEffector：Hand绕着UpperArm旋转，旋转量同步FootSwing

![1774492797071](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260326115558853-220380605.png)

> Hand.Translation加一个基于移动速度的z轴的偏移量

AddHandZOffset函数节点：

```cpp
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
```

```cpp
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
```

> 旋转量

GetArmMotionEffectorRotationAmount函数节点：

```cpp
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
```

```cpp
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
```

##### 手肘位置ElbowVector

> 手肘位置 = Lower- Hand和Upper的中点

ElbowVector = LowerArmRig - Interpolate(HandRig, UpperArmRig, 0.5)

##### BasicIK

![1774493074383](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260326115559573-1806617297.png)

> 主次轴：PrimaryAxis和SecondaryAxis

GetArmMotionAxisData函数节点：

```cpp
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
```

```cpp
#pragma region 计算ArmMotion的主次轴朝向数据
	FRigUnit_GetArmMotionAxisData_Execute()
	{
		//右骨骼朝向是反的，因此Index不为0时需要反向
		PrimaryAxis   = (ArmIndex == 0) ? FVector(1, 0, 0) : FVector(-1, 0, 0);
		SecondaryAxis = (ArmIndex == 0) ? FVector(0, -1, 0) : FVector(0, 1, 0);
	}
#pragma endregion
```

##### 恢复Hand的局部变换

![1774432031224](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260325181640740-1939353486.png)

##### 肩膀上下晃动

![1774493528331](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260326115600165-807404538.png)

> 肩膀z轴晃动偏移量

GetClavicleZOffset函数节点：

```cpp
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
```

```cpp
#pragma region 计算肩膀的晃动偏移量
FRigUnit_GetClavicleZOffset_Execute()
{
	ClavicleZOffset = sin(2 * PI * 2 * (MasterCyclePercent-0.25) )
									* MathFloatRemap(
										RigSpaceVelocity.Length(),
										0,
										300,
										0,
										1.5,
										true
										) ;
}
#pragma endregion
```

#### 身体偏移OffsetPelvis

##### 用轨迹追踪Z轴偏移量PreviousZTraceOffset

PreviousZTraceOffset = VectorLerp( PreviousZTraceOffset, GetMinHeightFootPlatform(), 5)

> GetMinHeightFootPlatform函数节点：
>
> 左右脚Platform分别做SphereTrace检测，找到最低平台高度
>
> ![1774510917430](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260326191913413-1764770796.png)

##### 保存OffsetPelvis之前的脚部Transform

![1774510947086](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260326191914042-1351903511.png)

##### 盆骨偏移

###### 盆骨朝向PelvisRotation

PelvisRotation = CalculatePelvisRotation(RigSpaceVelocity)

> CalculatePelvisRotation函数节点：
>
> ```cpp
> #pragma region 盆骨朝向
> 	//盆骨朝向：左脚在前顺时针旋转，右脚在前逆时针旋转
> 	USTRUCT(meta = (DisplayName = "CalculatePelvisRotation"), Category = "OffsetPelvis")
> 	struct PROCEDURALANIM_API FRigUnit_CalculatePelvisRotation : public FRigUnit
> 	{
> 		GENERATED_BODY()
>
> 		RIGVM_METHOD()
> 		virtual void Execute() override;
>
> 		UPROPERTY(meta = (Input))
> 		FVector RigSpaceVelocity;
>
> 		UPROPERTY(Transient)
> 		TArray<FTransform> SavedFootPlatformArray;
>
> 		UPROPERTY(meta = (Input))
> 		FQuat MovementAngleOffset;
>
> 		UPROPERTY(meta = (Output))
> 		FQuat Result;
>
> 	};
> #pragma endregion
> ```
>
> ```cpp
> #pragma region 盆骨朝向
> //盆骨朝向：左脚在前顺时针旋转，右脚在前逆时针旋转
> FRigUnit_CalculatePelvisRotation_Execute()
> {
> 	//比较哪个脚在前：脚的位置在移动方向上的投影值
> 	float FootProjectionOnMoveDir = FVector::DotProduct(
> 		SavedFootPlatformArray[0].GetTranslation() - SavedFootPlatformArray[1].GetTranslation(),
> 		MovementAngleOffset.RotateVector(FVector::UnitY())
> 		);
> 	//绕z轴的旋转量 = 速度映射 × 脚的位置在移动方向上的投影值
> 	float RotationAroundZAxis = MathFloatRemap(
> 		RigSpaceVelocity.Length(),
> 		100,
> 		300,
> 		0,
> 		0.2,
> 		true
> 		) * FootProjectionOnMoveDir;
> 	Result = AnimationCore::QuatFromEuler(FVector(0,0,RotationAroundZAxis));
> }
> #pragma endregion
> ```

###### 盆骨自旋转

![1774515198453](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260326191914564-839951691.png)

> 盆骨上下起伏偏移量
>
> GetPelvisZOffsetBasedOnVelocity函数节点：
>
> ```cpp
> #pragma region 盆骨上下起伏偏移量
> 	//盆骨上下起伏偏移量
> 	USTRUCT(meta = (DisplayName = "AddPelvisZOffset"), Category = "OffsetPelvis")
> 	struct PROCEDURALANIM_API FRigUnit_AddPelvisZOffset : public FRigUnit
> 	{
> 		GENERATED_BODY()
>
> 		RIGVM_METHOD()
> 		virtual void Execute() override;
>
> 		UPROPERTY(meta = (Input, Output))
> 		FVector Translation;
>
> 		UPROPERTY(meta = (Input))
> 		float MasterCyclePercent;
>
> 		UPROPERTY(meta = (Input))
> 		FVector RigSpaceVelocity;
>
> 		UPROPERTY(meta = (Input))
> 		float PreviousZTraceOffset;
> 	};
> #pragma endregion
> ```
>
> ```cpp
> #pragma region 盆骨上下起伏偏移量
> 	FRigUnit_AddPelvisZOffset_Execute()
> 	{
> 		//ZOffset = 速度映射 * sin(2Π * 2 * MasterCyclePercent)
> 		float ZOffset = MathFloatRemap(
> 			RigSpaceVelocity.Length(),
> 			0,
> 			300,
> 			0,
> 			5,
> 			true)
> 			* sin(2 * PI * MasterCyclePercent * 2)
> 			+ PreviousZTraceOffset;
> 		Translation += FVector(0,0,ZOffset);
> 	}
> #pragma endregion
> ```

##### 肩膀自旋转RotateSpine

![1774515437912](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260326191915126-1125378755.png)

###### RotateSpine节点

旋转总量/3 分给每一个Spine进行RotateSingleBone

![1774515489925](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260326191915714-1888233954.png)

> RotateSingleBone节点：自旋转
>
> ![1774515576324](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260326191916262-1973778036.png)

##### 基于速度的身体前后倾斜PelvisLean

PelvisLean函数节点

```cpp
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
```

```cpp
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
	float LeanRotateAmountAroundX = LeanRotateAmount * RigSpaceVelocityYProjection;
	//基于速度的前后旋转量(绕x轴)
	FQuat RotateAmount = AnimationCore::QuatFromEuler(FVector(LeanRotateAmountAroundX, 0, 0));
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
```

##### 盆骨侧倾

![1774520023068](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260326191916808-1177920230.png)

> PelvisSideLean函数节点

```cpp
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
```

```cpp
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
			-7,
			7,
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
```

##### 身体跟随脚部的旋转而自旋转

![1774523884042](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260326191917236-1127034094.png)

```cpp
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
```

```cpp
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
```

##### 恢复脚部Transform

![1774519970024](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260326191917686-423668827.png)

#### 腿部IK控制SetFinalLegIK

##### Rig引用

![1774584978310](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260327163513535-1552681184.png)

##### FootEffector

![1774598087636](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260327163514346-1954019163.png)

> CalculateFootEffector函数节点

```cpp
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
```

```cpp
#pragma region 计算FootEffector
	FRigUnit_CalculateFootEffector_Execute()
	{
		URigHierarchy* Hierarchy = ExecuteContext.Hierarchy;

		if(!Hierarchy)
		{
			return;
		}

		FTransform Foot = Hierarchy->GetGlobalTransform(FootRig);
		FTransform Calf = Hierarchy->GetGlobalTransform(CalfRig);
		FTransform Thigh = Hierarchy->GetGlobalTransform(ThighRig);
		FTransform InitialFoot = Hierarchy->GetGlobalTransform(FootRig,true);
		FTransform InitialCalf = Hierarchy->GetGlobalTransform(CalfRig,true);
		FTransform InitialThigh = Hierarchy->GetGlobalTransform(ThighRig,true);

		//把脚部能到达的范围限制在一个球体
		//球心：大腿
		//半径：ClampMaximum = 0.99 × 整条腿的长度(计算时用初始的骨骼位置)
		float DistanceBetweenFootCalf = static_cast<float>(FVector::Distance(InitialFoot.GetTranslation(), InitialCalf.GetTranslation()));
		float DistanceBetweenCalfThigh = static_cast<float>(FVector::Distance(InitialCalf.GetTranslation(), InitialThigh.GetTranslation()));
		float ClampMaximum = 0.99 * ( DistanceBetweenFootCalf + DistanceBetweenCalfThigh );

		OutFootEffector.SetTranslation(
			FRigVMMathLibrary::ClampSpatially(
				Foot.GetTranslation(),
				EAxis::X,
				ERigVMClampSpatialMode::Sphere,
				0,
				ClampMaximum,
				Hierarchy->GetGlobalTransform(ThighRig)
			)
		);
		OutFootEffector.SetRotation( Foot.GetRotation() );
		OutFootEffector.SetScale3D( Foot.GetScale3D() );
	}
#pragma endregion
```

##### 基于移动角度偏移的膝盖朝向向量KneeVector

> 关于RotateVector节点
>
> `A.RotateVector(B)` ：用旋转A旋转向量B，也就是向量B 按旋转 `A` 变换

![1774599510581](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260327163514792-177567963.png)

```cpp
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
```

```cpp
#pragma region 计算基于移动角度偏移的膝盖朝向向量KneeVector
	FRigUnit_CalculateKneeVector_Execute()
	{
		//膝盖朝向向量 = 默认的膝盖朝向向量 按照 脚部移动朝向的旋转 变换
		FQuat FootMovementRotation = FQuat::Slerp(SavedFootPlatformArray[FootIndex].GetRotation(), MovementAngleOffset, 0.25);
		OutKneeVector = FootMovementRotation.RotateVector(DefaultKneeVectorArray[FootIndex] * 20);
	}
#pragma endregion
```

##### BasicIK

![1774599609818](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260327163515296-827611803.png)

> 计算FinalLegIK的主次轴朝向数据CalculateFinalLegIKAxisData函数节点：

```cpp
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
```

```cpp
#pragma region 计算FinalLegIK的主次轴朝向数据
	FRigUnit_CalculateFinalLegIKAxisData_Execute()
	{
		//右脚的骨骼朝向是反的，因此Index不为0时需要乘以的是-1
		const float Sign = (LegIndex == 0) ? 1.0f : -1.0f;
		PrimaryAxis = FVector(-1, 0, 0) * Sign;
		SecondaryAxis = FVector(0, 1, 0) * Sign;
	}
#pragma endregion
```

##### 脚部在处于Swing阶段时，保持原旋转信息KeeoOriginalRotation

![1774599744007](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260327163515739-1387090305.png)

###### <1：处于Swing阶段，这一阶段映射为平滑的曲线

> 越靠近Swing的中间阶段(也就是脚部摆动到身体正下方)权重越高

![1774600234927](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260327163516180-1783767002.png)

###### 按照权重赋值给脚部以原始的旋转信息

> 额外需要考虑的速度因素：速度=0时，该操作的权重为0

![1774600352774](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260327163516689-576117921.png)
