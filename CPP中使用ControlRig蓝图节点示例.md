## 基本流程

![1774598530978](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260327160432721-994329303.png)

逻辑代码迁移到C++自定义ControlRig节点：

需要.Build中添加编译要用到的模块AnimationCore

![1774598536172](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260327160433289-1932975330.png)

RigUnit_ProceduralCharacter.h

```cpp
#pragma region 计算脚部的目标朝向
    //计算脚部的目标朝向
    USTRUCT(meta = (DisplayName = "GetFootTargetAngle"))
    struct PROCEDURALANIM_API FRigUnit_GetFootTargetAngle : public FRigUnit
    {
        GENERATED_BODY()

        RIGVM_METHOD()
        virtual void Execute() override;

        UPROPERTY(meta = (Input))
        FVector RigSpaceVelocity;
  
        UPROPERTY(meta = (Output))
        float FootTargetZAngle;

    };

    FVector EulerFromQuat(const FQuat& Rotation, EEulerRotationOrder RotationOrder = EEulerRotationOrder::ZYX, bool bUseUEHandyness = false);

    FQuat FromTwoVectors(const FVector& A, const FVector& B);

#pragma endregion
```

RigUnit_ProceduralCharacter.cpp

```cpp
#pragma region 计算脚部的目标朝向
    FRigUnit_GetFootTargetAngle_Execute()
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
```

![1774598542118](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260305232647459-86592147.png)

## 对于一些蓝图和实际C++API名字不一致的节点

搜DisplayName="蓝图节点名"找对应的C++API

> 比如：Interpolate节点
>
> ![1774598644513](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260327160434153-594007449.png)
>
> ![1774598764221](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260327160614137-1638330505.png)
