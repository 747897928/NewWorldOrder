---
note_id: UE5-001
title: SaveGame序列化规范
category: UE5
status: Active
created: 2025-11-06
updated: 2025-11-06
---

# SaveGame序列化规范

## 核心要点

- SaveGame结构体和类不能包含裸UObject指针
- 必须使用TSoftObjectPtr或FSoftObjectPath
- 违反此规则会导致跨平台崩溃
- Windows上可能偶发正常（堆内存清零），macOS必现崩溃

## 错误示例

```cpp
USTRUCT()
struct FSaveSlotItem
{
    GENERATED_BODY()

    UPROPERTY()
    UTexture2D* SaveSlotImage;  // 错误：裸指针
};
```

错误原因：
- SaveGame序列化会尝试写入UObject路径
- 裸指针可能是野指针或未初始化
- 序列化时调用GetPathName()崩溃

## 正确示例

```cpp
USTRUCT()
struct FSaveSlotItem
{
    GENERATED_BODY()

    UPROPERTY()
    TSoftObjectPtr<UTexture2D> SaveSlotImage;  // 正确：软引用

    FSaveSlotItem()
    {
        // 显式初始化
    }
};
```

## 保存时处理

```cpp
void SaveData()
{
    // 保存前清空运行时资源引用
    ensure(SaveData.SaveSlotImage.IsNull());

    // 或者显式设置软引用
    SaveData.SaveSlotImage = TSoftObjectPtr<UTexture2D>(SomeTexture);

    UGameplayStatics::SaveGameToSlot(SaveData, SlotName, 0);
}
```

## 加载时处理

```cpp
void LoadData()
{
    UMySaveGame* SaveData = Cast<UMySaveGame>(
        UGameplayStatics::LoadGameFromSlot(SlotName, 0));

    if (SaveData)
    {
        // 检查是否已加载
        if (SaveData->SaveSlotImage.IsValid())
        {
            UTexture2D* Texture = SaveData->SaveSlotImage.Get();
            // 使用Texture
        }
        // 异步加载
        else if (!SaveData->SaveSlotImage.IsNull())
        {
            // 参见Notes/UE5/SoftObjectPtr_软引用最佳实践.md
        }
    }
}
```

## 常见错误

错误：保存编辑器资源到SaveGame
- 症状：运行时加载失败，路径找不到
- 原因：编辑器资源路径在打包后无效
- 解决：只保存游戏运行时资源，或使用资源ID

错误：未初始化结构体成员
- 症状：跨平台表现不一致
- 原因：未初始化内存在不同平台值不同
- 解决：提供显式默认构造函数

## 检测方法

开启日志：
```
LogClass, LogSerialization
```

警告信息：
```
ObjectProperty XXX is not initialized properly
```

## 相关笔记

- [SoftObjectPtr软引用最佳实践](./SoftObjectPtr_软引用最佳实践.md)
- [跨平台注意事项](./CrossPlatform_跨平台注意事项.md)

## 来源

- DevelopmentLogs/2025-11/2025-11-01_SaveGame跨平台崩溃排查.md
- UE官方文档：SaveGame系统
