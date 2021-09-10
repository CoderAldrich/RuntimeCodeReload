#define RCR_ENABLED
#include "../Defines.h"
#include "../RuntimeCodeReloader.h"
#include "Tokenizer.h"
#include <windows.h>
#include <imagehlp.h>
#include <shlwapi.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <assert.h>

#pragma comment (lib, "Imagehlp.lib")
#pragma comment (lib, "Shlwapi.lib")

std::map<std::string, std::string> m_typedefRemap;
void RCR_addTypedef(const char* typedefName, const char* actualName)
{
    m_typedefRemap.insert( std::pair<std::string, std::string>(typedefName, actualName));
}

// Remap known typedef - Unfortunately the DLL symbol doesn't use the typedef but the actual type
std::string RCR_RemapKnownTypedef( const std::string& type )
{
    auto it = m_typedefRemap.find(type);
    if( it != m_typedefRemap.end() )
        return it->second;
    else
        return type;
}

std::string RCR_composeNameArgs(const std::string& args);
std::string RCR_processArgument(std::string argument)
{
    if(argument.length() == 0)
        return "";

    RCR::Tokenizer tokenizer(argument.c_str(), argument.length());
    RCR::Tokenizer::Token token = tokenizer.nextToken();
    std::string modifiers = "";
    std::string typeName = "";
    // First token should be either const/volatile or the type
    RCR_ASSERT(token == RCR::Tokenizer::Token::eString || token == RCR::Tokenizer::Token::eScopedType, "Invalid argument syntax");
    if(token == "const" || token == "volatile")
    {
        modifiers = modifiers + " " + (std::string)token;
        typeName = tokenizer.nextToken();
    }
    else
        typeName = token;

    // Remap any typedeffed known name
    typeName = RCR_RemapKnownTypedef(typeName);

    token = tokenizer.nextToken();
    // If we now have a < we process it
    if(token == RCR::Tokenizer::Token::eOpenAngularBracket)
    {
        token = tokenizer.nextToken();
        std::string templateArguments = "";
        size_t stack = 0;
        while(token != RCR::Tokenizer::Token::eClosedAngularBracket && stack == 0)
        {
            if(token == RCR::Tokenizer::Token::eOpenAngularBracket) stack ++;
            if(token == RCR::Tokenizer::Token::eClosedAngularBracket) stack --;
            templateArguments += token;
            token = tokenizer.nextToken();
        }
        typeName = typeName + "<" + RCR_composeNameArgs(templateArguments) + ">";
        token = tokenizer.nextToken();
    }

    // The next token can either be const/volatile, *, &, a literal or End. If we find the literal or End we finish
    while(token != RCR::Tokenizer::Token::eString && token != RCR::Tokenizer::Token::eEnd)
    {
        modifiers = modifiers + " " + (std::string)token;
        token = tokenizer.nextToken();
    }

    return typeName + modifiers;
}

std::string RCR_composeNameArgs(const std::string& args)
{
    size_t index = 0;
    size_t commaIndex = 0;
    std::string output;
    std::string argument;
    while ((commaIndex = args.find(',', index)) != std::string::npos) 
    {
        argument = args.substr(index, commaIndex);
        output += RCR_processArgument(argument) + ",";
        index = commaIndex + 1;
    }

    argument = args.substr(index, args.length() - index);
    return output + RCR_processArgument(argument);
}

std::string RCR_composeName( const char* returnType, const char* symbol, const char* args)
{
    std::string output = RCR_RemapKnownTypedef(returnType);
    output = output.length() > 0 ? output + " " + symbol : symbol;
    std::string argsStr = args;
    if(argsStr.length() != 0)
    {
        size_t roundBracketStart = argsStr.find_first_of('(');
        size_t roundBracketEnd = argsStr.find_last_of(')');
        RCR_ASSERT(roundBracketStart != std::string::npos && roundBracketEnd != std::string::npos, "Invalid args passed");
        roundBracketStart += 1;
        argsStr = argsStr.substr(roundBracketStart, roundBracketEnd - roundBracketStart);
        RCR_ASSERT(argsStr.find_first_of('(') == std::string::npos && argsStr.find_first_of(')') == std::string::npos, "Invalid args passed");
        return output + "(" + RCR_composeNameArgs(argsStr) + ")";
    }
    else
        return output;
}

std::string getFilename(const std::string& path)
{
    auto fileNameStart = path.find_last_of('/');
    if(fileNameStart == std::string::npos)
        fileNameStart = path.find_last_of('\\');
    if(fileNameStart != std::string::npos && fileNameStart + 1 < path.length() )
    {
        fileNameStart++;
        return path.substr(fileNameStart, path.length() - fileNameStart);
    }
    return path;
}

std::string getDirectories(const std::string& path)
{
    auto fileNameStart = path.find_last_of('/');
    if(fileNameStart == std::string::npos)
        fileNameStart = path.find_last_of('\\');
    if(fileNameStart != std::string::npos)
        return path.substr(fileNameStart);
    return ".\\";
}

void CopyAllFiles(const char* srcPath, const char* destPath)
{
    char fileSrc[256];
    char fileDest[256];
    WIN32_FIND_DATA info;
    HANDLE hp; 
    sprintf_s(fileSrc, "%s\\*.*", srcPath);
    hp = FindFirstFile(fileSrc, &info);
    if( hp != INVALID_HANDLE_VALUE )
    {
        do
        {
            sprintf_s(fileSrc,"%s\\%s", srcPath, info.cFileName);
            sprintf_s(fileDest,"%s\\%s", destPath, info.cFileName);
            CopyFileA(fileSrc, fileDest, false);

        }while(FindNextFile(hp, &info)); 
        FindClose(hp);
    }
}

void DeleteAllFiles(char* folderPath)
{
    char fileFound[256];
    WIN32_FIND_DATA info;
    HANDLE hp; 
    sprintf_s(fileFound, "%s\\*.*", folderPath);
    hp = FindFirstFile(fileFound, &info);
    if( hp != INVALID_HANDLE_VALUE )
    {
        do
        {
            sprintf_s(fileFound,"%s\\%s", folderPath, info.cFileName);
            DeleteFile(fileFound);

        }while(FindNextFile(hp, &info)); 
        FindClose(hp);
    }
}

namespace RCR
{
    RuntimeCodeReloader::RuntimeCodeReloader(HINSTANCE* hDLLInstance, std::map<std::string, std::string>* dllSymbolMap, const char* vcvarsallPath, const char* solutionFullPath, const char* dllFullPath, const char* dllTemporaryDir, const char* projectName, std::vector<std::string> &configurations)
        : m_hGetProcIDDLL(hDLLInstance)
        , m_dllSymbolMap(dllSymbolMap)
        , m_configurations(configurations)
    {
        RCR_addTypedef("size_t", "unsigned int");
        RCR_addTypedef("std::string", "std::basic_string<char,std::char_traits<char>,std::allocator<char> >");

        RCR_ASSERT(strlen(vcvarsallPath) < 256, "Invalid string length for vcvarsallPath: path too long (> 256 chars)");
        RCR_ASSERT(strlen(solutionFullPath) < 256, "Invalid string length for solutionFullPath: path too long (> 256 chars)");
        RCR_ASSERT(strlen(dllFullPath) < 256, "Invalid string length for dllFullPath: path too long (> 256 chars)");
        RCR_ASSERT(strlen(dllTemporaryDir) < 256, "Invalid string length for dllTemporaryDir: path too long (> 256 chars)");
        RCR_ASSERT(dllTemporaryDir[0] != '.', "dllTemporaryDir must be an absolute path (because it's called from MSBuild and from the exe, and their relative paths are different)");
        RCR_ASSERT(strlen(projectName) < 128, "Invalid string length for projectName: path too long (> 128 chars)");
        memcpy(m_vcvarsallPath,vcvarsallPath,strlen(vcvarsallPath));
        m_vcvarsallPath[strlen(vcvarsallPath)] = 0;
        memcpy(m_solutionFullPath,solutionFullPath,strlen(solutionFullPath));
        m_solutionFullPath[strlen(solutionFullPath)] = 0;
        memcpy(m_dllFullPath,dllFullPath,strlen(dllFullPath));
        m_dllFullPath[strlen(dllFullPath)] = 0;
        memcpy(m_dllTemporaryDir,dllTemporaryDir,strlen(dllTemporaryDir));
        m_dllTemporaryDir[strlen(dllTemporaryDir)] = 0;
        if(strlen(dllTemporaryDir) > 1 && m_dllTemporaryDir[strlen(dllTemporaryDir)-1] == '\\')
            m_dllTemporaryDir[strlen(dllTemporaryDir)-1] = 0;
        memcpy(m_projectName,projectName,strlen(projectName));
        m_projectName[strlen(projectName)] = 0;
        reload(eCleanCompileAndReload);
    }

    RuntimeCodeReloader::~RuntimeCodeReloader()
    {
        if(m_hGetProcIDDLL && *m_hGetProcIDDLL)
        {
            FreeLibrary(*m_hGetProcIDDLL);
            *m_hGetProcIDDLL = 0;
        }
        m_dllSymbolMap->clear();

        DeleteAllFiles(m_dllTemporaryDir);
        RemoveDirectory(m_dllTemporaryDir);
    }

    bool RuntimeCodeReloader::reload(Flag flag)
    {
        bool needCompiling = flag == eCleanCompileAndReload || flag == eCleanCompileAndNoReload || flag == eCompileAndReload || flag == eCompileNoReload;
        bool needReloading = flag == eCleanCompileAndReload || flag == eCompileAndReload || flag == eNoCompileAndReload;

        if(needCompiling)
        {
            if( !compileDLL(flag) )
                MessageBoxA(0, "Failed to compile the required DLL; check that all the paths are correct and retry.", "Runtime Code Reload - ASSERT", MB_ICONERROR);
        }

        if(needReloading)
        {
            // Release the DLL
            if(m_hGetProcIDDLL)
            {
                FreeLibrary(*m_hGetProcIDDLL);
                *m_hGetProcIDDLL = 0;
            }

            // The new DLL is created in a temporary folder; retrieve the DLL and copy it to the right place
            bool result = CopyFile( (std::string(m_dllTemporaryDir) + '\\' + getFilename(m_dllFullPath) ).c_str(), m_dllFullPath, false) == TRUE;
            if(!result)
            {
                char auxString[256];
                sprintf_s(auxString, "Failed to copy the DLL from %s to %s", (std::string(m_dllTemporaryDir) + '\\' + getFilename(m_dllFullPath) ).c_str(), m_dllFullPath);
                MessageBoxA(0, auxString, "Runtime Code Reload - ASSERT", MB_ICONERROR);
            }
            // Copy all the files from the directory; this will copy the DLL again, useless but won't fix for now
            CopyAllFiles(m_dllTemporaryDir, getDirectories(m_dllFullPath).c_str());
            DeleteAllFiles(m_dllTemporaryDir);
            RemoveDirectory(m_dllTemporaryDir);

            // Even if we filed we try to load it, we may just have failed to compile due to code issues but the old dll is actually there
            *m_hGetProcIDDLL = LoadLibraryA(m_dllFullPath);

            if (!*m_hGetProcIDDLL) 
            {
                RCR_ASSERT(false, "Failed to load the required DLL; check that the DLL's path is correct and retry.");
                return false;
            }

            *m_dllSymbolMap = createDLLExportedSymbolsConversionTable(m_dllFullPath);
        }
        
        return true;
    }

    bool RuntimeCodeReloader::compileDLL(Flag flag)
    {
        bool cleanCompile = flag == eCleanCompileAndReload || flag == eCleanCompileAndNoReload;

		if(!PathFileExists(m_vcvarsallPath))
        {
            char errorMessage[256];
            sprintf_s(errorMessage, 256, "Failed to find Visual Studio MSBuild bat file.\nPlease check that the vcvarsall path provided (%s) exists.", m_vcvarsallPath);
			MessageBox(0, errorMessage, "Error - MSBuild not found", MB_ICONINFORMATION);
			return false;
		}

		if(!PathFileExists(m_solutionFullPath))
		{
			MessageBox(0, "Failed to find the specified solution. Please check the path.", "Error - Solution not found", MB_ICONINFORMATION);
			return false;
		}

        char auxString[256];
        std::ofstream batch;
        batch.open ("_compile.bat");
        sprintf_s(auxString, "call \"%s\" x86", m_vcvarsallPath);
        batch << auxString << std::endl;
        std::string solutionFolder =  getDirectories(m_solutionFullPath);
        for(auto it = m_configurations.begin(); it != m_configurations.end(); ++it)
        {
            sprintf_s(auxString, "MSBuild \"%s\" /t:%s%s /p:Configuration=%s /p:OutDir=\"%s\\\\\" >> _compilation_output.txt", m_solutionFullPath, m_projectName, cleanCompile ? ":Rebuild" : "", (*it).c_str(), m_dllTemporaryDir);
            batch << auxString << std::endl;
        }
        batch.close();

        // Start process
        STARTUPINFO info = {0};
        PROCESS_INFORMATION procInfo = {0};
        ZeroMemory(&info, sizeof(info));
        info.cb = sizeof(info);
        ZeroMemory(&procInfo, sizeof(procInfo));

        DWORD ret = 0;
        if (CreateProcessA("_compile.bat", NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &info, &procInfo))
        {
            WaitForSingleObject(procInfo.hProcess, INFINITE);
            CloseHandle(procInfo.hProcess);
            CloseHandle(procInfo.hThread);
            GetExitCodeProcess(procInfo.hProcess, &ret);
        }

        DeleteFile("_compile.bat");

        std::ifstream compilationOutput;
        std::string auxSTDSTring;
        compilationOutput.open ("_compilation_output.txt");
        compilationOutput.seekg(0, std::ios::end);   
        auxSTDSTring.reserve((size_t)compilationOutput.tellg());
        compilationOutput.seekg(0, std::ios::beg);
        auxSTDSTring.assign((std::istreambuf_iterator<char>(compilationOutput)), std::istreambuf_iterator<char>());
        compilationOutput.close();

        if(auxSTDSTring.find("Build FAILED") != std::string::npos)
        {
            OutputDebugStringA(auxSTDSTring.c_str());
            OutputDebugStringA("\n");
            MessageBox(0, "Failed to compile; please check your output window.", "Error - Compilation Failed", MB_ICONERROR);
        }

        DeleteFile("_compilation_output.txt");

        return ret == 0;
    }

    std::string replaceAllOccurrences(std::string input, const std::string& stringToReplace, const std::string& stringForReplacement)
    {
        size_t start_pos = 0;
        while((start_pos = input.find(stringToReplace, start_pos)) != std::string::npos) {
            input.replace(start_pos, stringToReplace.length(), stringForReplacement);
            start_pos += stringForReplacement.length();
        }
        return input;
    }

    std::map<std::string, std::string> RuntimeCodeReloader::createDLLExportedSymbolsConversionTable(const char* sDllName)
    {
        std::map<std::string, std::string> sMapDllSymbols;
        DWORD *dNameRVAs(0);
        _IMAGE_EXPORT_DIRECTORY *ImageExportDirectory;
        unsigned long cDirSize;
        _LOADED_IMAGE LoadedImage;
        std::string sName;
        sMapDllSymbols.clear();
        if (MapAndLoad(sDllName, NULL, &LoadedImage, TRUE, TRUE))
        {
            ImageExportDirectory = (_IMAGE_EXPORT_DIRECTORY*) ImageDirectoryEntryToData(LoadedImage.MappedAddress, false, IMAGE_DIRECTORY_ENTRY_EXPORT, &cDirSize);
            if (ImageExportDirectory != NULL)
            {
                dNameRVAs = (DWORD *)ImageRvaToVa(LoadedImage.FileHeader, LoadedImage.MappedAddress, ImageExportDirectory->AddressOfNames, NULL);
                for(size_t i = 0; i < ImageExportDirectory->NumberOfNames; i++)
                {
                    sName = (char *)ImageRvaToVa(LoadedImage.FileHeader, LoadedImage.MappedAddress, dNameRVAs[i], NULL);

                    char undecoratedName[256];
                    UnDecorateSymbolName(sName.c_str(), undecoratedName, 256, UNDNAME_NO_ACCESS_SPECIFIERS | UNDNAME_NO_ALLOCATION_LANGUAGE | UNDNAME_NO_ALLOCATION_MODEL | UNDNAME_NO_SPECIAL_SYMS | UNDNAME_NO_RETURN_UDT_MODEL | UNDNAME_NO_MS_THISTYPE | UNDNAME_NO_MS_KEYWORDS);
                    std::string undecoratedNameString = std::string(undecoratedName);

                    undecoratedNameString = replaceAllOccurrences(undecoratedNameString, "(void)", "()");
                    undecoratedNameString = replaceAllOccurrences(undecoratedNameString, "class ", "");
                    undecoratedNameString = replaceAllOccurrences(undecoratedNameString, "struct ", "");
                    undecoratedNameString = replaceAllOccurrences(undecoratedNameString, "enum ", "");

                    sMapDllSymbols.insert(std::make_pair(undecoratedNameString,sName) );
                }
            }
            UnMapAndLoad(&LoadedImage);
        }
        return sMapDllSymbols;
    }
}