#include "UnitTest_RCR_DLL.h"

namespace UnitTest_RCR
{
    size_t g_globalValue = 13;

    RCR_API int testFunction1(int a, float b)
    {
        return (int)(a + b);
    }

    RCR_API float testFunction2(const TemplateStruct<float>* t)
    {
        return t->value / 2.0f;
    }

    size_t someInternalFunction(size_t val)
    {
        return val / 2;
    }

    RCR_API void testFunction3(size_t globalValue)
    {
        g_globalValue = someInternalFunction( globalValue );
    }

    RCR_API char testFunction4(const std::string& s)
    {
        return s[1];
    }

    RCR_API TestClass::TestClass()
        : m_field1(8)
        , m_field2(2.0f)
    {}

    RCR_API TestClass::TestClass(int f1, float f2)
        : m_field1(f1)
        , m_field2(f2)
    {}

    RCR_API TestClass::~TestClass()
    {}

    RCR_API double TestClass::test(int u)
    {
        return m_field1 + m_field2 / u;
    }

    RCR_API double TestClass::test(int u, int v)
    {
        return m_field1 + m_field2 / u;
    }
}