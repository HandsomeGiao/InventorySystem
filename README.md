# Multiplayer 库存与物品系统

## 关键组件介绍

- UInv_InventoryBase : 将其添加到能够保存物品的Actor上，内部可配置Slots数量，库存窗口类等
- UInv_ItemComponent : 将其添加到代表一个物品的Actor上，内部可配置所代表的物品，以及数量等
- UInv_InventoryWidgetBase : 代表库存UI窗口，可以由UInv_InventoryBase自动创建，也可以在外部创建后再绑定
- UInv_ItemOptionsWidget : 对物品进行操作的窗口类，例如Split，Drop等操作。
- UInv_InventoryEntry : 代表了一个可以存储物品的窗口类，存储的物品信息最终保存在这里。

## 如何使用此插件

1. 创建UInv_ItemOptionsWidget，UInv_InventoryEntry的蓝图派生类。
2. 创建UInv_InventoryWidgetBase的派生类，并且在其中配置步骤1,2中创建的类。
3. 创建UInv_InventoryBase的派生类，将其添加到PlayerCharacter或者容器Actor的组件中，并在其中配置步骤2中创建的类。
4. 在对应的物品Actor中添加UInv_ItemComponent组件，并在其中配置对应的物品属性。
5. 在自定义检测逻辑，对检测到的物品Actor调用UInv_InventoryBase::ServerPickupItem函数

## UI绑定方式

- 默认情况下，UInv_InventoryBase会在BeginPlay时根据InventoryWidgetClass自动创建并初始化库存UI。
- 如果希望像箱子这类容器一样按需显示UI，可以关闭bAutoCreateInventoryWidget，然后在需要时调用CreateInventoryWidget或CreateInventoryWidgetByType。
- UInv_InventoryBase现在支持按EInv_InventoryWidgetType管理多个窗口实例；同一种类型在同一个LocalPlayer下同一时间只会保留一个实例，但不同类型可以同时存在。
- UInv_InventoryBase提供了InventoryOwnerType，用来区分该Inventory属于玩家、容器还是其它Actor；其中玩家Inventory会在运行时根据观察者自动解析成PlayerSelf或PlayerOther。
- 可以通过DefaultInventoryWidgetType指定旧接口对应的默认窗口类型，并通过InventoryWidgetConfigs为不同类型分别配置WidgetClass。
- 如果容器UI由外部窗口统一管理，可以先在外部创建UInv_InventoryWidgetBase，再调用BindInventoryWidget或BindInventoryWidgetByType进行绑定；关闭窗口时调用对应的Unbind/Destroy接口即可。

## 角色交互

- AInventorySystemVCharacter 会继续使用屏幕中心射线检测可拾取物品。
- 当没有检测到可拾取物品时，现有的PickupAction会改为检测角色附近的其它Inventory，并切换对应窗口的开关状态。
- 角色自身的Inventory在构造函数中会默认标记为Player；箱子等容器Actor需要把自己的InventoryOwnerType配置为Container。
- 对于玩家Inventory，本地拥有者看到的是PlayerSelf窗口，其他玩家看到的是PlayerOther窗口。

## 槽位同步

- FInv_RealItemData 现在会同步 SlotIndex，表示该物品当前所在的库存槽位。
- UI在创建后会先读取当前库存快照，再依据SlotIndex将物品放入正确的格子中，因此不再依赖“Widget必须在一开始就存在”。

## 拖拽

- UInv_InventoryEntry 现在重新支持左键拖拽。
- 拖拽既可以发生在同一个Inventory内部，也可以发生在不同Inventory之间。
- 拖拽到空槽位时会移动物品。
- 拖拽到同类且可堆叠的物品上时会优先进行堆叠。
- 拖拽到其它物品上时会尝试交换两个槽位中的物品；跨Inventory交换时也会检查目标Inventory的类型过滤配置。
- 跨Inventory拖拽时，UI会自动选择一个客户端可发送RPC的Inventory组件代发请求，因此常见的“玩家背包 <-> 箱子”场景可以直接工作。

## 自定义物品与数据

1. 创建FInv_VirtualItemData的DataTable
2. 在C++中自定义FInv_VirtualItemFragment和FInv_RealItemFragment的派生类，可以包含任何类型的数据
3. 在1中的DataTable中添加行，指定物品的GameplayTag，以及对应的Fragments(Real or Virtual)数组
4. 在UInv_ItemComponent中指定该行
