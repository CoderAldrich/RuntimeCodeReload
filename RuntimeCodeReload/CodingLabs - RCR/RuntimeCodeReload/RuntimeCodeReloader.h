#include <windows.h>
#include <string>
#include <vector>
#include <map>

#ifndef RCR_ENABLED
#error Can't include RuntimeCodeReloader.h. If RCR is not enabled (define RCR_ENABLED) then the RuntimeCodeReloader can't work; please use ifdefs to avoid compiling it in and link the DLL library instead (or enable RCR if that's the intention)
#endif

namespace RCR
{
    class RuntimeCodeReloader
    {
    public:
        enum Flag
        {
            eCleanCompileAndReload,
            eCleanCompileAndNoReload,
            eCompileAndReload,
            eCompileNoReload,
            eNoCompileAndReload,
        };

        // Ctor/Dtor
        // ------------------------------------------------------------------------
        RuntimeCodeReloader(HINSTANCE* hDLLInstance, std::map<std::string, std::string>* dllSymbolMap, const char* vcvarsallPath, const char* solutionFullPath, const char* dllFullPath, const char* dllTemporaryDir, const char* projectName, std::vector<std::string> &configurations);
        ~RuntimeCodeReloader();

        // Methods
        // ------------------------------------------------------------------------
        bool reload(Flag flag = eCompileAndReload);

    private:
        // Methods
        // ------------------------------------------------------------------------
        bool compileDLL(Flag flag);
        static std::map<std::string, std::string> createDLLExportedSymbolsConversionTable(const char* sDllName);

    private:
        // Fields
        // ------------------------------------------------------------------------
        char                                m_vcvarsallPath[256];
        char                                m_solutionFullPath[256];
        char                                m_dllFullPath[256];         // Where the DLL is copied to once compiled
        char                                m_dllTemporaryDir[256];    // Where the DLL is compiled to
        char                                m_projectName[128];
        HINSTANCE*                          m_hGetProcIDDLL;
        std::map<std::string, std::string>* m_dllSymbolMap;
        std::vector<std::string>            m_configurations;
    };
}