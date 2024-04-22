#pragma once

#define PROFILE_TIME_CONSUMING 1

#include <stdint.h>
#include "il2cpp-config.h"

#if PROFILE_TIME_CONSUMING
#include <map>
#endif

struct Il2CppClass;
struct Il2CppObject;

namespace il2cpp
{
namespace vm
{
    class LIBIL2CPP_CODEGEN_API Liveness
    {
    public:
        typedef void (*register_object_callback)(Il2CppObject** arr, int size, void* userdata);
        typedef void*(*ReallocateArrayCallback)(void* ptr, size_t size, void* state);

        static void* AllocateStruct(Il2CppClass* filter, int max_object_count, register_object_callback callback, void* userdata, ReallocateArrayCallback reallocateArray);
        static void FreeStruct(void* state);
        static void Finalize(void* state);
        static void FromRoot(Il2CppObject* root, void* state);
        static void FromStatics(void* state);
        static int GetStaticsCount();
        static void CollectStaticsByWorker(void* state, int need_reset, int start_index, int batch_count);
        static void ProcessObjects(void* state);
        static void FromStaticsByWorker(void* state, int worker_index, int worker_count);
        static int TryStealTraverseJob(void* state, void* state_to_steal, int worker_count, int min_steal_count);

        static void ResetTimeConsumingReport(bool profileOn);
        static const char* GetClassName(Il2CppClass* klass);
        static const char* GenerateTimeConsumingReport(void* state, uint32_t threshold);
        static void AppendTimeConsumingReport(std::string& report, std::map<const char*, uint32_t>& records, uint32_t threshold);
    };

#if PROFILE_TIME_CONSUMING
        static bool s_ProfileTimeConsuming;
#endif
} /* namespace vm */
} /* namespace il2cpp */
