---
note_id: UE5-003
title: 跨平台注意事项
category: UE5
status: Active
created: 2025-11-06
updated: 2025-11-06
---

# 跨平台注意事项

## 核心要点

- 未初始化内存在不同平台表现不同
- Windows上堆内存常被清零，macOS上是随机值
- 跨平台差异常暴露隐藏的Bug
- 必须显式初始化所有成员变量

## 内存初始化差异

Windows行为：
```cpp
struct FMyStruct
{
    int32 Value;  // 未初始化
    UObject* Pointer;  // 未初始化
};

FMyStruct* Data = new FMyStruct();
// Windows: Value可能是0, Pointer可能是nullptr
```

macOS行为：
```cpp
FMyStruct* Data = new FMyStruct();
// macOS: Value是随机值, Pointer是野指针
```

## 正确的初始化

结构体初始化：
```cpp
USTRUCT()
struct FMyStruct
{
    GENERATED_BODY()

    UPROPERTY()
    int32 Value = 0;  // 方法1：声明时初始化

    UPROPERTY()
    UObject* Pointer = nullptr;

    FMyStruct()  // 方法2：构造函数初始化
        : Value(0)
        , Pointer(nullptr)
    {
    }
};
```

类成员初始化：
```cpp
UCLASS()
class UMyClass : public UObject
{
    GENERATED_BODY()

public:
    UMyClass()
    {
        Value = 0;
        Pointer = nullptr;
    }

private:
    int32 Value;
    UObject* Pointer;
};
```

## SaveGame跨平台问题

问题示例：
```cpp
USTRUCT()
struct FSaveSlotItem
{
    GENERATED_BODY()

    UPROPERTY()
    UTexture2D* SaveSlotImage;  // 未初始化裸指针
};
```

表现：
- Windows：序列化时可能遇到nullptr，看似正常
- macOS：序列化时遇到野指针，调用GetPathName()崩溃

解决方案：
```cpp
USTRUCT()
struct FSaveSlotItem
{
    GENERATED_BODY()

    UPROPERTY()
    TSoftObjectPtr<UTexture2D> SaveSlotImage;  // 软引用自动初始化

    FSaveSlotItem()  // 显式构造
    {
        // SaveSlotImage自动初始化为null
    }
};
```

## 调试技巧

开启日志检测未初始化：
```
LogClass, LogSerialization
```

警告信息：
```
ObjectProperty XXX is not initialized properly
```

使用内存检测工具：
- Windows: Visual Studio内存诊断
- macOS: Xcode Address Sanitizer
- Linux: Valgrind

## 常见陷阱

陷阱1：依赖默认零初始化
```cpp
int32 Counter;  // 错误：未初始化
if (Counter == 0)  // Windows可能成立，macOS必错
{
    // ...
}
```

陷阱2：未初始化布尔值
```cpp
bool bIsActive;  // 错误：未初始化
if (bIsActive)  // 随机为true或false
{
    // ...
}
```

陷阱3：裸指针未置空
```cpp
UObject* MyObject;  // 错误：未初始化
if (MyObject)  // 可能是野指针
{
    MyObject->DoSomething();  // 崩溃
}
```

## 最佳实践

规则1：所有成员变量必须初始化
```cpp
// 结构体：声明时或构造函数
USTRUCT()
struct FMyStruct
{
    GENERATED_BODY()

    int32 Value = 0;
    bool bFlag = false;
    UObject* Pointer = nullptr;
};
```

规则2：SaveGame使用软引用
```cpp
// 不使用裸UObject指针
TSoftObjectPtr<UTexture2D> MyTexture;
FSoftObjectPath MyAssetPath;
```

规则3：构造函数初始化列表
```cpp
UMyClass::UMyClass()
    : Value(0)
    , bFlag(false)
    , Pointer(nullptr)
{
}
```

规则4：多平台测试
- 开发时在不同平台编译运行
- CI/CD包含多平台构建
- 重点测试SaveGame和序列化逻辑

## 相关笔记

- [SaveGame序列化规范](./SaveGame_序列化规范.md)
- [SoftObjectPtr软引用最佳实践](./SoftObjectPtr_软引用最佳实践.md)

## 来源

- DevelopmentLogs/2025-11/2025-11-01_SaveGame跨平台崩溃排查.md
- 实践经验总结
