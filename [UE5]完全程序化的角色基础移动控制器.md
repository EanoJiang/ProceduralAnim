# 程序化Locomoition角色控制器

## 第一个程序动画

创建一个control rig

![1771432079406](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260219003736840-1957939409.png)

control rig蓝图中，获取骨骼的transform，暂时先控制pelvis来验证该功能，选择Space为全局空间，让过渡Translation的Z轴附加一个sin函数，模拟身体上下浮动

![1771432273355](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260219003737343-1767167385.png)

选择Use Specific Animation

![1771432468447](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260219003737557-304339491.png)

效果：

![1771432739182](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260219003908933-1333027228.gif)

## BasicLegIK

> 基础的腿部IK

### 设置好腿部IK链：thigh-calf-foot，Efector设置为ik_foot

![1771696491553](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260224025711404-969884943.png)

### 这里还需要设置主次朝向，也就是Primary和Secondaru Axis

Primary Axis为Item A(也就是thigh)，Secondaru Axis为Item B(也就是calf)

thigh_l的坐标系如图(红绿蓝分别对应xyz)：

![1771696810920](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260224025711855-419014939.png)

calf_l的坐标系如图：

![1771696918143](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260224025712047-1385117244.png)

大腿需要朝向下，膝盖需要朝向正前

因此Primary Axis的x=-1，Secondaru Axis的y=1

![1771697607866](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260224025712498-973873507.png)

**Pole Vector（极向量）** ：控制 IK 的关节弯曲方向

因此设置朝向正前

![1771697799806](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260224025712863-667743868.png)

效果：

![1771698439534](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260224025713196-595031713.gif)

把控制脚部偏移的节点加进来

![1771699821664](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260224025713546-251428571.png)

### 用循环来为左右脚都设置IK

先建立一个脚部Transform的列表

![1771713374328](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260224025713812-1092341731.png)

然后给左右脚都设置好ik，因为右脚的骨骼朝向与左脚相反，所以当index!=0时主次轴朝向都需要乘-1

![1771713587578](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260224025714045-879843016.png)

效果：

![1771715122305](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260224025714283-401710303.png)

暂时先删去脚部偏移

![1771721536395](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260224025714480-1606346751.png)

## FootRotation

> 脚的旋转

### RotateAroundPoint

> 脚绕着目标点旋转

也就是“要旋转的物体” 相对于 “旋转中心点” 的旋转偏移，如图所示也就是向量A-B

![1772088881553](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170220091-1554002163.png)

![1772004217010](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260225184331416-126447513.png)

> 四元数乘的顺序问题：
>
> | A*B | 先乘B后乘A |
> | --- | ---------- |
>
> 也就是Quat_B要乘以Quat_A的量，因此要旋转的Transform放在B

> RotateVector 节点：
>
> Transform：旋转信息变换，也就是旋转量
>
> Vector：要被旋转的方向向量Vector

### SetFootTransforms

> 根据脚的位置计算脚的旋转方式

如果脚在身体前面，脚绕着脚踝旋转

如果脚在身体后面，脚绕着脚前掌旋转

#### Foot以及Ball骨骼的引用

![1771923895200](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260224182234000-1740565635.png)

![1771923812800](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260224182234431-94168320.png)

#### 脚踩平台追踪

从每个Foot的z轴(-50,50)追踪脚踩平台的接触点

![1771926458334](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260224182235059-1884292044.png)

![1771926161002](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260224182237218-244697539.gif)

#### 脚部放置

设置Foot的Transform属性，Translation(位置)取自脚踩平台高度，Rotation和Scale保持Foot本身的不变

![1771927780254](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260224182238374-40463336.png)

效果：

![1771928419031](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260224182241835-2012960109.gif)

#### 脚踩平台旋转偏移

最终脚部的Rotation  = 脚踩平台的Rotation * 脚部的Rotation

![1772006712916](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260225184332316-1627290789.png)

> 再次强调四元数乘的顺序问题：
>
> | A * B | 先 B，后 A | 子对象（A）在父对象（B）的空间内变换 |
> | ----- | ---------- | ------------------------------------ |
> | B * A | 先 A，后 B | 父对象（B）在子对象（A）的空间内变换 |
>
> A是子对象，B是父对象
>
> 我们需要的是先让脚部按照自身的 `Foot.Rotation` 旋转，然后再将整个脚部（包括它自己的旋转）作为一个整体，跟随平台的 `TargetFootPlatform.Rotation` 进行旋转
>
> 也就是平台先转，脚再跟着平台转，因此是脚踩平台的 `TargetFootPlatform.Rotation`* `Foot.Rotation`，而不是反过来乘

先绕Z轴旋转45度看下效果：

![1772003050920](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260225184333631-1630878904.png)

![1772006792679](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260225184334982-1298560636.png)

#### 计算脚踩平台的前后偏移量

向前方向是y = 1

![1772013041788](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260225184335394-750579281.png)

如果脚在后面，那么点积结果<0，因此就可以根据这两个向量的点积结果来判断脚的位置(相对于脚踩处的位置)

![1772014163983](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260225184335950-1274931643.png)

因此，(Vector_脚踩平台位置 - Vector_大腿位置) ·(0,1,0)得到的值即为脚部向前偏移量FootForwardOffset

![1772013837859](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260225184336538-638331927.png)

#### 绕着脚踩平台旋转脚部

删去之前写的Rotation逻辑，直接取用FootRig.Rotation即可

然后再单独处理脚部旋转逻辑：调用之前写的RotateAroundPoint函数，待旋转的是脚部，旋转点和旋转量是脚踩处

![1772015965153](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260225184337511-775819291.png)

#### 计算脚前掌ball的旋转偏移点

> **Make Relative** ：将一个变换从全局空间转换为相对于另一个变换的局部空间

全局坐标中，脚前掌旋转点的水平方向XY取自脚前掌，高度Z取自脚踩处

需要变换到脚部Foot的局部坐标

![1772090580544](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170220848-1208799406.png)

在脚部旋转的逻辑处理之后，可视化脚前掌旋转点

> Transform相乘：坐标系转换，通常是子Transform乘以父Transform转换为世界坐标

![1772090515152](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170221415-1268909137.png)

如图中绿色框

![1772075837766](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170221813-786024168.png)

如果脚部的z轴旋转量增大，会发现脚前掌旋转点位置不精确，这是因为最终的LegIK会限制脚部的旋转

#### 计算脚尖tip的旋转偏移点

> ball沿着脚底向前移动一段距离即为tip

脚底向前方向向量 = Vector_脚前掌(ball)- Vector_脚踝(foot)

![1772089335200](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170222220-31913696.png)

脚底向前方向向量 + 脚前掌Ball的世界坐标 = 脚尖旋转偏移点的世界坐标，然后相关于脚前掌的局部坐标

![1772090864997](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170222951-48573394.png)

绘制脚尖旋转偏移点(需要转换为世界坐标)

![1772177837011](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170223535-1116824328.png)

最终得到

![1772089382802](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170223890-1454147542.png)

#### 计算脚后跟heel的旋转偏移点

脚踩点位置 + 脚底向前方向向量*(-0.8) = 脚后跟旋转偏移点的世界坐标，然后相关于脚部的局部坐标

![1772092948348](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170224605-1841208154.png)

绘制脚后跟旋转偏移点

![1772177870868](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170225244-1988844386.png)

最终得到

![1772093237126](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170225616-409653651.png)

#### 所有点位

![1772093487161](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170225971-1752755266.png)

#### 封装绘制这些点位的函数

![1772178342219](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170226605-210759112.png)

![1772178441775](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170227220-287116351.png)

#### 旋转脚部

##### 腿在后面：先绕着脚前掌Ball旋转脚部Foot，再绕着脚尖Tip旋转脚部Foot

因为旋转量是取自脚部向前偏移量，因此把这个偏移量分为两个阶段：(-10 ~ -40)，(-40 ~ -70)

###### 绕着脚前掌Ball旋转脚部Foot

> 旋转量取自脚部向前偏移量，从(-40,-10)Remap为(-25,0)

![1772182237348](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170228029-406061828.png)

暂时关闭SetFinalLegIK

![1772179113059](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170228534-1859221509.png)

设置脚部前后偏移(沿y轴)

![1772179144232](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170228936-184262135.png)

![1772178852073](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170229469-1505452978.png)

修改一下前面计算点位时的Z轴逻辑

脚前掌

![1772178658418](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170229972-166298626.png)

脚后跟

![1772178769163](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170230534-1336774268.png)

取消旋转脚前掌ball

![1772180893796](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170231216-810783063.png)

###### 绕着脚尖Tip旋转脚部Foot

> 旋转量取自脚部向前偏移量，从(-70,-40)Remap为(-30,0)

![1772180915709](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170231943-1594964675.png)

##### 腿在前面：绕着脚后跟heel旋转脚部Foot

![1772181245196](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170232725-790897987.png)

开启SetFinalLegIK

![1772181797229](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170233212-561434100.png)

设置Z轴偏移，防止脚部浮空

![1772181765909](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170233790-1472486525.png)

最终效果：

![1772182929911](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260227170235932-660111330.gif)

## Velocity cycles and leg movement

### 计算速度

> CalculateVelocity

世界空间速度 = (根骨骼世界坐标 - 上一帧的根骨骼世界坐标) / DeltaTime

![1772250962641](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260228184950432-61572292.png)

骨骼空间速度 = (世界空间速度 + 骨骼位置).转换到骨骼空间 - 骨骼位置

绘制相对于根骨骼的角色速度 = 从根骨骼位置出发 -> (根骨骼位置+骨骼空间速度)

![1772250990422](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260228184951496-1249100348.png)

![1772251672715](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260228184952441-1136601168.png)

### 锁定脚部位置的数组

构造一个LockedFootLocationArray

![1772272837034](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260228184952980-214666193.png)

![1772263836582](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260228184953514-253908292.png)

根骨骼前后帧的相对变换：`T_now * T_last⁻¹`：相当于 “先撤销上一帧的所有变换（逆操作），再执行当前帧的变换”，最终得到的就是 “从上一帧到当前帧，物体相对动了多少”

![1772262856028](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260228184954229-1808055187.png)

### 计算Cycle

![1772264943770](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260228184954649-784097948.png)

![1772264926816](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260228184955061-800474028.png)

### 计算脚踩目标平台的位置

> CalculateFootTargetPlatform

![1772272870902](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260228184955529-610894640.png)

![1772272891048](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260228184955916-744103439.png)

#### UnLocked状态

##### 对脚踩目标平台位置进行Lerp线性插值计算

如果处于UnLocked状态，让局部变量脚踩目标平台位置(TempFootPlatform)从 原锁定位置-Lerp->新的脚部位置，Lerp的阻尼取自Cycle进度，也就是Cycle从0 ~ 0.5映射到Lerp的阻尼就是0 ~ 1

为什么是0.5？因为下面会设置规则Cycle百分比大于0.5就锁定脚部

![1772273005885](https://img2024.cnblogs.com/blog/3614909/202602/3614909-20260228184956495-968709934.png)

##### 更新数组

然后更新脚部锁定数组Cycle百分比大于0.5就锁定，再更新脚部锁定位置数组

![1772387162500](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260302040038463-1330553780.png)

#### Locked状态

![1772391588977](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260302040039088-1781525512.png)

#### UnLocked状态，对未来脚部位置进行预测，然后Lerp

> 之前只是默认Lerp到角色正下方的脚部位置，现在需要预测角色即将到达的脚部位置，再Lerp

##### 新建预测脚部位置数组

![1772433873869](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004812349-1582938587.png)

##### 预测脚部落点

![1772435916646](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004813113-1332294188.png)

适配步幅的骨骼空间移动速度 = 骨骼空间速度的单位向量 × (步幅/2)

![1772436641931](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004813717-1605169085.png)

得到的PredictFeetLocationArray就可以传入CalculateFootTargerTransform函数，将脚踩目标平台Lerp到角色未来即将到达的脚部位置

![1772435938098](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004814291-624215549.png)

![1772435965012](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004814907-1300258154.png)

效果：

![1772436971225](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004818210-1848424984.gif)

### 交替移动两条腿

> 脚部Cycle以异步模式进行

#### 新建每只脚的Cycle百分比数组

![1772437991266](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004818837-1981714543.png)

每只脚的Cycle百分比 = 主Cycle百分比 + (FootIndex × 0.5) ，最后保证这个百分比超过1自动重置

这样当开始主Cycle时，左脚index = 0，右脚index = 1，右脚就比左脚慢了半个周期

![1772440215913](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004819379-143851587.png)

替换计算目标脚踩平台中的MasterCyclePercent为每只脚的Cycle百分比

![1772440713807](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004820067-2124001166.png)

![1772440685761](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004820617-830070521.png)

确保脚底在地面上

![1772442061844](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004821287-997048593.png)

### 预测角色的移动以实现脚部落点的追踪

未落地前的的骨骼空间移动速度应该是即将前进的距离，加上移动方向上的步幅，这样在脚部遇到障碍物之前，会提前抬高一些防止脚部穿模

其中，即将前进的距离 = RigSpaceVelocity × 慢慢变小的预测时间，由于预测时间在慢慢减小，这个前进的距离也在由大变小，因此脚部抬高与前进会产生一个曲线，自然过渡到可能要遇到的障碍物的平面落点

![1772457150788](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004822049-1862465812.png)

### 计算脚部的插值平滑曲线

位置Translation从StartingTransform平滑插值到EndTransform，平滑模式设置为BSpline(全局平滑)，中间可自定义2个插值的z轴高度

![1772461785003](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004822638-1038883934.png)

旋转Rotation直接简单Lerp插值即可

![1772461821241](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004823109-1303188977.png)

回到计算脚踩目标平台的函数：将原来的Interpolate替换为写好的平滑曲线过渡

![1772462148199](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004823657-438865889.png)

效果：

![1772462372749](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004828575-883660091.gif)

### 动态Cycle时间

> ![1772462557496](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004831375-1825417182.gif)
>
> 可以发现，如果移动速度过快，会出现往后迈的腿的锁定位置远远落后，导致该腿会绷直一段时间，很不自然
>
> 这是因为：主Cycle百分比是固定值，当速度很快的时候脚的锁定时间过长
>
> 因此需要改为动态的Cycle时间，当移动速度变得更快的时候这个Cycle时间要相应的更短

![1772467125971](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004832104-897387842.png)

得到的摆动时间占Cycle周期长度的百分比SwingTimeAsAPercent替换掉原来的固定值0.5

![1772467268936](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004832725-593435552.png)

![1772467306421](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004833251-1842405635.png)

![1772467705734](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004833813-389892349.png)

修复脚部锁定过于向前：抵消掉预测即将前进距离

![1772467442570](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004834537-713568779.png)

### 修复：静止状态时原地踏步的问题

![1772468125823](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004835186-1200799332.png)

不同速度下，应该对应的平滑曲线示意图：

高速——黄色关键帧和红色曲线所示，低速——蓝色关键帧和紫色曲线所示

![1772469476856](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303004835577-260061151.png)

改造CalculateFootSpline

![1772523634540](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303165908790-777489239.png)

从起始点沿着移动速度方向，向前/后偏移一定距离处，-1后退，1前进

因此点位1就是开始位置0向后偏移8，点位2是结束位置6向前偏移20——(偏移量随时更改)

![1772523482254](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303165909358-343120255.png)

点位2是点位1沿z轴向上偏移10，点位4是点位5沿z轴向上偏移20，点位3是2和4的中点——(偏移量随时更改)

![1772523950413](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303165910037-1371520921.png)

最后按序传入BSpline平滑曲线的点位，得到的Position传给返回值OutputTransform的Translation属性，Rotation属性这里先直接用StartingTransform和EndTransform的混合值即可

![1772524046756](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303165910650-149499612.png)

效果：

![1772524858618](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303165914097-1453139087.gif)

### 适当减小地面停留时间，让脚部重置的响应快一些

![1772524308916](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303165914861-2072940166.png)

### 修复：脚部落地前出现一段时间的大小腿绷直

这是因为脚部的目标IK位置距离太远，脚部要想到达该位置会尽量伸直大小腿来实现，膝盖就会绷得很紧

解决方法：限制IK距离

![1772527890218](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303165915541-1416418940.png)

Foot到Calf + Calf到Thigh，距离之和是大小腿的总长，那么脚部IK的目标位置不能超过腿长的1.02——(这个值可以调整)

> 要保证获取的Transform是初始的值，不要被其他地方修改所影响，因此要勾选Initial

![1772527932189](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303165916163-92251060.png)

效果：

![1772528278547](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303165918077-243874878.gif)

## Pelvis and spine control

> 身体控制

### 初始化设置——盆骨偏移OffsetPelvis放到SetFinalIK之前

![1772534591440](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303184328361-1465361875.png)

先保存OffsetPelvis之前的脚部Transform，然后恢复

![1772533122178](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303184328910-2034870564.png)

![1772531682587](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260303184329449-1562605076.png)

### Pelvis Cycle

#### 基于速度的身体上下循环偏移

![1772607757245](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260304231852128-239251802.png)

#### 身体左右旋转循环偏移

人体运动学：左脚迈出时，上半身体顺时针旋转；右脚迈出时，上半身体逆时针旋转

![1772614623284](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260304231852704-1666388943.png)

因此，需要比较哪个脚在前，来决定上半身体的旋转方向以及旋转角度

![1772616847472](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260304231853559-890571464.png)

![1772616889142](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260304231854258-1321256866.png)

效果：

![1772619583824](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260304231858724-649956173.gif)

#### 肩膀左右旋转循环偏移

> 用来补偿身体的左右旋转带来的手臂顺拐现象，因此肩膀的左右旋转方向与身体应当是相反的

SpineRotation = PelvisRotation × -2

![1772618961015](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260304231859564-755706868.png)

![1772680505185](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260305232637880-218780512.png)

![1772680516455](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260305232638864-1736375812.png)

效果：

![1772619740146](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260304231905932-103797108.gif)

如果把Scale设置的更大，可以看到更夸张的效果，因此这个Scale可以根据想要的效果更改值大小：

![1772619402343](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260304231909701-1231608457.gif)

#### 脖子左右旋转循环偏移

![1772680416619](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260305232639571-59164947.png)

![1772680549327](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260305232640228-1102891344.png)

效果：

![1772680758792](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260305232643928-1789930536.gif)

### 斜向移动时骨骼朝向偏移

#### 将脚踩平台的Transform存为数组以供随时访问

在SetupFootArray中初始化后，再存储该数组

![1772693516310](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260305232644700-566863198.png)

用左右脚踩平台用来替换左右脚的引用

![1772695548617](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260305232645276-1303488832.png)

#### 斜向移动时左右脚的朝向偏移

对于靶向移动来说，在同一条对角线上的两个方向移动时的脚朝向是相同的，因此只需要考虑角色面前的180度半圆内的脚朝向

需要设置一定死区，防止越过临界值的时候移动方向突变，这里设置为10度死区

![1772696872362](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260305232645835-857602160.png)

从图中可以看出：

向斜后方移动的角度 > 100，实际的脚部朝向角 = 移动方向角 - 180

向斜后方移动的角度 <-100，实际的脚部朝向角 = 移动方向角 + 180

![1772703106545](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260305232646481-357227241.png)

得到的FootTargetZAngle转换为Quat，得到移动角度偏移MovementAngleOffset

![1772765702434](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260306173952685-801369888.png)

逻辑代码迁移到C++自定义ControlRig节点：

需要.Build中添加编译要用到的模块AnimationCore

![1772703564402](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260305232646898-1176934408.png)

RigUnit_ProceduralCharacter.h

```cpp
#pragma region 计算移动角度偏移
	//计算移动角度偏移
	USTRUCT(meta = (DisplayName = "GetMovementAngleOffset"))
	struct PROCEDURALANIM_API FRigUnit_GetMovementAngleOffset : public FRigUnit
	{
		GENERATED_BODY()

		RIGVM_METHOD()
		virtual void Execute() override;

		UPROPERTY(meta = (Input))
		FVector RigSpaceVelocity;

		UPROPERTY(meta = (Output))
		float FootTargetZAngle;

		UPROPERTY(meta = (Output))
		FQuat MovementAngleOffset;
	};

	FVector EulerFromQuat(const FQuat& Rotation, EEulerRotationOrder RotationOrder = EEulerRotationOrder::ZYX, bool bUseUEHandyness = false);

	FQuat FromTwoVectors(const FVector& A, const FVector& B);

#pragma endregion
```

RigUnit_ProceduralCharacter.cpp

```cpp
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

```

![1772766089385](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260306173953430-2030078327.png)

在PredictFootLandingSpot中传入移动角度偏移

![1772767143284](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260306173954051-521816941.png)

效果：

![1772768332916](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260306173958885-473144745.gif)

#### 在FinalLegIK中传递移动角度偏移到膝盖

![1772779918985](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260306173959842-1944296981.png)

效果：

![1772780260626](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260306174005060-62762318.gif)

### 身体跟随脚部的旋转而自旋转

上面解决了脚部旋转以及膝盖的跟随旋转，但是Pelvis还没有跟着旋转，因此需要找到双脚间的平均旋转偏移来附加到Pelvis上

![1772788823676](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260306174005988-499352574.png)

![1772786694356](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260306174006779-766120383.png)

### 预测脚步落点位置需要绕着身体旋转

![1772789901280](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260306174007338-1529557124.png)

效果：

八向移动

![1772790080868](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260306174533162-2087098154.gif)

斜向移动

![1772790290564](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260306174539631-498234878.gif)

## Smoothing and rotation limits

> 转向限制，尽可能避免出现腿部交叉的情况

### 减少方向切换时的移动角度偏移

![1773000780089](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260309041640916-1659453663.png)

### 让更新前后的向量平滑过渡(不受帧速率影响)

> 之前的向量更新是瞬时的，因此会出现腿部位置突变的情况，极其破坏动作的说服力

#### 新建用于向量的Lerp函数(消除帧率差异)

LerpedVector = InVector + 差值 * 基于DeltaTime的变化量

![1773026607721](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260309180029582-1861564373.png)

迁移到C++

```cpp
#pragma region 消除帧率差异的用于Vector的Lerp函数
	USTRUCT(meta = (DisplayName = "VectorLerp"), Category = "Lerp")
	struct PROCEDURALANIM_API FRigUnit_VectorLerpIndependentOnFrameRate : public FRigUnit
	{
		GENERATED_BODY()

		FRigUnit_VectorLerpIndependentOnFrameRate()
		{
			LerpedVector = TargetVector = InVector = FVector(1.f, 0.f, 0.f);
			MaxDelVectorPerSecond = 0.f;
		}
		RIGVM_METHOD()
		virtual void Execute() override;

		UPROPERTY(meta = (Input))
		FVector InVector;

		UPROPERTY(meta = (Input))
		FVector TargetVector;

		UPROPERTY(meta = (Input))
		float MaxDelVectorPerSecond = 0;

		UPROPERTY(meta = (Output))
		FVector LerpedVector;
	};

FVector MathVectorClampLength(FVector Value = FVector(1.f, 0.f, 0.f), float MinimumLength = 0, float MaximumLength = 1);
#pragma endregion
```

```cpp
#pragma region 消除帧率差异的用于Vector的Lerp函数
FRigUnit_VectorLerpIndependentOnFrameRate_Execute()
{
	FVector DeltaVector = MathVectorClampLength(TargetVector - InVector, 0,MaxDelVectorPerSecond * ExecuteContext.GetDeltaTime<float>());
	LerpedVector = InVector + DeltaVector;
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
```

RigSpaceVelocity加入自定义的VectorLerp节点

![1773027985876](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260309180032144-386160337.png)

#### 修复：预测脚部落点位置更新不及时的Bug

> 当阻尼值设置较小时会出现，停下后脚部落点位置更新不及时
>
> ![1773028171213](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260309180045607-1578889088.gif)
>
> 这是因为停下的瞬间RigSpaceVelocity已经为0，这时候的移动方向是未知方向，因此移动方向上的步幅会出问题
>
> 因此还需要修改预测脚部落点位置的函数逻辑

移动方向上的步幅中，步幅原来的逻辑是RigSpaceVelocity的单位向量 × 步长的一半，因为归一化节点unit对于零向量会出问题，因此改为RigSpaceVelocity×在地面停留时间的一半

![1773032574271](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260309180357642-454482477.png)

#### 移动角度偏移MovementAngleOffset加入自定义的VectorLerp节点

考虑到之前把移动角度偏移MovementAngleOffset的逻辑也封装到了C++中，那么要想在C++中也调用这个自定义的VecotrLerp节点，需要把该节点的功能再封装一层为独立的函数

```cpp
#pragma region 消除帧率差异的用于Vector的Lerp函数
	USTRUCT(meta = (DisplayName = "VectorLerp"), Category = "Lerp")
	struct PROCEDURALANIM_API FRigUnit_VectorLerpIndependentOnFrameRate : public FRigUnit
	{
		GENERATED_BODY()

		FRigUnit_VectorLerpIndependentOnFrameRate()
		{
			LerpedVector = TargetVector = InVector = FVector(1.f, 0.f, 0.f);
			MaxDelVectorPerSecond = 0.f;
		}
		RIGVM_METHOD()
		virtual void Execute() override;

		UPROPERTY(meta = (Input))
		FVector InVector;

		UPROPERTY(meta = (Input))
		FVector TargetVector;

		UPROPERTY(meta = (Input))
		float MaxDelVectorPerSecond = 0;

		UPROPERTY(meta = (Output))
		FVector LerpedVector;
	};

	FVector VectorLerpIndependentOnFrameRate(FVector InVector, FVector TargetVector, float MaxDelVectorPerSecond = 0, float DeltaTime = 0);

	FVector MathVectorClampLength(FVector Value = FVector(1.f, 0.f, 0.f), float MinimumLength = 0, float MaximumLength = 1);
#pragma endregion
```

```cpp
#pragma region 消除帧率差异的用于Vector的Lerp函数
FRigUnit_VectorLerpIndependentOnFrameRate_Execute()
{
	float DeltaTime = ExecuteContext.GetDeltaTime<float>();
	LerpedVector = VectorLerpIndependentOnFrameRate(InVector, TargetVector, MaxDelVectorPerSecond, DeltaTime);
}

FVector VectorLerpIndependentOnFrameRate(FVector InVector, FVector TargetVector, float MaxDelVectorPerSecond, float DeltaTime)
{
	FVector DeltaVector = MathVectorClampLength(TargetVector - InVector, 0,MaxDelVectorPerSecond * DeltaTime);
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
```

加入自定义的VectorLerp节点

![1773034466179](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260309180358402-202197218.png)

迁移到C++：

```cpp
		UPROPERTY(meta = (Input))
		float MaxDelVectorPerSecond = 360.0f;
```

![1773035661355](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260309180110739-688213998.png)

```cpp
		FVector LerpedVector = VectorLerpIndependentOnFrameRate(
			AnimationCore::EulerFromQuat(MovementAngleOffset),
			FVector(0,0,FootTargetZAngle),
			MaxDelVectorPerSecond,
			ExecuteContext.GetDeltaTime<float>()
			);

		MovementAngleOffset = AnimationCore::QuatFromEuler(FVector(0,0,LerpedVector.Z));
```

![1773035703037](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260309180114569-1355151275.png)

![1773035609623](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260309180359574-25394959.png)

暂时把每秒的最大变换量设置为360，在这里也就是1圈

效果：

![1773036272151](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260309180402966-1314681567.gif)

> 突然改变移动方向不会出现盆骨Rotation突变的情况

#### 修复：侧向移动时脚部是平移过去的，没有正确旋转

> 正确次序是：绕脚掌ball旋转->绕脚尖tip旋转->绕脚后跟旋转->绕脚踩处旋转

原来的逻辑：

![1773037347045](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260309180403770-603951408.png)

![1773037443379](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260309180407391-1644290511.gif)

修改后：

![1773037161646](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260309180408171-906638952.png)

![1773037310392](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260309180234469-1970409750.gif)

#### 限制脚部的旋转偏移

脚部的Z轴旋转(左右旋转)受移动角度偏移限制，限制权重为0.5

![1773043107974](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260309180236320-2128186786.png)

限制在(-25,25)度

![1773042936582](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260309180237368-1466338623.png)

将之前预测落点函数中的移动角度偏移权重也更换为这个变量

![1773043047250](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260309180238083-2043655884.png)

#### 修复：当脚部经过身体正下方会处于很平的浮空

让脚部在完成Swing阶段后保持最初始的旋转信息

怎么判断已完成Swing阶段？

每只脚的Cycle实时百分比 / Swing阶段占比 < 1，说明当前处于Swing阶段

并且用曲线来平滑处理，中点就是Swing阶段时脚部经过身体正下方的时刻，权重为1，完全是最初始的旋转信息

![1773049567322](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260309180238823-1814572723.png)

效果：

![1773049054575](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260309180241866-1221455551.gif)

#### 修复：出现腿部交叉的情况

> 对比现实中，如果腿部即将交叉，我们会让一条腿绕着另一条腿向前或向后旋转

怎么实现"一条腿绕着另一条腿旋转"？

这个自定义脚部曲线的中间点3的水平面位置xy绕着盆骨旋转，旋转量为移动角度偏移，z轴位置还是原先的InsertPoint3

![1773087349065](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260310071857004-643128445.png)

### 让步幅与移动速度相关

跑的时候步幅小，走的时候步幅大

![1773092931518](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260310071857389-589265815.png)

## Arm motion

> 手部运动

由于身体偏移是在z轴进行的，如果要让手部运动sin型，只需在身体偏移之前让手部运动沿着移动速度的方向做前后运动即可

### 建立HandArray

![1773134402936](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175413254-1556794188.png)

迁移到C++

```cpp
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

```

### ArmMotion

![1773136230299](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175414008-88741050.png)

![1773136246710](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175414682-600491753.png)

主次朝向逻辑判断迁移到C++中：

```cpp
#pragma region 计算ArmMotion的主次轴朝向数据
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

![1773136397589](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175415258-96964285.png)

给Effector加一点偏移，效果如下：

![1773137363097](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175415693-1157092082.png)

### 绕着肩膀旋转手臂

![1773200218511](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175416554-1982974411.png)

基于移动速度的Arm摆动旋转曲线：
RigSpaceVelocity.Length.Remap * sin( 2Π * (PerFootCyclePercent+0.25)%1 )

手臂摆动旋转曲线中，由于需要和腿部实际摆动周期同步，需要加上一定偏移量，这里暂时设为提前1/4周期

![1773200263309](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175417222-1772499763.png)

Arm摆动旋转曲线迁移到C++

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

		UPROPERTY(meta = (Output))
		FQuat RotateAmount;
	};

	float MathFloatRemap(float Value, float SourceMinimum, float SourceMaximum, float TargetMinimum, float TargetMaximum, bool bClamp);

#pragma endregion
```

```cpp
#pragma region 计算ArmMotion的Effector的RotationAmount值
FRigUnit_GetArmMotionEffectorRotationAmount_Execute()
{
	const float PerFootCyclePercent = PerFootCyclePercentArray[ArmIndex];
	//RigSpaceVelocity.Length.Remap * sin( 2Π * (PerFootCyclePercent+0.25)%1 )
	const float ArmSwingCurve = sin(2 * UE_PI * FMath::Fmod(PerFootCyclePercent + 0.25f, 1.0f))
						* MathFloatRemap(
							RigSpaceVelocity.Length(),
							0,
							500,
							0,
							30,
							true
							);

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
```

迁移后：

![1773200490788](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175417704-891020538.png)

效果：

![1773200606409](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175421273-1654607214.gif)

### 给Hand加一个基于移动速度的Z轴偏移量

![1773213367307](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175422109-1984636337.png)

![1773213379910](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175422617-1868972791.png)

### 手臂摆动的中轴向前偏移一定距离

![1773214144609](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175423266-676618092.png)

因此C++改动为：

```cpp
#pragma region 计算ArmMotion的Effector的RotationAmount值
	FRigUnit_GetArmMotionEffectorRotationAmount_Execute()
	{
		const float PerFootCyclePercent = PerFootCyclePercentArray[ArmIndex];
		//摆动的幅度
		const float ArmSwingAmplitude = MathFloatRemap(
										RigSpaceVelocity.Length(),
										0,
										500,
										0,
										45,
										true
										);
		//摆动的中轴向前偏移量
		const float ArmSwingAxisOffset = MathFloatRemap(
										RigSpaceVelocity.Length(),
										0,
										500,
										0,
										20,
										true
										);
		//ArmSwingAmplitude * sin( 2Π * (PerFootCyclePercent+0.25)%1 ) + ArmSwingAxisOffset
		const float ArmSwingCurve = sin(2 * UE_PI * FMath::Fmod(PerFootCyclePercent + 0.25f, 1.0f))
									* ArmSwingAmplitude
									+ ArmSwingAxisOffset;

		RotateAmount = AnimationCore::QuatFromEuler(FVector(ArmSwingCurve, 0, 0));
	}
```

效果：

![1773214682336](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175426189-1497440140.gif)

### 修复：后退时同手同脚

![1773216593401](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175427063-1315202448.png)

因此C++改动为：

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
		FQuat MovementAngleOffset;	//新增

		UPROPERTY(meta = (Output))
		FQuat RotateAmount;
	};
```

```cpp
#pragma region 计算ArmMotion的Effector的RotationAmount值
	FRigUnit_GetArmMotionEffectorRotationAmount_Execute()
	{
		const float PerFootCyclePercent = PerFootCyclePercentArray[ArmIndex];
		//摆动的正负号(向后摆动时需要乘-1)
		const float ArmSwingSign = FVector::DotProduct(
			RigSpaceVelocity.GetSafeNormal(),
			MovementAngleOffset.RotateVector(FVector(0,1,0) )
			);
		//摆动的幅度
		const float ArmSwingAmplitude = MathFloatRemap(
										RigSpaceVelocity.Length(),
										0,
										500,
										0,
										30,
										true
										);
		//摆动的中轴向前偏移量
		const float ArmSwingAxisOffset = MathFloatRemap(
										RigSpaceVelocity.Length(),
										0,
										500,
										0,
										20,
										true
										);
		//Sign * ArmSwingAmplitude * sin( 2Π * (PerFootCyclePercent+0.25)%1 ) + ArmSwingAxisOffset
		const float ArmSwingCurve = sin(2 * UE_PI * FMath::Fmod(PerFootCyclePercent + 0.25f, 1.0f))
									* ArmSwingSign
									* ArmSwingAmplitude
									+ ArmSwingAxisOffset;

		RotateAmount = AnimationCore::QuatFromEuler(FVector(ArmSwingCurve, 0, 0));
	}
```

![1773216966087](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175427535-1615953907.png)

效果：

![1773216940096](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175432043-2011335949.gif)

### 让向后移动时的手臂摆动幅度更小

![1773221070305](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175432975-34716699.png)

因此C++改动为：

```cpp
#pragma region 计算ArmMotion的Effector的RotationAmount值
	FRigUnit_GetArmMotionEffectorRotationAmount_Execute()
	{
		const float PerFootCyclePercent = PerFootCyclePercentArray[ArmIndex];
		//摆动的正负号(向后摆动时需要乘-1)

		//向后摆动时的幅度小一些
		const float ArmSwingSignClamp = FMath::Clamp(ArmSwingSign,-0.5,1);

		//摆动的幅度

		//摆动的中轴向前偏移量

		//ArmSwingSignClamp * ArmSwingAmplitude * sin( 2Π * (PerFootCyclePercent+0.25)%1 ) + ArmSwingAxisOffset
		const float ArmSwingCurve = sin(2 * UE_PI * FMath::Fmod(PerFootCyclePercent + 0.25f, 1.0f))
									* ArmSwingSignClamp 
									* ArmSwingAmplitude
									+ ArmSwingAxisOffset;

		RotateAmount = AnimationCore::QuatFromEuler(FVector(ArmSwingCurve, 0, 0));
	}
```

### 修复：手臂摆动小幅度向两侧打圈

把周期偏移0.25改为0.15

```cpp
		//Sign * ArmSwingAmplitude * sin( 2Π * (PerFootCyclePercent+0.15)%1 ) + ArmSwingAxisOffset
		const float ArmSwingCurve = sin(2 * UE_PI * FMath::Fmod(PerFootCyclePercent + 0.15f, 1.0f))
									* ArmSwingSignClamp
									* ArmSwingAmplitude
									+ ArmSwingAxisOffset;
```

### 基于速度的肩膀晃动

![1773220242746](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175433770-546420819.png)

-0.25是为了让肩膀晃动与跑姿同步：

* **左脚落地** → **左肩往下**
* **右脚落地** → **右肩往下**

![1773222423021](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175434468-402116629.png)

迁移到C++：

```cpp
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
```

```cpp
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
```

![1773222674642](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175435146-1914132678.png)

效果：

![1773222804874](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260311175443505-839307495.gif)

## Tweaks fixes and improvements

> 一些细节调整

### 移动时身体倾斜

![1773300319602](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260312164221144-1250011018.png)

![1773300304976](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260312164222176-299544763.png)

迁移到C++

```cpp
#pragma region 身体倾斜
	//身体倾斜
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
```

效果：

![1773303948948](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260312164225823-2025886230.gif)

### 身体随着不同脚部迈出而左右侧倾

![1773386748922](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260313195836967-325238102.png)

![1773386765378](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260313195837938-1610869679.png)

效果：

![1773387043254](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260313195846780-2020349971.gif)

### 修复：奔跑时由于脚部落点位置落后导致后腿绷直

FinalLegIK中的Clamp Sphere的圆心应该放在大腿Thigh处，而且这个Thigh的Transform不应该为初始值

![1773394735431](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260313195848110-1583942563.png)

### 修复：移动速度突变时脚部落点更新滞后A

#### 脚部锁定位置加一个Clamp Sphere限制

![1773394377776](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260313195848677-309345910.png)

![1773394256303](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260313195849328-751677224.png)

#### 增大Lerp速度

![1773402015705](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260313195849740-1005519118.png)

#### 脚在地面停留的时间：速度越快，停留在地面的时间越长

![1773402829485](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260313195850298-1548534128.png)

### 修复：身体前后倾斜速度以及手臂运动速度过快

新建一个Lerp速度更慢的RigSpaceVelocity

![1773648231733](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316172332449-213551652.png)

![1773532215013](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316072522435-901092649.png)

![1773532902670](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316072522651-1977857137.png)

效果：

![1773648411694](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316172337667-1722725401.gif)

### 修复：移动方向改变时腿部交叉

> 这个也是als的通病

FootMovementAngleOffsetLimit变量名字修改为FootRotationFactor：

![1773534211490](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316072522919-1057461561.png)

![1773534296191](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316072523095-2027907429.png)

可以发现当这个值为1时，脚部交叉现象几乎没了

因此，我们需要让FootRotationFactor的值是动态的

![1773546770830](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316072523320-741483704.png)

![1773651943485](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316172338671-1356333647.png)

迁移到C++：

```cpp
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
```

```cpp
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

```

效果：

![1773547733525](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316072524024-514489791.gif)

![1773652262046](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316172348480-1109096956.gif)

### 让膝盖的朝向略微受MovementAngleOffset影响

![1773549842616](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316072524380-188438509.png)

### 脚部适应斜坡角度

![1773654387279](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316183604404-1640969451.png)

效果：

![1773553602209](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316072524908-1267052637.png)

### 身体向下偏移量固定值改为向下射线的距离

![1773600686425](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316072525135-1640255676.png)

![1773600894239](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316072525363-542701322.png)

### 修复：InputPose更换蹲姿时膝盖没有分开

初始的FootPole朝向Vector数组：

![1773630484110](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316172350565-1034170494.png)

![1773630602623](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316172351340-1427504470.png)

把原先SetFinalLegIK中的RotateVector固定值(0,10000,0)换为该数组

![1773630424222](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316172351934-782040794.png)

效果：

![1773631123250](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316172352732-73618618.png)

### 修改：ArmMotion中的肘部Pole朝向Vector也改为初始姿态的默认值

![1773630780513](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316172353195-455587090.png)

![1773631076327](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316172353880-1210360587.png)

### 让脚部预测落点移动速度更加平滑

![1773632470216](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316172354678-2001591475.png)

### 修改VectorLerp函数逻辑：

在即将到达目标值时减慢速度

![1773632703264](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316172355308-1272951942.png)

C++的修改：

```cpp
FVector VectorLerpIndependentOnFrameRate(FVector InVector, FVector TargetVector, float BlendSpeed, float DeltaTime)
{
	const float LerpFactor = FMath::Clamp<float>(BlendSpeed * DeltaTime, 0, 1);
	FVector DeltaVector = (TargetVector - InVector) * LerpFactor;
	return InVector + DeltaVector;
}
```

所有用到该函数的地方需要更改BlendSpeed的值

### 侧向移动时的ArmMotion

#### 手臂前后摆动受移动角度偏移的影响少一些

![1773649932932](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316172355767-2002061753.png)

#### 摆动幅度受速度沿正前方分量的影响，斜向移动时这个分量小，映射之后手臂摆动幅度会变小

![1773650041918](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316172356243-611330688.png)

因此，C++的改动：

```cpp
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
										500,
										0,
										50,
										true
										)
										* FMath::Clamp(FMath::Abs(RigSpaceVelocity.Dot(FVector(0,0.4,0))), 0.4, 1);
```

效果：

![1773651714895](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316172407461-744881468.gif)

### 增加：上半身的左右摆动

修改前：上半身没有左右摆动

![1773652809091](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316172410003-865339448.gif)

修改后：

![1773652882659](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316172412307-1515247235.gif)

## Improved foot traces and foot avoidance

> 优化脚部轨迹追踪、避免脚部交叉

### 修复：从高处落下时，脚部位置延后导致轨迹异常

> 解决方法：

1.预测脚部落点的节点中，如果Sphere Trace没有击中，那么沿用以前的值

![1773657318473](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316183605290-2140226576.png)

2.限制脚的高度

![1773657789360](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316184850650-1544869834.png)

效果：

![1773657973728](https://img2024.cnblogs.com/blog/3614909/202603/3614909-20260316184854549-274501316.gif)

### 当两只脚踩在不同高度的平面时，按照更低的脚的追踪轨迹做PelvisOffset
