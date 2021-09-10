#pragma once

#include "../RuntimeCodeReload/RCR.h"

namespace UnitTest_RCR
{
    #undef RCR_CURRENT_NAMESPACE_SEQUENCE
    #define RCR_CURRENT_NAMESPACE_SEQUENCE "UnitTest_RCR::"

    template <typename T>
    struct TemplateStruct
    {
        T value;
    };
    
    RCR_VARIABLE(size_t, g_globalValue);
    RCR_FUNCTION(int, testFunction1, (int a, float b), (a,b));
    RCR_FUNCTION(float, testFunction2, (const UnitTest_RCR::TemplateStruct<float>* t), (t));
    RCR_FUNCTION(void, testFunction3, (size_t globalValue), (globalValue));
    RCR_FUNCTION(char, testFunction4, (const std::string& s), (s));

    RCR_CLASS TestClass {
    public:
        RCR_CONSTRUCTOR(TestClass, (), ());
        RCR_CONSTRUCTOR(TestClass, (int f1, float f2), (f1, f2));
        RCR_DESTRUCTOR(TestClass);

        RCR_METHOD(TestClass, double, test, (int u), (u));
        RCR_METHOD(TestClass, double, test, (int u, int v), (u, v));

    private:
        int m_field1;
        float m_field2;
    };

    #undef RCR_CURRENT_NAMESPACE_SEQUENCE
    #define RCR_CURRENT_NAMESPACE_SEQUENCE
}