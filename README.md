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
- 如果希望像箱子这类容器一样按需显示UI，可以关闭bAutoCreateInventoryWidget，然后在需要时调用CreateInventoryWidget。
- 如果容器UI由外部窗口统一管理，可以先在外部创建UInv_InventoryWidgetBase，再调用BindInventoryWidget进行绑定；关闭窗口时调用UnbindInventoryWidget即可。

## 槽位同步

- FInv_RealItemData 现在会同步 SlotIndex，表示该物品当前所在的库存槽位。
- UI在创建后会先读取当前库存快照，再依据SlotIndex将物品放入正确的格子中，因此不再依赖“Widget必须在一开始就存在”。

## 自定义物品与数据

1. 创建FInv_VirtualItemData的DataTable
2. 在C++中自定义FInv_VirtualItemFragment和FInv_RealItemFragment的派生类，可以包含任何类型的数据
3. 在1中的DataTable中添加行，指定物品的GameplayTag，以及对应的Fragments(Real or Virtual)数组
4. 在UInv_ItemComponent中指定该行
