# 行为树（BTs）

---

## 为什么选择BTs

* **结构清晰、逻辑直观**：通过树形结构组织复杂的任务逻辑，比大量 `if/else` 更容易理解和维护
* **模块化、可复用**：可以将复杂任务拆分成多个 `ActionNode`、`ConditionNode` 和 `Subtree`，不同任务之间可以复用
* **易于扩展和修改**：通过调整行为树结构即可改变决策逻辑，不需要大幅修改底层功能代码；配合 Groot2 还可以图形化编辑
* **天然支持状态管理**：节点具有 `SUCCESS`、`FAILURE`、`RUNNING` 三种状态，非常适合机器人这种需要持续执行、不断获取反馈的任务
* **适合异常处理和恢复**：可以通过 `Fallback`、`RecoveryNode` 等机制方便地实现失败检测、重试和恢复行为

---

## 基本概念

### 官方文档 ：

### `https://www.behaviortree.dev/docs/learn-the-basics/BT_basics `

* 一个名为“tick”的信号被发送到树的根节点，然后沿着树的结构传播，直到到达叶节点（leaf node）

* 任何接收到信号（tick）的 TreeNode 都会执行其回调函数。该回调函数必须返回某种结果[^1]

* “RUNNING（运行）”意味着该操作需要更多的时间才能得出有效的结果

* 如果一个 TreeNode 有一个或多个子节点，那么就需要负责传播tick；每种 Node 类型可能有不同的规则来决定是否、何时以及多少次对子节点进行tick[^2]

### 同步节点和异步节点
同步节点（synchronous node）：以原子方式执行，在返回 SUCCESS 或 FAILURE 结果之前会阻塞该进程。就是“单线程”处理一个流下来的tick,在当前节点给出成功or失败的结果前是就在这里干等

异步节点（asynchronous node）：异步操作可能会返回“RUNNING”来表示该操作仍在进行中。就是能接受running这个结果先去tick后面的node

### node的类型
四类：ControlNode 负责“怎么安排别人干活”；DecoratorNode 负责“修改一个节点的行为”；ConditionNode 负责“判断现在是什么情况”；ActionNode 负责“真正去干活”
                    TreeNode
                       │
          ┌─────────┼─────────┐
          ↓                      ↓                      ↓
    ControlNode   DecoratorNode   LeafNode
                                                      /      \
                                                     ↓        ↓
                                                ActionNode  ConditionNode

* ControlNode：通常有多个子节点  → 管别人（我的几个孩子，应该按照什么规则执行？）
		比如 Sequence（顺序）就是一个ControlNode 它规定了它的几个子node按顺序同步执行，从左到右执行，**全部成功才成功**，一个失败就整体失败
	
	​	还有 Fallback（回退/选择器）从左到右尝试，**第一个成功就整体成功**，全部失败才失败[^3]
	
* DecoratorNode：只有一个子节点  → 改自己的子节点的结果

* ActionNode：执行具体动作，没有子节点  → 判断

* ConditionNode：检查（是否满足某一）条件，没有子节点   → 干活


## 黑板（blackboard）与端口（port）

* “黑板”是一种简单的键值（**key -> value**）存储结构，由树中的所有节点共享使用。

  ​	key   →   value		比如       name → xiaoming       age  → 18

  ​	名字      对应的值
  
* “Blackboard”的“条目(entry)”实际上是一个键值对(**key/value pair**)

* 输入端口可以读取黑板上的条目，而输出端口则可以向条目中写入数据           

* `{goal}` `{path}` 花括号是**黑板（Blackboard）变量**——树内节点间传数据的共享内存

---

## Navigation Subtree和Recovery Subtree（导航子树，恢复子树）

* **Navigation Subtree = “正常情况下，怎么完成导航任务”**

* **Recovery Subtree = “正常导航失败/遇到问题后，怎么尝试恢复”**

---

## nav2的BTs实现导航的“原理”

由BT节点去调用一个个server,server里面干活的是各种具体的算法

比如：

* ComputePathToPose 调用Planner Server 里面可能的算法是A*，迪杰斯特拉 等等
* FollowPath 调用Controller Server 里面可能的算法是 MPPI,TEB 等等

---

## 拆解 fzsd_nav 的导航树

（感觉写在文档里比直接写注释要好看一点？）
navigate_to_pose_w_replanning_and_recovery.xml（去掉注释后的骨架）（ai帮助生成骨架图）：

RecoveryNode "NavigateRecovery" (重试10次)          ← 装饰节点：整体失败就重试[^4]
├── PipelineSequence "NavigateWithReplanning"       ← 流水线：并行推进两个任务
│   ├── RateController hz=3.0                       ← 装饰节点：子节点每秒最多 tick 3 次[^5]
│   │   └── RecoveryNode "ComputePathToPose"
│   │       ├── ComputePathToPose  ← 行为：规划路径
│   │       └── ClearEntireCostmap ← 行为：失败时清全局代价地图（恢复动作）
│   └── RecoveryNode "FollowPath"
│       ├── FollowPath             ← 行为：跟随路径
│       └── ClearEntireCostmap     ← 行为：失败时清局部代价地图
└── ReactiveFallback "RecoveryFallback"             ← 反应式回退：随时可以打断上面的流程
    ├── GoalUpdated                ← 条件：目标是否变了
    └── RoundRobin "RecoveryActions"                ← 轮流执行恢复动作
        ├── Sequence (清局部+全局代价地图)
        └── BackUp                 ← 后退

* 这个并不是负责决策的行为树，是对nav2本来就有的负责执行动作的行为树的优化

---

缝合包里src/rm_bt_bridge/config/cod_behavior_pose.yaml 中写的各种点位坐标是COD地图，但是pb仿真的地图不一样，需要ros2 topic echo /clicked_point 来手动标一下坐标再填进去

---

[^5]: RateController（限频）
[^4]: RecoveryNode = 重试 + 恢复子节点
[^3]: Fallback 的子节点顺序 = 优先级顺序
[^2]: tick感觉像是给树节点打勾
[^1]: 结果为 SUCCESS FAILURE RUNNING 中的一种