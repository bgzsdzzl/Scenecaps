# 快速教程（1.0.0）

本教程用最短路径跑通「创建集合 → 添加条目 → 变体 → 放入关卡 → 批量实例化」。

## 1. 启用插件

将插件放到工程的 `Plugins/Scenecaps/`，重新生成工程文件，编译后在编辑器插件面板启用 Scenecaps。

## 2. 创建场景集合资产

在内容浏览器中新建 **Scene Collection**（`UScenecapsAsset`），命名例如 `SC_Forest`。

## 3. 添加条目

1. 双击资产打开 Scenecaps 编辑器。
2. 从内容浏览器拖拽 Static Mesh、Blueprint Actor 等到视口或 Outliner。
3. 在 Details 面板调整 `RelativeTransform`、`bVisible`、材质覆盖与自定义参数。
4. 需要层级时，在 Outliner 中把这些条目挂到父条目下。

## 4. 创建变体

1. 在资产的 `Variants` 数组中新增一项并命名（如 `Snow`）。
2. 在 `Overrides` 里只填写需要改变的字段，并勾选对应的 `bOverrideXxx`。
3. 变体是稀疏覆盖：未覆盖的字段沿用基础条目。

## 5. 放入关卡

1. 把 **Scene Collection Actor**（`AScenecapsActor`）拖入关卡。
2. 将 `CollectionAsset` 指向 `SC_Forest`。
3. 编辑器中会自动重建；也可手动调用 `Rebuild()`。
4. 切换 `ActiveVariant` 即可看到变体切换效果。

## 6. 变体平滑过渡

1. 给场景 Actor 添加 **Scenecaps Variant Transition** 组件。
2. 设置 `TransitionAsset`、`FromVariant`、`ToVariant`、`Duration`。
3. 勾选 `bAutoPlay` 或调用 `Play()` / `PlayFromTo()`。

## 7. 批量实例化（可选）

在蓝图或 C++ 中调用：

```cpp
UInstancedStaticMeshComponent* ISM =
    Asset->SpawnCollectionAsISM(WorldContextActor, FTransform::Identity, TEXT("Snow"));
```

生成时会把场景 Actor 与批量容器关联，变体过渡可精确定位到每个实例的条目来源。

## 8. 视口 HDRI 天空盒（可选）

1. 打开任意 Scenecaps 资产编辑器，视口顶部有 `HDRI Skybox` 工具条。
2. 点击 `Select HDRI...`，从内容浏览器选择一个 `TextureCube`。
3. 选择后自动启用，取消复选框或清除贴图即关闭。默认关闭，配置会持久保存。

更多细节见 [Usage.md](Usage.md) 与 [API.md](API.md)。
