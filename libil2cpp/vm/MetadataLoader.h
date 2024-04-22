#pragma once
#include "il2cpp-config.h"

//#define IL2CPP_VMP 1
#ifdef IL2CPP_VMP
#include "VMProtectSDK.h"
#define VMP_ALLOC_STRING(s) VMProtectDecryptStringA(s)
#define VMP_FREE_STRING(s) VMProtectFreeString(s)
#define VMP_BEGIN(s) VMProtectBegin(s)
#define VMP_END() VMProtectEnd()
#else
#define VMP_ALLOC_STRING(s) (s)
#define VMP_FREE_STRING(s)
#define VMP_BEGIN(s) 
#define VMP_END() 
#endif


namespace il2cpp
{
namespace vm
{
    class LIBIL2CPP_CODEGEN_API MetadataLoader
    {
    public:
        static void* LoadMetadataFile(const char* fileName);
        static void UnloadMetadataFile(void* fileBuffer);

		static void* GetIL2CPPDataFile(const char* fileName);
    };
} // namespace vm
} // namespace il2cpp
