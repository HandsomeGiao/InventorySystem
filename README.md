# Multiplayer 库存与物品系统

## 关键组件介绍

- UInv_InventoryBase : 将其添加到能够保存物品的Actor上，内部可配置Slots数量，库存窗口类等
- UInv_ItemComponent : 将其添加到代表一个物品的Actor上，内部可配置所代表的物品，以及数量等
- UInv_InventoryWidgetBase : 由UInv_InventoryBase创建，代表库存UI窗口
- UInv_ItemOptionsWidget : 对物品进行操作的窗口类，例如Split，Drop等操作。
- UInv_InventoryEntry : 代表了一个可以存储物品的窗口类，存储的物品信息最终保存在这里。
- UInv_InvDragDrop : 拖拽操作窗口。

## 如何使用此插件

1. 创建UInv_ItemOptionsWidget，UInv_InventoryEntry的蓝图派生类。
2. 创建UInv_InventoryWidgetBase的派生类，并且在其中配置步骤1中创建的类。
3. 创建UInv_InventoryBase的派生类，将其添加到PlayerCharacter的组件中，并在其中配置步骤2中创建的类。
4. 在对应的物品Actor中添加UInv_ItemComponent组件，并在其中配置对应的物品属性。
5. 在自定义检测逻辑，对检测到的物品Actor调用UInv_InventoryBase::ServerPickupItem函数

## 自定义物品与数据

1. 创建FInv_VirtualItemData的DataTable
2. 在C++中自定义FInv_VirtualItemFragment的派生类，可以包含任何类型的数据
3. 在1中的DataTable中添加行，指定物品的GameplayTag，以及对应的FInv_VirtualItemFragment数组
4. 在UInv_ItemComponent中指定该行
