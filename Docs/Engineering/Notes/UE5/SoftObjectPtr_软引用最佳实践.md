---
note_id: UE5-002
title: SoftObjectPtr软引用最佳实践
category: UE5
status: Active
created: 2025-11-06
updated: 2025-11-06
---

# SoftObjectPtr软引用最佳实践

## 核心要点

- TSoftObjectPtr用于延迟加载资源
- 可安全序列化到SaveGame
- 需要手动加载，支持同步和异步
- 使用IsValid()判断是否已加载，IsNull()判断是否为空引用

## 基本用法

声明：
```cpp
UPROPERTY()
TSoftObjectPtr<UTexture2D> MyTexture;

UPROPERTY()
TSoftObjectPtr<UStaticMesh> MyMesh;
```

设置引用：
```cpp
MyTexture = TSoftObjectPtr<UTexture2D>(SomeTexturePointer);

// 或使用路径
MyTexture = TSoftObjectPtr<UTexture2D>(
    FSoftObjectPath(TEXT("/Game/Textures/MyTexture.MyTexture")));
```

## 检查状态

```cpp
// 检查是否为空引用
if (MyTexture.IsNull())
{
    // 未设置任何引用
}

// 检查是否已加载到内存
if (MyTexture.IsValid())
{
    UTexture2D* Texture = MyTexture.Get();
    // 使用Texture
}

// 检查有引用但未加载
if (!MyTexture.IsNull() && !MyTexture.IsValid())
{
    // 需要加载
}
```

## 同步加载

```cpp
UTexture2D* LoadTextureSync()
{
    if (MyTexture.IsNull())
        return nullptr;

    if (MyTexture.IsValid())
        return MyTexture.Get();

    // 同步加载（会阻塞）
    UTexture2D* LoadedTexture = MyTexture.LoadSynchronous();
    return LoadedTexture;
}
```

注意：
- 同步加载会阻塞游戏线程
- 仅在必要时使用（如关卡加载时）
- 避免在Tick或频繁调用的函数中使用

## 异步加载

```cpp
void LoadTextureAsync()
{
    if (MyTexture.IsNull())
        return;

    if (MyTexture.IsValid())
    {
        OnTextureLoaded(MyTexture.Get());
        return;
    }

    FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

    Streamable.RequestAsyncLoad(
        MyTexture.ToSoftObjectPath(),
        FStreamableDelegate::CreateUObject(this, &UMyClass::OnTextureLoadComplete)
    );
}

void OnTextureLoadComplete()
{
    if (MyTexture.IsValid())
    {
        UTexture2D* LoadedTexture = MyTexture.Get();
        OnTextureLoaded(LoadedTexture);
    }
}
```

## Lambda异步加载

```cpp
void LoadTextureAsyncLambda()
{
    if (MyTexture.IsNull()) return;
    if (MyTexture.IsValid())
    {
        UseTexture(MyTexture.Get());
        return;
    }

    FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

    // 捕获弱引用避免循环引用
    Streamable.RequestAsyncLoad(
        MyTexture.ToSoftObjectPath(),
        FStreamableDelegate::CreateLambda([WeakRef = MyTexture]()
        {
            if (WeakRef.IsValid())
            {
                UTexture2D* LoadedTexture = WeakRef.Get();
                // 使用LoadedTexture
            }
        })
    );
}
```

## 批量异步加载

```cpp
void LoadMultipleAssets()
{
    TArray<FSoftObjectPath> AssetsToLoad;
    AssetsToLoad.Add(MyTexture.ToSoftObjectPath());
    AssetsToLoad.Add(MyMesh.ToSoftObjectPath());
    AssetsToLoad.Add(MyMaterial.ToSoftObjectPath());

    FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

    Streamable.RequestAsyncLoad(
        AssetsToLoad,
        FStreamableDelegate::CreateUObject(this, &UMyClass::OnAssetsLoaded)
    );
}
```

## 常见错误

错误：直接使用Get()不检查IsValid()
```cpp
UTexture2D* Texture = MyTexture.Get();  // 可能返回nullptr
```

正确：
```cpp
if (MyTexture.IsValid())
{
    UTexture2D* Texture = MyTexture.Get();
    // 使用Texture
}
```

错误：混淆IsNull()和IsValid()
```cpp
if (!MyTexture.IsNull())  // 有引用但可能未加载
{
    UTexture2D* Texture = MyTexture.Get();  // 可能是nullptr
}
```

正确：
```cpp
if (MyTexture.IsValid())  // 已加载到内存
{
    UTexture2D* Texture = MyTexture.Get();  // 保证非nullptr
}
```

## 使用场景

适合使用TSoftObjectPtr：
- SaveGame序列化
- 延迟加载的UI资源
- 条件加载的特效/音效
- 大量可选资源（如皮肤、装饰）

不适合使用：
- 必须立即使用的资源
- 频繁访问的资源（应硬引用）
- 网络同步的资源（同步困难）

## 相关笔记

- [SaveGame序列化规范](./SaveGame_序列化规范.md)
- [跨平台注意事项](./CrossPlatform_跨平台注意事项.md)

## 来源

- DevelopmentLogs/2025-11/2025-11-01_SaveGame跨平台崩溃排查.md
- UE官方文档：Asset Management
