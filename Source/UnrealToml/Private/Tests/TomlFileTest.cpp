#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UnrealToml.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTomlFileBasicTest, "UnrealToml.FTomlFile.Basic", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FTomlFileBasicTest::RunTest(const FString& Parameters)
{
    // Test simple TOML string loading
    const FString TestToml = TEXT(R"(
        title = "TOML Example"
        integer = 42
        float = 3.14
        boolean = true
        
        array = [1, 2, 3]
        strings = ["a", "b", "c"]
        u8strings = ["中文", "👊🀄🔥"]
        
        [table]
        key = "value"
    )");
    
    FTomlTable Toml = FTomlTable::LoadString(TestToml);
    UTEST_VALID_EXPR(Toml);
    
    // Test basic value getters
    UTEST_EQUAL_EXPR(Toml.GetString("title"), TEXT("TOML Example"));
    UTEST_EQUAL_EXPR(Toml.GetInt("integer"), 42);
    UTEST_EQUAL_EXPR(Toml.GetFloat("float"), 3.14f);
    UTEST_EQUAL_EXPR(Toml.GetBool("boolean"), true);
    
    // Test default value getters
    FString Default = TEXT("default");
    UTEST_EQUAL_EXPR(Toml.GetString("non_existent", Default), Default);
    UTEST_EQUAL_EXPR(Toml.GetInt("non_existent", 100), 100);
    UTEST_EQUAL_EXPR(Toml.GetFloat("non_existent", 1.0), 1.0);
    UTEST_EQUAL_EXPR(Toml.GetBool("non_existent", false), false);
    UTEST_NOT_EQUAL_EXPR(Toml.GetString("title", Default), Default);
    UTEST_NOT_EQUAL_EXPR(Toml.GetInt("integer", 100), 100);
    UTEST_NOT_EQUAL_EXPR(Toml.GetFloat("float", 1.0), 1.0);
    UTEST_NOT_EQUAL_EXPR(Toml.GetBool("boolean", false), false);
    
    // Test array getters
    TArray<int32> ExpectedInts = {1, 2, 3};
    TArray<int64> ExpectedNativeInts = {1, 2, 3};
    TArray<FString> ExpectedStrings = {TEXT("a"), TEXT("b"), TEXT("c")};
    TArray<FString> ExpectedUTFStrings = {TEXT("中文"), TEXT("👊🀄🔥")};
    UTEST_EQUAL_EXPR(Toml.GetHomoArray<int32>("array"), ExpectedInts);
    UTEST_EQUAL_EXPR(Toml.GetHomoArray<FString>("strings"), ExpectedStrings);
    UTEST_EQUAL_EXPR(Toml.GetHomoArray<int64>("array"), ExpectedNativeInts);
    UTEST_EQUAL_EXPR(Toml.GetHomoArray<FString>("u8strings"), ExpectedUTFStrings);
    
    // Test table operations
    UTEST_TRUE_EXPR(Toml.HasKey("table"));
    
    FTomlTable Table = Toml.GetTable("table");
    UTEST_TRUE_EXPR(Table.HasKey("key"));
    UTEST_EQUAL_EXPR(Table.GetString("key"), TEXT("value"));
    
    // Test key existence at different levels
    UTEST_TRUE_EXPR(Toml.HasKey("title"));
    UTEST_FALSE_EXPR(Toml.HasKey("non_existent"));
    
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTomlFileLoadTest, "UnrealToml.FTomlFile.FileLoading", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FTomlFileLoadTest::RunTest(const FString& Parameters)
{
    const FString TestFilePath = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("UnrealToml/Source/UnrealToml/Private/Tests/TestFile.toml"));
    
    // Test file loading
    FTomlTable Toml = FTomlTable::LoadFile(TestFilePath);
    UTEST_VALID_EXPR(Toml);
    
    // Verify basic values
    UTEST_EQUAL_EXPR(Toml.GetString("title"), TEXT("Test Config"));
    UTEST_EQUAL_EXPR(Toml.GetInt("version"), 1);
    
    // Test settings table
    UTEST_TRUE_EXPR(Toml.HasKey("settings"));
    UTEST_EQUAL_EXPR(Toml.AtPath<bool>("settings.debug"), true);
    UTEST_EQUAL_EXPR(Toml.AtPath<int32>("settings.max_retries"), 3);
    
    // Test database table
    FTomlTable DbTable = Toml.GetTable("database");
    UTEST_EQUAL_EXPR(DbTable.GetString("host"), TEXT("localhost"));
    UTEST_EQUAL_EXPR(DbTable.GetInt("port"), 5432);
    UTEST_EQUAL_EXPR(DbTable.GetBool("enabled"), true);
    
    // Test array of tables (servers)
    FTomlTable Server1 = Toml.GetTableAtPath("servers[0]");
    FTomlTable Server2 = Toml.GetTableAtPath("servers[1]");
    
    UTEST_EQUAL_EXPR(Server1.GetString("name"), TEXT("primary"));
    UTEST_EQUAL_EXPR(Server1.GetString("ip"), TEXT("192.168.1.1"));
    UTEST_EQUAL_EXPR(Server2.GetString("name"), TEXT("backup"));
    UTEST_EQUAL_EXPR(Server2.GetString("ip"), TEXT("192.168.1.2"));
    
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTomlFilePathTest, "UnrealToml.FTomlFile.PathAccess", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FTomlFilePathTest::RunTest(const FString& Parameters)
{
    const FString TestToml = TEXT(R"(
        [server]
        host = "example.com"
        port = 8080
        
        [database]
        enabled = true
        [database.primary]
        host = "db1.example.com"
        port = 5432
        
        [array_table]
        numbers = [1, 2, 3]
        nested.value = 42
    )");
    
    FTomlTable Toml = FTomlTable::LoadString(TestToml);
    UTEST_VALID_EXPR(Toml);
    
    // Test path-based access
    UTEST_EQUAL_EXPR(Toml.AtPathString("server.host"), TEXT("example.com"));
    UTEST_EQUAL_EXPR(Toml.AtPathInt("server.port"), 8080);
    UTEST_EQUAL_EXPR(Toml.AtPathBool("database.enabled"), true);
    UTEST_EQUAL_EXPR(Toml.AtPathString("database.primary.host"), TEXT("db1.example.com"));
    UTEST_EQUAL_EXPR(Toml.AtPathInt("database.primary.port"), 5432);
    UTEST_EQUAL_EXPR(Toml.AtPathInt("array_table.nested.value"), 42);
    
    // Test path-based access with defaults
    UTEST_EQUAL_EXPR(Toml.AtPathString("non.existent.path", TEXT("default")), TEXT("default"));
    
    // Test table access at path
    FTomlTable DbTable = Toml.GetTableAtPath("database.primary");
    UTEST_EQUAL_EXPR(DbTable.GetString("host"), TEXT("db1.example.com"));
    UTEST_EQUAL_EXPR(DbTable.GetInt("port"), 5432);
    
    return true;
}

// Test for type conversion investigation
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTomlFileTypeConversionTest, "UnrealToml.FTomlFile.TypeConversion", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FTomlFileTypeConversionTest::RunTest(const FString& Parameters)
{
    // Test type conversion behavior
    const FString TestToml = TEXT(R"(
        true_value = true
        false_value = false
        int_value = 42
        float_value = 3.14
    )");
    
    FTomlTable Toml = FTomlTable::LoadString(TestToml);
    UTEST_VALID_EXPR(Toml);
    
    // Test bool -> other type conversions
    UTEST_EQUAL_EXPR(Toml.GetInt("true_value", -1), -1);
    UTEST_EQUAL_EXPR(Toml.GetInt("false_value", -1), -1);
    UTEST_EQUAL_EXPR(Toml.GetFloat("true_value", -1.0), -1.0);
    
    // Test int -> other type conversions
    UTEST_EQUAL_EXPR(Toml.GetBool("int_value", false), false);
    
    // Test float -> other type conversions
    UTEST_EQUAL_EXPR(Toml.GetBool("float_value", false), false);
    UTEST_EQUAL_EXPR(Toml.GetInt("float_value", -1), -1);
    
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTomlFileErrorTest, "UnrealToml.FTomlFile.ErrorHandling", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FTomlFileErrorTest::RunTest(const FString& Parameters)
{
    const FString TestToml = TEXT(R"(
        string_value = "test"
        int_value = 42
    )");
    
    FTomlTable Toml = FTomlTable::LoadString(TestToml);
    UTEST_VALID_EXPR(Toml);
    
    // Test type mismatches using non-checked getters
    const FString TypeMismatchToml = TEXT(R"(
        string_value = "not a number"
        bool_value = true
        number_value = 42.5
    )");
    
    FTomlTable TypeMismatchTest = FTomlTable::LoadString(TypeMismatchToml);
    UTEST_VALID_EXPR(TypeMismatchTest);
    
    // Try to get string as other types
    UTEST_EQUAL_EXPR(TypeMismatchTest.GetInt("string_value", -1), -1);
    UTEST_EQUAL_EXPR(TypeMismatchTest.GetBool("string_value", true), true);
    UTEST_EQUAL_EXPR(TypeMismatchTest.GetFloat("string_value", -1.0), -1.0);
    
    // Try to get bool with wrong types - should get defaults since type mismatches
    UTEST_EQUAL_EXPR(TypeMismatchTest.GetInt("bool_value", -1), -1);
    UTEST_EQUAL_EXPR(TypeMismatchTest.GetString("bool_value", TEXT("default")), TEXT("default"));
    UTEST_EQUAL_EXPR(TypeMismatchTest.GetFloat("bool_value", -1.0), -1.0);
    
    // Try to get number as other types
    UTEST_EQUAL_EXPR(TypeMismatchTest.GetString("number_value", TEXT("default")), TEXT("default"));
    UTEST_EQUAL_EXPR(TypeMismatchTest.GetBool("number_value", true), true);
    
    // Test non-existent paths
    const FString DefaultStr = TEXT("default");
    UTEST_EQUAL_EXPR(Toml.AtPathString("non.existent.path", DefaultStr), DefaultStr);
    
    // Test invalid TOML
    const FString InvalidToml = TEXT("invalid ] toml = content");
    AddExpectedError(TEXT("Failed to parse TOML string"), EAutomationExpectedErrorFlags::Contains);
    FTomlTable InvalidTest = FTomlTable::LoadString(InvalidToml);
    UTEST_INVALID_EXPR(InvalidTest);
    
    return true;
}
