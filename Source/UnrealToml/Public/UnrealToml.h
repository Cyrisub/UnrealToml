// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FUnrealTomlModule : public IModuleInterface
{};

struct FTomlFileImpl;

namespace Toml
{
    template<typename T>
    concept CValueType =
        std::is_same_v<T, bool> ||
        std::is_same_v<T, int64> ||
        std::is_same_v<T, double> ||
        std::is_same_v<T, int32> ||
        std::is_same_v<T, float>;

    template<typename T>
    concept CStringType =
        std::is_same_v<T, FString>;

    template<typename T>
    concept CSupportedType =
        CValueType<T> ||
        CStringType<T>;
}

class FTomlTable final
{
public:
    // Factory methods - only way to create instances from outside
    static FTomlTable LoadFile(const FString& FilePath);
    static FTomlTable LoadString(const FString& Content);

    // Allow copying
    FTomlTable(const FTomlTable& Other);
    FTomlTable& operator=(const FTomlTable& Other);
    ~FTomlTable() = default;

    bool IsValid() const;
    bool IsEmpty() const;

    // Basic value getters - Checked variants (with checkf assertion)
    template<typename T> requires Toml::CSupportedType<T>
    T Get(const FString& Key) const;

    // Basic value getters - with defaults
    template<typename T> requires Toml::CSupportedType<T>
    T Get(const FString& Key, T Default) const;
    
    // Non-template getters
    bool GetBool(const FString& Key) const { return Get<bool>(Key); }
    int32 GetInt(const FString& Key) const { return Get<int32>(Key); }
    float GetFloat(const FString& Key) const { return Get<float>(Key); }
    FString GetString(const FString& Key) const { return Get<FString>(Key); }
    bool GetBool(const FString& Key, bool Default) const { return Get(Key, Default); }
    int32 GetInt(const FString& Key, int32 Default) const { return Get(Key, Default); }
    double GetFloat(const FString& Key, double Default) const { return Get(Key, Default); }
    FString GetString(const FString& Key, const FString& Default) const { return Get(Key, Default); }
    
    // Array getters - Checked variants
    template<typename T> requires Toml::CSupportedType<T>
    TArray<T> GetHomoArray(const FString& Key) const;
    
    // Path-based access - Checked variants
    template<typename T> requires Toml::CSupportedType<T>
    T AtPath(const FString& Path) const;
    
    // Path-based access - with defaults
    template<typename T> requires Toml::CSupportedType<T>
    T AtPath(const FString& Path, T Default) const;

    // Non-template getters
    bool AtPathBool(const FString& Path) const { return AtPath<bool>(Path); }
    int32 AtPathInt(const FString& Path) const { return AtPath<int32>(Path); }
    float AtPathFloat(const FString& Path) const { return AtPath<float>(Path); }
    FString AtPathString(const FString& Path) const { return AtPath<FString>(Path); }
    bool AtPathBool(const FString& Path, bool Default) const { return AtPath(Path, Default); }
    int32 AtPathInt(const FString& Path, int32 Default) const { return AtPath(Path, Default); }
    double AtPathFloat(const FString& Path, double Default) const { return AtPath(Path, Default); }
    FString AtPathString(const FString& Path, const FString& Default) const { return AtPath(Path, Default); }
    
    // Table operations
    bool HasKey(const FString& Key) const;
    TArray<FString> GetKeys() const;
    
    // Table getters
    FTomlTable GetTable(const FString& Key) const;
    FTomlTable GetTableAtPath(const FString& Path) const;

private:
    // Private constructor - only used internally
    FTomlTable();
    void Init();
    
    TUniquePtr<FTomlFileImpl> Impl;
};
