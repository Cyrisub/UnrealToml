#include "UnrealToml.h"

#include "Logging/LogMacros.h"
#include "Misc/FileHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogToml, Log, All);

IMPLEMENT_MODULE(FUnrealTomlModule, UnrealToml)

#define TOML_EXCEPTIONS 0
#define TOML_ENABLE_WINDOWS_COMPAT 0
#include "toml.hpp"

struct FTomlFileImpl
{
    toml::table tbl;
};

FTomlTable::FTomlTable()
    : Impl(nullptr)
{
}

void FTomlTable::Init()
{
    Impl = new FTomlFileImpl{};
}

FTomlTable::FTomlTable(const FTomlTable& Other)
    : Impl(new FTomlFileImpl{})
{
    if (Other.Impl)
    {
        Impl->tbl = Other.Impl->tbl;
    }
}

FTomlTable& FTomlTable::operator=(const FTomlTable& Other)
{
    if (this != &Other)
    {
        if (!Impl)
        {
            Impl = new FTomlFileImpl{};
        }
        
        if (Other.Impl)
        {
            Impl->tbl = Other.Impl->tbl;
        }
        else
        {
            Impl->tbl = toml::table{};
        }
    }
    return *this;
}

FTomlTable::~FTomlTable()
{
    delete Impl;
}

bool FTomlTable::IsValid() const
{
    return Impl != nullptr;
}

bool FTomlTable::IsEmpty() const
{
    return IsValid() && Impl->tbl.empty();
}

namespace Toml
{
    template<typename T>
    struct UETypeToNativeType { using Type = T; };
    template<>
    struct UETypeToNativeType<int32> { using Type = int64; };
    template<>
    struct UETypeToNativeType<float> { using Type = double; };
    template<>
    struct UETypeToNativeType<FString> { using Type = std::string; };

    [[noreturn]] inline void Unreachable()
    {
        // Uses compiler specific extensions if possible.
        // Even if no extension is used, undefined behavior is still raised by
        // an empty function body and the noreturn attribute.
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
        __assume(false);
#else // GCC, Clang
        __builtin_unreachable();
#endif
    }
    
    template<typename T>
    const TCHAR* GetTypeName()
    {
        if constexpr (std::is_same_v<T, bool>)
        {
            return TEXT("bool");
        }
        else if constexpr (std::is_same_v<T, int64>)
        {
            return TEXT("integer");
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            return TEXT("float-point");
        }
        else if (std::is_same_v<T, std::string>)
        {
            return TEXT("string");
        }
        Unreachable();
    }

    bool ParseTomlTable(const TCHAR* Content, toml::table& OutTable, FString& OutError)
    {
        auto ParseResult = toml::parse(TCHAR_TO_UTF8(Content));
        if (!ParseResult)
        {
            const auto& Error = ParseResult.error();
            const auto& Source = Error.source();
            OutError = FString::Printf(TEXT("%s %s(%d:%d, %d:%d)"),
                UTF8_TO_TCHAR(Error.description().data()),
                Source.path ? UTF8_TO_TCHAR(Source.path->data()) : TEXT(""),
                Source.begin.line,
                Source.end.line,
                Source.begin.column,
                Source.end.column);
            return false;
        }

        OutTable = std::move(ParseResult).table();
        return true;
    }
}

FTomlTable FTomlTable::LoadFile(const FString& FilePath)
{
    FTomlTable Result;
    
    FString FileContent;
    if (!FFileHelper::LoadFileToString(FileContent, *FilePath))
    {
        UE_LOG(LogToml, Error, TEXT("Failed to read TOML file: %s"), *FilePath);
        return Result;
    }
    
    FString Error;
    Result.Init();
    if (!Toml::ParseTomlTable(*FileContent, Result.Impl->tbl, Error))
    {
        UE_LOG(LogToml, Error, TEXT("Failed to parse TOML file '%s': %s"), *FilePath, *Error);
        Result.Impl = nullptr;
    }
    
    return Result;
}

FTomlTable FTomlTable::LoadString(const FString& Content)
{
    FTomlTable Result;
    
    FString Error;
    Result.Init();
    if (!Toml::ParseTomlTable(*Content, Result.Impl->tbl, Error))
    {
        UE_LOG(LogToml, Error, TEXT("Failed to parse TOML string: %s"), *Error);
        Result.Impl = nullptr;
    }
    
    return Result;
}

// Basic value getters - Checked variants
template <typename T> requires Toml::CSupportedType<T>
T FTomlTable::Get(const FString& Key) const
{
    using NativeType = typename Toml::UETypeToNativeType<T>::Type;
    checkf(HasKey(Key), TEXT("Key '%s' not found in TOML file"), *Key);
    auto node = Impl->tbl[TCHAR_TO_UTF8(*Key)];
    checkf(node.is<NativeType>(), TEXT("Key '%s' is not a %s"), *Key, Toml::GetTypeName<NativeType>());
    if constexpr (Toml::CValueType<T>)
    {
        return static_cast<T>(node.as<NativeType>()->get());
    }
    else if constexpr (Toml::CStringType<T>)
    {
        auto& str = node.ref<NativeType>();
        return UTF8_TO_TCHAR(str.data());
    }
    Toml::Unreachable();
}

// Basic value getters - with defaults
template <typename T> requires Toml::CSupportedType<T>
T FTomlTable::Get(const FString& Key, T Default) const
{
    using NativeType = typename Toml::UETypeToNativeType<T>::Type;
    auto node = Impl->tbl[TCHAR_TO_UTF8(*Key)];
    if (!node.is<NativeType>())
    {
        return Default;
    }
    if constexpr (Toml::CValueType<T>)
    {
        return static_cast<T>(node.value_or(Default));
    }
    else if constexpr (Toml::CStringType<T>)
    {
        return UTF8_TO_TCHAR(node.value_or(TCHAR_TO_UTF8(*Default)));
    }
    Toml::Unreachable();
}

template <typename T> requires Toml::CSupportedType<T>
TArray<T> FTomlTable::GetHomoArray(const FString& Key) const
{
    using NativeType = typename Toml::UETypeToNativeType<T>::Type;
    checkf(HasKey(Key), TEXT("Key '%s' not found in TOML file"), *Key);
    auto node = Impl->tbl[TCHAR_TO_UTF8(*Key)];
    checkf(node.is_array(), TEXT("Key '%s' is not a array"), *Key);
    checkf(node.is_homogeneous<NativeType>(), TEXT("Key '%s' is not homogeneous"), *Key);

    auto arr = node.as_array();
    TArray<T> Result;
    Result.Reserve(arr->size());
    arr->for_each([&Result](const toml::value<NativeType>& item)
    {
        if constexpr (Toml::CValueType<T>)
        {
            Result.Add(item.get());
        }
        else if constexpr (Toml::CStringType<T>)
        {
            auto& str = item.template ref<std::string>();
            Result.Add(UTF8_TO_TCHAR(str.data()));
        }
        else
        {
            Result.Add(static_cast<T>(item.get()));
        }
    });
    return Result;
}

template <typename T> requires Toml::CSupportedType<T>
T FTomlTable::AtPath(const FString& Path) const
{
    using NativeType = typename Toml::UETypeToNativeType<T>::Type;
    auto node = Impl->tbl.at_path(TCHAR_TO_UTF8(*Path));
    checkf(node.is<NativeType>(), TEXT("Key '%s' is not a %s"), *Path, Toml::GetTypeName<NativeType>());
    if constexpr (Toml::CValueType<T>)
    {
        return static_cast<T>(node.as<NativeType>()->get());
    }
    else if constexpr (Toml::CStringType<T>)
    {
        auto& str = node.ref<NativeType>();
        return UTF8_TO_TCHAR(str.data());
    }
    Toml::Unreachable();
}

template <typename T> requires Toml::CSupportedType<T>
T FTomlTable::AtPath(const FString& Path, T Default) const
{
    auto node = Impl->tbl.at_path(TCHAR_TO_UTF8(*Path));
    if constexpr (Toml::CValueType<T>)
    {
        return static_cast<T>(node.value_or(Default));
    }
    else if constexpr (Toml::CStringType<T>)
    {
        return UTF8_TO_TCHAR(node.value_or(TCHAR_TO_UTF8(*Default)));
    }
    Toml::Unreachable();
}

bool FTomlTable::HasKey(const FString& Key) const
{
    return Impl->tbl.contains(TCHAR_TO_UTF8(*Key));
}

TArray<FString> FTomlTable::GetKeys() const
{
    TArray<FString> Keys;
    if (!Impl) return Keys;
    
    for (const auto& [key, value] : Impl->tbl)
    {
        Keys.Add(UTF8_TO_TCHAR(key.data()));
    }
    return Keys;
}

// Table getters
FTomlTable FTomlTable::GetTable(const FString& Key) const
{
    checkf(Impl && HasKey(Key), TEXT("Key '%s' not found in TOML file"), *Key);
    auto table = Impl->tbl[TCHAR_TO_UTF8(*Key)].as_table();
    checkf(table != nullptr, TEXT("Key '%s' is not a table"), *Key);

    FTomlTable Result;
    Result.Init();
    Result.Impl->tbl = MoveTemp(*table);
    return Result;
}

FTomlTable FTomlTable::GetTableAtPath(const FString& Path) const
{
    checkf(Impl, TEXT("Invalid TOML file"));
    auto node = Impl->tbl.at_path(TCHAR_TO_UTF8(*Path));
    auto table = node.as_table();
    checkf(table != nullptr, TEXT("Path '%s' not found or not a table"), *Path);

    FTomlTable Result{};
    Result.Init();
    Result.Impl->tbl = MoveTemp(*table);
    return Result;
}
