#include <tchar.h>
#include <iostream>

#ifdef RCR_ENABLED
#define RCR_DLL_NAME UnitTestRCRDLL
#include "../UnitTest_RCR_DLL/UnitTest_RCR_DLL.h"
#include "../RuntimeCodeReload/RuntimeCodeReloader.h"
RCR_DEFINE_EXTERNALS
#else
#include "../UnitTest_RCR_DLL/UnitTest_RCR_DLL.h"
#endif

#define UNIT_TEST_ASSERT(a, mex) if(!(a)){ std::cout << "[!]\t"<< mex << " failed." << std::endl; testPassed = false; }else{ std::cout << "\t" << mex << " passed." << std::endl; }

int _tmain(int, _TCHAR*[])
{
	using namespace UnitTest_RCR;
	bool testPassed = true;

	std::cout << "[RCR Unit Test]" << std::endl;

#ifdef RCR_ENABLED
	auto attributes = GetFileAttributesA("..\\UnitTest_RCR_DLL\\UnitTest_RCR_DLL.cpp");
	SetFileAttributesA("..\\UnitTest_RCR_DLL\\UnitTest_RCR_DLL.cpp", FILE_ATTRIBUTE_NORMAL);
	CopyFileA("..\\UnitTest_RCR_DLL\\UnitTest_RCR_DLL_1.cpp", "..\\UnitTest_RCR_DLL\\UnitTest_RCR_DLL.cpp", false);
	SetFileAttributesA("..\\UnitTest_RCR_DLL\\UnitTest_RCR_DLL.cpp", attributes);
	std::cout << "\tCompiling DLL..." << std::endl;
	std::vector<std::string> configurations;
	configurations.push_back("Debug");
	RCR::RuntimeCodeReloader codeReloader(&RCR_DLL_HISTANCE, &RCR_SYMBOL_MAP, "C:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\vcvarsall.bat","..\\RuntimeCodeReload.sln", "..\\bin\\UnitTest_RCR_DLL_d.dll",  "C:\\temp\\UnitTest_RCR_DLL_d\\", "UnitTest_RCR_DLL", configurations );
	std::cout << "\tDone..." << std::endl << std::endl;
#endif

	TemplateStruct<float> paramTemplateStruct;
	paramTemplateStruct.value = 2.0f;

	UNIT_TEST_ASSERT( testFunction1(10, 8.0f) == 80, "testFunction1 code test ");
	UNIT_TEST_ASSERT( testFunction2(&paramTemplateStruct) == 4.0f, "testFunction2 code test ");

	DEFINE_RCR_VARIABLE_PTR(size_t, UnitTest_RCR:: , g_globalValue);
	UNIT_TEST_ASSERT( *g_globalValue == 90, "g_globalValue default test ");
	testFunction3( 23 );
	UNIT_TEST_ASSERT( *g_globalValue == 23, "g_globalValue change test ");

	UNIT_TEST_ASSERT( testFunction4("AB") == 'A', "testFunction4 code test ");
	{
		TestClass testObj(12,30);
		UNIT_TEST_ASSERT( testObj.test(5) == 12 + 30 * 5, "object method invocation test ");
	}

#ifdef RCR_ENABLED
	std::cout << std::endl << "\tModifing source code..." << std::endl;
	attributes = GetFileAttributesA("..\\UnitTest_RCR_DLL\\UnitTest_RCR_DLL.cpp");
	SetFileAttributesA("..\\UnitTest_RCR_DLL\\UnitTest_RCR_DLL.cpp", FILE_ATTRIBUTE_NORMAL);
	CopyFileA("..\\UnitTest_RCR_DLL\\UnitTest_RCR_DLL_2.cpp", "..\\UnitTest_RCR_DLL\\UnitTest_RCR_DLL.cpp", false);
	SetFileAttributesA("..\\UnitTest_RCR_DLL\\UnitTest_RCR_DLL.cpp", attributes);
	std::cout << "\tRecompiling DLL..." << std::endl;
	codeReloader.reload(RCR::RuntimeCodeReloader::eCleanCompileAndReload);
	std::cout << "\tDone..." << std::endl << std::endl;

	UNIT_TEST_ASSERT( testFunction1(10, 8.0f) == 18, "testFunction1 code test ");
    UNIT_TEST_ASSERT( testFunction2(&paramTemplateStruct) == 1.0f, "testFunction2 code test ");
    g_globalValue = GET_RCR_VARIABLE_PTR(size_t, UnitTest_RCR:: , g_globalValue);
	UNIT_TEST_ASSERT( *g_globalValue == 13, "g_globalValue default test ");
	testFunction3( 20 );
	UNIT_TEST_ASSERT( *g_globalValue == 10, "g_globalValue change test ");
	UNIT_TEST_ASSERT( testFunction4("AB") == 'B', "testFunction4 code test ");

	{
		TestClass testObj(12,30);
		UNIT_TEST_ASSERT( testObj.test(5) == 12 + 30 / 5, "object method invocation test ");
	}
	CopyFileA("..\\UnitTest_RCR_DLL\\UnitTest_RCR_DLL_1.cpp", "..\\UnitTest_RCR_DLL\\UnitTest_RCR_DLL.cpp", false);
#endif

	if(testPassed)
		std::cout << "[PASSED]" << std::endl;
	else
		std::cout << "[FAILED]" << std::endl;

    return 0;
}

