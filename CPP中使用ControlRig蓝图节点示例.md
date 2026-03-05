![1772703106545](file:///E:/UE_Projects/ProceduralAnim/image/%5BUE5%5D%E5%AE%8C%E5%85%A8%E7%A8%8B%E5%BA%8F%E5%8C%96%E7%9A%84%E8%A7%92%E8%89%B2%E5%9F%BA%E7%A1%80%E7%A7%BB%E5%8A%A8%E6%8E%A7%E5%88%B6%E5%99%A8/1772703106545.png?lastModify=1772703649)

逻辑代码迁移到C++自定义ControlRig节点：

需要.Build中添加编译要用到的模块AnimationCore

![1772703564402](file:///E:/UE_Projects/ProceduralAnim/image/%5BUE5%5D%E5%AE%8C%E5%85%A8%E7%A8%8B%E5%BA%8F%E5%8C%96%E7%9A%84%E8%A7%92%E8%89%B2%E5%9F%BA%E7%A1%80%E7%A7%BB%E5%8A%A8%E6%8E%A7%E5%88%B6%E5%99%A8/1772703564402.png?lastModify=1772703649)

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

![1772703209005](file:///E:/UE_Projects/ProceduralAnim/image/%5BUE5%5D%E5%AE%8C%E5%85%A8%E7%A8%8B%E5%BA%8F%E5%8C%96%E7%9A%84%E8%A7%92%E8%89%B2%E5%9F%BA%E7%A1%80%E7%A7%BB%E5%8A%A8%E6%8E%A7%E5%88%B6%E5%99%A8/1772703209005.png?lastModify=1772703649)