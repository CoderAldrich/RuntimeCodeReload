#pragma once

#include "Defines.h"
#include <windows.h>
#include <map>
#include <string>

// ************************************************************************************************************************************************
//  STEPS FOR SETTING UP A NEW RCR MODULE
// 
//  DLL PROJECT
//  1 - Create e new DLL VS project
//  2 - Include RCR.h into all the interface files and use the appropriate macros to create the interface (example shown a the end)
//  3 - Add the RCR_DLL_EXPORTS define to the DLL preprocessor symbols
// 
//  MAIN PROJECT
//  4 - Include the interface file into the main project
//      a - Set the preprocessor define for the desired configurations (usually Debug) to define RCR_ENABLED
//      b - Set the library input for the desired configurations (all those that ARE NOT chosen for the previous step) to include the DLL .lib file
//  5 - Before the inclusion line add a define for RCR_DLL_NAME
//  6 - Add the RCR_DEFINE_EXTERNALS line in a CPP file; if the file doesn't include the interface file then you'll need to define RCR_ENABLED 
//      and RCR_DLL_NAME just before the inclusion line for RCR.h
//  7 - Create an instance of RuntimeCodeReloader in a file where the appropriate inclusion has been done. Provide RCR_DLL_HISTANCE and
//      RCR_SYMBOL_MAP to the object so that it binds to the correct global symbols
//
// ::::::::::::::::::::::::::::::::::::::::::::::::::::: EXAMPLE of INTERFACE FILE ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//
//  #pragma once
//  
//  #include "../RuntimeCodeReload/RCR.h"
//  
//  namespace UnitTest_RCR
//  {
//  #undef RCR_CURRENT_NAMESPACE_SEQUENCE
//  #define RCR_CURRENT_NAMESPACE_SEQUENCE "UnitTest_RCR::"
//  
//      RCR_VARIABLE(size_t, g_globalValue);
//      RCR_FUNCTION(int, testFunction1, (int a, float b), (a,b));
//      RCR_FUNCTION(char, testFunction2, (const std::string& s), (s));
//      RCR_FUNCTION(void, testFunction3, (size_t globalValue), (globalValue));
//  
//      RCR_CLASS TestClass {
//  public:
//      RCR_CONSTRUCTOR(TestClass, (), ());
//      RCR_CONSTRUCTOR(TestClass, (int f1, float f2), (f1, f2));
//      RCR_DESTRUCTOR(TestClass);
//  
//      RCR_METHOD(TestClass, double, test, (int u), (u));
//  
//  private:
//      int m_field1;
//      float m_field2;
//      };
//  
//  #undef RCR_CURRENT_NAMESPACE_SEQUENCE
//  #define RCR_CURRENT_NAMESPACE_SEQUENCE
//  }
//
// ************************************************************************************************************************************************

#define RCR_CONCAT(a,b) RCR_CONCATBASE(a,b)
#define RCR_CONCATBASE(a,b) a##b
#define RCR_MAKE_STRING(s) #s

#ifdef RCR_DLL_EXPORTS
#define RCR_VARIABLE( returnType, name ) extern __declspec(dllexport) returnType name
#define RCR_FUNCTION( returnType, name, args, vals ) __declspec(dllexport) returnType name args
#define RCR_CONSTRUCTOR( className, args, vals ) className args
#define RCR_DESTRUCTOR( className ) ~className ()
#define RCR_METHOD( className, returnType, name, args, vals ) returnType name args
#define RCR_API __declspec(dllexport)
#define RCR_CLASS class __declspec(dllexport)
#else // RCR_DLL_EXPORTS
// --------------------------------------------------------- RUR ENABLED ---------------------------------------------------------
#ifdef RCR_ENABLED
#define RCR_API __declspec(dllimport)

#ifndef RCR_DLL_NAME
#error When including <filename.h> you first need to define an unique name for the DLL (defining RCR_DLL_NAME)
#endif

#define RCR_CURRENT_NAMESPACE_SEQUENCE

#define RCR_DLL_HISTANCE RCR_CONCAT(g_hGetProcIDDLL_,RCR_DLL_NAME)
#define RCR_SYMBOL_MAP RCR_CONCAT(g_symbolMap_,RCR_DLL_NAME)

extern HINSTANCE RCR_DLL_HISTANCE;
extern std::map<std::string, std::string> RCR_SYMBOL_MAP;
#define RCR_DEFINE_EXTERNALS HINSTANCE RCR_DLL_HISTANCE; std::map<std::string, std::string> RCR_SYMBOL_MAP;

void RCR_addTypedef( const char* typedefName, const char* actualName);
std::string RCR_composeName( const char* returnType, const char* symbol, const char* args);

static inline const char* RCR_findSymbol( const char* returnType, const char* symbol, const char* args)
{
    auto composedName = RCR_composeName(returnType, symbol, args);
    auto it = RCR_SYMBOL_MAP.find(composedName.c_str());
    RCR_ASSERT(it != RCR_SYMBOL_MAP.end(), (std::string("The undecorated name (") + composedName + ") provided doesn't have an entry to map back to the dll mangled equivalent. This could be because of some expanded typedef that is not present into RCR_RemapKnownTypedef. The application will exit.").c_str());
    return it->second.c_str();
}

static inline const char* RCR_findSymbol(const char* symbol, const char* args)
{
    return RCR_findSymbol("", symbol, args);
}

#define GET_RCR_VARIABLE_PTR(variableType, namespc, variableName) ((variableType*)GetProcAddress(RCR_DLL_HISTANCE, RCR_findSymbol(RCR_MAKE_STRING(variableType), RCR_MAKE_STRING(namespc)RCR_MAKE_STRING(variableName), "")));
#define DEFINE_RCR_VARIABLE_PTR(variableType, namespc, variableName) variableType* variableName = GET_RCR_VARIABLE_PTR(variableType, namespc, variableName);
#define GET_RCR_FUNCTION(functionName, functionType, sym) functionType functionName = (functionType)GetProcAddress(RCR_DLL_HISTANCE, sym);

#define RCR_VARIABLE( returnType, name )
#define RCR_FUNCTION( returnType, name, args, vals )\
    static returnType name args { \
    typedef returnType (*name##_type) args; \
    GET_RCR_FUNCTION(func, name##_type, RCR_findSymbol(RCR_MAKE_STRING(returnType), RCR_CONCAT(RCR_CURRENT_NAMESPACE_SEQUENCE,RCR_MAKE_STRING(name)),RCR_MAKE_STRING(args))); \
    return func##vals;\
}

#define RCR_CONSTRUCTOR( className, args, vals )\
    className args { \
    typedef void (className::*className##_type) args; \
        union MethodUnion { \
            struct CompilerDependantStructure { \
                void* methodAddress; \
            } compound; \
            className##_type methodPointer; \
        } methodUnion; \
        methodUnion.compound.methodAddress = (void*)GetProcAddress(RCR_DLL_HISTANCE, RCR_findSymbol(RCR_CONCAT(RCR_CURRENT_NAMESPACE_SEQUENCE,RCR_MAKE_STRING(className::className)),RCR_MAKE_STRING(args))); \
        ((this)->*(methodUnion.methodPointer))##vals;\
    }
#define RCR_DESTRUCTOR( className )\
    ~className () { \
    typedef void (className::*className##_type)(); \
        union MethodUnion { \
            struct CompilerDependantStructure { \
                void* methodAddress; \
            } compound; \
            className##_type methodPointer; \
        } methodUnion; \
        methodUnion.compound.methodAddress = (void*)GetProcAddress(RCR_DLL_HISTANCE, RCR_findSymbol(RCR_CONCAT(RCR_CURRENT_NAMESPACE_SEQUENCE,RCR_MAKE_STRING(className::~className)),"()")); \
        ((this)->*(methodUnion.methodPointer))();\
    }
#define RCR_METHOD( className, returnType, name, args, vals )\
    returnType name args { \
        typedef returnType (className::*name##_type) args; \
        union MethodUnion { \
            struct CompilerDependantStructure { \
                void* methodAddress; \
            } compound; \
            name##_type methodPointer; \
        } methodUnion; \
        methodUnion.compound.methodAddress = (void*)GetProcAddress(RCR_DLL_HISTANCE, RCR_findSymbol(RCR_MAKE_STRING(returnType), RCR_CONCAT(RCR_CURRENT_NAMESPACE_SEQUENCE,RCR_MAKE_STRING(className::name)), RCR_MAKE_STRING(args))); \
        return ((this)->*(methodUnion.methodPointer))##vals;\
    }
#define RCR_CLASS class

#else // RCR_ENABLED

// --------------------------------------------------------- RUR DISABLED ---------------------------------------------------------
#define RCR_DEFINE_EXTERNALS
#define RCR_API __declspec(dllimport)
#define RCR_VARIABLE( returnType, name ) extern __declspec(dllimport) returnType name
#define RCR_FUNCTION( returnType, name, args, vals ) __declspec(dllimport) returnType name args
#define RCR_CONSTRUCTOR( className, args, vals ) className args
#define RCR_DESTRUCTOR( className ) ~className ()
#define RCR_METHOD( className, returnType, name, args, vals ) returnType name args
#define RCR_CLASS class __declspec(dllimport)
#define GET_RCR_VARIABLE_PTR(variableType, namespc, variableName) &RCR_CONCAT(namespc,variableName)
#define DEFINE_RCR_VARIABLE_PTR(variableType, namespc, variableName) variableType* variableName = GET_RCR_VARIABLE_PTR(variableType, namespc, variableName)

#endif // RCR_ENABLED
#endif // RCR_DLL_EXPORTS