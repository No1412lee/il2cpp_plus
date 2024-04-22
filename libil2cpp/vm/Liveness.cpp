#include "il2cpp-config.h"
#include "gc/GarbageCollector.h"
#include <utils/dynamic_array.h>
#include "vm/Array.h"
#include "vm/Class.h"
#include "vm/ClassInlines.h"
#include "vm/Field.h"
#include "vm/Liveness.h"
#include "vm/Type.h"
#include "il2cpp-tabledefs.h"
#include "il2cpp-class-internals.h"
#include "il2cpp-object-internals.h"
#include "Baselib.h"
#include "Cpp/ReentrantLock.h"
#if PROFILE_TIME_CONSUMING
#include "os/Time.h"
#include <vector>
#endif

#define MARK_OBJ(obj) \
    do { \
        (obj)->klass = (Il2CppClass*)(((size_t)(obj)->klass) | (size_t)1); \
    } while (0)

#define CLEAR_OBJ(obj) \
    do { \
        (obj)->klass = (Il2CppClass*)(((size_t)(obj)->klass) & ~(size_t)1); \
    } while (0)

#define IS_MARKED(obj) \
    (((size_t)(obj)->klass) & (size_t)1)

#define GET_CLASS(obj) \
    ((Il2CppClass*)(((size_t)(obj)->klass) & ~(size_t)1))

namespace il2cpp
{
namespace vm
{
    /* number of sub elements of an array to process before recursing
    * we take a depth first approach to use stack space rather than re-allocating
    * processing array which requires restarting world to ensure allocator lock is not held
    */
    const int kArrayElementsPerChunk = 256;

    /* how far we recurse processing array elements before we stop. Prevents stack overflow */
    const int kMaxTraverseRecursionDepth = 128;

    const int kMaxLocalElementsSize = 256;

    struct CustomGrowableBlockArray;

#define kBlockSize (8 * 1024)
#define kArrayElementsPerBlock ((kBlockSize - 3 *sizeof (void*)) / sizeof (void*))

    struct CustomArrayBlock
    {
        Il2CppObject** next_item;
        CustomArrayBlock *prev_block;
        CustomArrayBlock *next_block;
        Il2CppObject* p_data[kArrayElementsPerBlock];
    };

    struct LivenessState
    {
        LivenessState(Il2CppClass* filter, uint32_t maxCount, Liveness::register_object_callback callback, void*callback_userdata, Liveness::ReallocateArrayCallback reallocateArray);
        ~LivenessState();

        void Finalize();
        void Reset();
        void TraverseObjects();
        void FilterObjects();

        static void TraverseGenericObject(Il2CppObject* object, LivenessState* state);
        static void TraverseObject(Il2CppObject* object, LivenessState* state);
        static void TraverseGCDescriptor(Il2CppObject* object, LivenessState* state);
        static bool TraverseObjectInternal(Il2CppObject* object, bool isStruct, Il2CppClass* klass, LivenessState* state);
        static void TraverseArray(Il2CppArray* array, LivenessState* state);
        static bool AddProcessObject(Il2CppObject* object, LivenessState* state);
        static bool ShouldProcessValue(Il2CppObject* val, Il2CppClass* filter);
        static bool FieldCanContainReferences(FieldInfo* field);

        static bool ShouldTraverseObjects(size_t index, int32_t recursion_depth)
        {
            // Add kArrayElementsPerChunk objects at a time and then traverse
            // return ((index + 1) & (kArrayElementsPerChunk - 1)) == 0 && recursion_depth < kMaxTraverseRecursionDepth;
            return false;
        }

        CustomGrowableBlockArray* all_objects;

        Il2CppClass*          filter;

        CustomGrowableBlockArray* process_array;
        CustomArrayBlock process_array_local;

        void*               callback_userdata;

        Liveness::register_object_callback filter_callback;
        Liveness::ReallocateArrayCallback reallocateArray;

        int32_t               traverse_depth; // track recursion. Prevent stack overflow by limiting recurion

        // profile
#if PROFILE_TIME_CONSUMING
        std::map<const char*, uint32_t> collectStaticCostMap;
        std::map<const char*, uint32_t> traverseObjCostMap;
        uint32_t processObjectCount;
#endif
    };

    struct CustomBlockArrayIterator;
    struct CustomGrowableBlockArray
    {
        CustomArrayBlock *first_block;
        CustomArrayBlock *current_block;
        CustomBlockArrayIterator *iterator;
        baselib::ReentrantLock mutex;
        volatile int count;

        CustomGrowableBlockArray(LivenessState *state);

        bool IsEmpty();
        void PushBack(Il2CppObject* value, LivenessState *state);
        void PushBackNoLock(Il2CppObject* value, LivenessState *state);
        Il2CppObject* PopBack();
        Il2CppObject* PopBackNoLock();
        void ResetIterator();
        Il2CppObject* Next();
        void Clear();
        void Destroy(LivenessState *state);
    };

    struct CustomBlockArrayIterator
    {
        CustomGrowableBlockArray *array;
        CustomArrayBlock *current_block;
        Il2CppObject** current_position;
    };

    CustomGrowableBlockArray::CustomGrowableBlockArray(LivenessState *state)
    {
        current_block = (CustomArrayBlock*)state->reallocateArray(NULL, kBlockSize, state->callback_userdata);
        current_block->prev_block = NULL;
        current_block->next_block = NULL;
        current_block->next_item = current_block->p_data;
        first_block = current_block;
        count = 0;

        iterator = new CustomBlockArrayIterator();
        iterator->array = this;
        iterator->current_block = first_block;
        iterator->current_position = first_block->p_data;
    }

    bool CustomGrowableBlockArray::IsEmpty()
    {
        return first_block->next_item == first_block->p_data;
    }

    void CustomGrowableBlockArray::PushBackNoLock(Il2CppObject* value, LivenessState *state)
    {
        if (current_block->next_item == current_block->p_data + kArrayElementsPerBlock)
        {
            CustomArrayBlock* new_block = current_block->next_block;
            if (current_block->next_block == NULL)
            {
                new_block = (CustomArrayBlock*)state->reallocateArray(NULL, kBlockSize, state->callback_userdata);
                new_block->next_block = NULL;
                new_block->prev_block = current_block;
                new_block->next_item = new_block->p_data;
                current_block->next_block = new_block;
            }
            current_block = new_block;
        }
        *current_block->next_item++ = value;
        ++count;
    }

    void CustomGrowableBlockArray::PushBack(Il2CppObject* value, LivenessState *state)
    {
        mutex.Acquire();
        PushBackNoLock(value, state);
        mutex.Release();
    }

    Il2CppObject* CustomGrowableBlockArray::PopBackNoLock()
    {
        if (current_block->next_item == current_block->p_data)
        {
            if (current_block->prev_block == NULL)
                return NULL;
            current_block = current_block->prev_block;
            current_block->next_item = current_block->p_data + kArrayElementsPerBlock;
        }
        --count;
        return *--current_block->next_item;
    }

    Il2CppObject* CustomGrowableBlockArray::PopBack()
    {
        mutex.Acquire();
        Il2CppObject* obj = PopBackNoLock();
        mutex.Release();
        return obj;
    }

    void CustomGrowableBlockArray::ResetIterator()
    {
        iterator->current_block = first_block;
        iterator->current_position = first_block->p_data;
    }

    Il2CppObject* CustomGrowableBlockArray::Next()
    {
        if (iterator->current_position != iterator->current_block->next_item)
            return *iterator->current_position++;
        if (iterator->current_block->next_block == NULL)
            return NULL;
        iterator->current_block = iterator->current_block->next_block;
        iterator->current_position = iterator->current_block->p_data;
        if (iterator->current_position == iterator->current_block->next_item)
            return NULL;
        return *iterator->current_position++;
    }

    void CustomGrowableBlockArray::Clear()
    {
        CustomArrayBlock *block = first_block;
        while (block != NULL)
        {
            block->next_item = block->p_data;
            block = block->next_block;
        }
        count = 0;
    }

    void CustomGrowableBlockArray::Destroy(LivenessState *state)
    {
        CustomArrayBlock *block = first_block;
        while (block != NULL)
        {
            CustomArrayBlock *data_block = block;
            block = block->next_block;
            state->reallocateArray(data_block, 0, state->callback_userdata);
        }
        delete iterator;
        delete this;
    }

    LivenessState::LivenessState(Il2CppClass* filter, uint32_t maxCount, Liveness::register_object_callback callback, void*callback_userdata, Liveness::ReallocateArrayCallback reallocateArray) :
        all_objects(NULL),
        filter(NULL),
        process_array(NULL),
        callback_userdata(NULL),
        filter_callback(NULL),
        reallocateArray(reallocateArray),
        traverse_depth(0)
    {
// construct liveness_state;
// allocate memory for the following structs
// all_objects: contains a list of all referenced objects to be able to clean the vtable bits after the traversal
// process_array. array that contains the objcets that should be processed. this should run depth first to reduce memory usage
// if all_objects run out of space, run through list, add objects that match the filter, clear bit in vtable and then clear the array.

        this->filter = filter;

        this->callback_userdata = callback_userdata;
        this->filter_callback = callback;

        all_objects = new CustomGrowableBlockArray(this);
        process_array = new CustomGrowableBlockArray(this);

        process_array_local.next_item = process_array_local.p_data;
        process_array_local.prev_block = NULL;
        process_array_local.next_block = NULL;

#if PROFILE_TIME_CONSUMING
        processObjectCount = 0;
#endif
    }

    LivenessState::~LivenessState()
    {
        all_objects->Destroy(this);
        process_array->Destroy(this);
    }

    void LivenessState::Finalize()
    {
        all_objects->ResetIterator();
        Il2CppObject* object = all_objects->Next();
        while (object != NULL)
        {
            CLEAR_OBJ(object);
            object = all_objects->Next();
        }
    }

    void LivenessState::Reset()
    {
        process_array->Clear();
    }

    void LivenessState::TraverseObjects()
    {
        Il2CppObject* object = NULL;

        traverse_depth++;
        while (process_array_local.next_item != process_array_local.p_data || !process_array->IsEmpty())
        {
            if (process_array_local.next_item > process_array_local.p_data)
            {
                object = *--process_array_local.next_item;
            }
            else
            {
                object = process_array->PopBack();
            }
            if (object != NULL)
            {
#if PROFILE_TIME_CONSUMING
                uint32_t startTime = os::Time::GetTicksMillisecondsMonotonic();
#endif
                TraverseGenericObject(object, this);
#if PROFILE_TIME_CONSUMING
                if (s_ProfileTimeConsuming)
                {
                    uint32_t delta = os::Time::GetTicksMillisecondsMonotonic() - startTime;
                    Il2CppClass* klass = GET_CLASS(object);
                    const char* klassName = Liveness::GetClassName(klass);
                    if (klassName != NULL)
                    {
                        std::map<const char*, uint32_t>::iterator it = traverseObjCostMap.find(klassName);
                        if (it == traverseObjCostMap.end() || it->second < delta)
                        {
                            traverseObjCostMap[klassName] = delta;
                        }
                    }
                }
#endif
            }
        }
        traverse_depth--;
    }

    void LivenessState::FilterObjects()
    {
        Il2CppObject* filtered_objects[64];
        int32_t num_objects = 0;

        Il2CppObject* value = all_objects->Next();
        while (value)
        {
            Il2CppObject* object = value;
            if (ShouldProcessValue(object, filter))
                filtered_objects[num_objects++] = object;
            if (num_objects == 64)
            {
                filter_callback(filtered_objects, 64, callback_userdata);
                num_objects = 0;
            }
            value = all_objects->Next();
        }

        if (num_objects != 0)
            filter_callback(filtered_objects, num_objects, callback_userdata);
    }

    void LivenessState::TraverseGenericObject(Il2CppObject* object, LivenessState* state)
    {
        IL2CPP_NOT_IMPLEMENTED_NO_ASSERT(LivenessState::TraverseGenericObject, "Use GC bitmap when we have one");

#if IL2CPP_HAS_GC_DESCRIPTORS
        size_t gc_desc = (size_t)(GET_CLASS(object)->gc_desc);

        if (gc_desc & (size_t)1)
            TraverseGCDescriptor(object, state);
        else
#endif
        if (GET_CLASS(object)->rank)
            TraverseArray((Il2CppArray*)object, state);
        else
            TraverseObject(object, state);
    }

    void LivenessState::TraverseObject(Il2CppObject* object, LivenessState* state)
    {
        TraverseObjectInternal(object, false, GET_CLASS(object), state);
    }

    void LivenessState::TraverseGCDescriptor(Il2CppObject* object, LivenessState* state)
    {
#define WORDSIZE ((int)sizeof(size_t)*8)
        int i = 0;
        size_t mask = (size_t)(GET_CLASS(object)->gc_desc);

        IL2CPP_ASSERT(mask & (size_t)1);

        for (i = 0; i < WORDSIZE - 2; i++)
        {
            size_t offset = ((size_t)1 << (WORDSIZE - 1 - i));
            if (mask & offset)
            {
                Il2CppObject* val = *(Il2CppObject**)(((char*)object) + i * sizeof(void*));
                AddProcessObject(val, state);
            }
        }
    }

    bool LivenessState::TraverseObjectInternal(Il2CppObject* object, bool isStruct, Il2CppClass* klass, LivenessState* state)
    {
        FieldInfo *field;
        Il2CppClass *p;
        bool added_objects = false;

        IL2CPP_ASSERT(object);

        if (!klass->size_inited)
        {
            IL2CPP_ASSERT(isStruct);
            return false;
        }

        // subtract the added offset for the vtable. This is added to the offset even though it is a struct
        if (isStruct)
            object--;

        for (p = klass; p != NULL; p = p->parent)
        {
            void* iter = NULL;
            while ((field = Class::GetFields(p, &iter)))
            {
                if (field->type->attrs & FIELD_ATTRIBUTE_STATIC)
                    continue;

                if (!FieldCanContainReferences(field))
                    continue;

                if (Type::IsStruct(field->type))
                {
                    char* offseted = (char*)object;
                    offseted += field->offset;
                    if (Type::IsGenericInstance(field->type))
                    {
                        IL2CPP_ASSERT(field->type->data.generic_class->cached_class);
                        added_objects |= TraverseObjectInternal((Il2CppObject*)offseted, true, field->type->data.generic_class->cached_class, state);
                    }
                    else
                        added_objects |= TraverseObjectInternal((Il2CppObject*)offseted, true, Type::GetClass(field->type), state);
                    continue;
                }

                if (field->offset == THREAD_STATIC_FIELD_OFFSET)
                {
                    IL2CPP_ASSERT(0);
                }
                else
                {
                    Il2CppObject* val = NULL;
                    Field::GetValue(object, field, &val);
                    added_objects |= AddProcessObject(val, state);
                }
            }
        }

        return added_objects;
    }

    void LivenessState::TraverseArray(Il2CppArray* array, LivenessState* state)
    {
        size_t i = 0;
        bool has_references;
        Il2CppObject* object = (Il2CppObject*)array;
        Il2CppClass* element_class;
        size_t elementClassSize;
        size_t array_length;

        IL2CPP_ASSERT(object);

        element_class = GET_CLASS(object)->element_class;
        has_references = !Class::IsValuetype(element_class);
        IL2CPP_ASSERT(element_class->size_inited != 0);

        FieldInfo* field;
        void* iter = NULL;
        while ((field = Class::GetFields(element_class, &iter)))
        {
            has_references |= FieldCanContainReferences(field);
            if (has_references)
                break;
        }

        if (!has_references)
            return;

        array_length = Array::GetLength(array);
        if (element_class->byval_arg.valuetype)
        {
            size_t items_processed = 0;
            elementClassSize = Class::GetArrayElementSize(element_class);
            for (i = 0; i < array_length; i++)
            {
                Il2CppObject* object = (Il2CppObject*)il2cpp_array_addr_with_size(array, (int32_t)elementClassSize, i);
                if (TraverseObjectInternal(object, 1, element_class, state))
                    items_processed++;

                // Add 64 objects at a time and then traverse
                if (ShouldTraverseObjects(items_processed, state->traverse_depth))
                    state->TraverseObjects();
            }
        }
        else
        {
            size_t items_processed = 0;
            for (i = 0; i < array_length; i++)
            {
                Il2CppObject* val =  il2cpp_array_get(array, Il2CppObject*, i);
                if (AddProcessObject(val, state))
                    items_processed++;

                // Add 64 objects at a time and then traverse
                if (ShouldTraverseObjects(items_processed, state->traverse_depth))
                    state->TraverseObjects();
            }
        }
    }

    bool LivenessState::AddProcessObject(Il2CppObject* object, LivenessState* state)
    {
        if (!object || IS_MARKED(object))
            return false;

#if PROFILE_TIME_CONSUMING
        ++state->processObjectCount;
#endif
        bool has_references = GET_CLASS(object)->has_references;
        if (has_references || ShouldProcessValue(object, state->filter))
        {
            state->all_objects->PushBack(object, state);
            MARK_OBJ(object);
        }
        // Check if klass has further references - if not skip adding
        if (has_references)
        {
            *state->process_array_local.next_item++ = object;
            if (state->process_array_local.next_item - state->process_array_local.p_data >= kMaxLocalElementsSize)
            {
                state->process_array->mutex.Acquire();
                for (int i = 0; i < kMaxLocalElementsSize; ++i)
                {
                    Il2CppObject* item = *--state->process_array_local.next_item;
                    state->process_array->PushBackNoLock(item, state);
                }
                state->process_array->mutex.Release();
            }
            return true;
        }

        return false;
    }

    bool LivenessState::ShouldProcessValue(Il2CppObject* val, Il2CppClass* filter)
    {
        Il2CppClass* val_class = GET_CLASS(val);
        if (filter && !ClassInlines::HasParentUnsafe(val_class, filter))
            return false;

        return true;
    }

    bool LivenessState::FieldCanContainReferences(FieldInfo* field)
    {
        if (Type::IsStruct(field->type))
            return true;
        if (field->type->attrs & FIELD_ATTRIBUTE_LITERAL)
            return false;
        if (field->type->type == IL2CPP_TYPE_STRING)
            return false;
        return Type::IsReference(field->type);
    }

    void* Liveness::AllocateStruct(Il2CppClass* filter, int max_object_count, register_object_callback callback, void* userdata, ReallocateArrayCallback reallocateArray)
    {
        // ensure filter is initialized so we can do fast (and lock free) check HasParentUnsafe
        Class::SetupTypeHierarchy(filter);
        LivenessState* state = new LivenessState(filter, max_object_count, callback, userdata, reallocateArray);
        // no allocations can happen beyond this point
        return state;
    }

    void Liveness::FreeStruct(void* state)
    {
        LivenessState* lstate = (LivenessState*)state;
        delete lstate;
    }

    void Liveness::Finalize(void* state)
    {
        LivenessState* lstate = (LivenessState*)state;
        lstate->Finalize();
    }

    void Liveness::FromRoot(Il2CppObject* root, void* state)
    {
        LivenessState* liveness_state = (LivenessState*)state;
        liveness_state->Reset();

        liveness_state->process_array->PushBack(root, liveness_state);

        liveness_state->TraverseObjects();

        //Filter objects and call callback to register found objects
        liveness_state->FilterObjects();
    }

    void Liveness::FromStatics(void* state)
    {
        LivenessState* liveness_state = (LivenessState*)state;
        const il2cpp::utils::dynamic_array<Il2CppClass*>& classesWithStatics = Class::GetStaticFieldData();

        liveness_state->Reset();

        for (il2cpp::utils::dynamic_array<Il2CppClass*>::const_iterator iter = classesWithStatics.begin();
             iter != classesWithStatics.end();
             iter++)
        {
            Il2CppClass* klass = *iter;
            FieldInfo *field;
            if (!klass)
                continue;
            if (klass->image == il2cpp_defaults.corlib)
                continue;
            if (klass->size_inited == 0)
                continue;

            void* fieldIter = NULL;
            while ((field = Class::GetFields(klass, &fieldIter)))
            {
                if (!vm::Field::IsNormalStatic(field))
                    continue;
                if (!LivenessState::FieldCanContainReferences(field))
                    continue;

                if (Type::IsStruct(field->type))
                {
                    char* offseted = (char*)klass->static_fields;
                    offseted += field->offset;
                    if (Type::IsGenericInstance(field->type))
                    {
                        IL2CPP_ASSERT(field->type->data.generic_class->cached_class);
                        LivenessState::TraverseObjectInternal((Il2CppObject*)offseted, true, field->type->data.generic_class->cached_class, liveness_state);
                    }
                    else
                    {
                        LivenessState::TraverseObjectInternal((Il2CppObject*)offseted, true, Type::GetClass(field->type), liveness_state);
                    }
                }
                else
                {
                    Il2CppObject* val = NULL;

                    Field::StaticGetValue(field, &val);

                    if (val)
                    {
                        LivenessState::AddProcessObject(val, liveness_state);
                    }
                }
            }
        }
        liveness_state->TraverseObjects();
        //Filter objects and call callback to register found objects
        liveness_state->FilterObjects();
    }

    int Liveness::GetStaticsCount()
    {
        const il2cpp::utils::dynamic_array<Il2CppClass*>& classesWithStatics = Class::GetStaticFieldData();
        return classesWithStatics.size();
    }
    
    void Liveness::CollectStaticsByWorker(void* state, int need_reset, int start_index, int batch_count)
    {
        LivenessState* liveness_state = (LivenessState*)state;
        const il2cpp::utils::dynamic_array<Il2CppClass*>& classesWithStatics = Class::GetStaticFieldData();

        if (need_reset != 0)
        {
            liveness_state->Reset();
        }

        int count = 0;
        for (il2cpp::utils::dynamic_array<Il2CppClass*>::const_iterator iter = classesWithStatics.begin() + start_index;
             iter != classesWithStatics.end();
             iter++)
        {
            if (count >= batch_count) break;
            Il2CppClass* klass = *iter;
            FieldInfo *field;
            if (!klass)
                continue;
            if (klass->image == il2cpp_defaults.corlib)
                continue;
            if (klass->size_inited == 0)
                continue;

#if PROFILE_TIME_CONSUMING
            uint32_t startTime = os::Time::GetTicksMillisecondsMonotonic();
#endif
            void* fieldIter = NULL;
            while ((field = Class::GetFields(klass, &fieldIter)))
            {
                if (!(field->type->attrs & FIELD_ATTRIBUTE_STATIC))
                    continue;
                if (!LivenessState::FieldCanContainReferences(field))
                    continue;
                // shortcut check for thread-static field
                if (field->offset == THREAD_STATIC_FIELD_OFFSET)
                    continue;

                if (Type::IsStruct(field->type))
                {
                    char* offseted = (char*)klass->static_fields;
                    offseted += field->offset;
                    if (Type::IsGenericInstance(field->type))
                    {
                        IL2CPP_ASSERT(field->type->data.generic_class->cached_class);
                        LivenessState::TraverseObjectInternal((Il2CppObject*)offseted, true, field->type->data.generic_class->cached_class, liveness_state);
                    }
                    else
                    {
                        LivenessState::TraverseObjectInternal((Il2CppObject*)offseted, true, Type::GetClass(field->type), liveness_state);
                    }
                }
                else
                {
                    Il2CppObject* val = NULL;

                    Field::StaticGetValue(field, &val);

                    if (val)
                    {
                        LivenessState::AddProcessObject(val, liveness_state);
                    }
                }
            }
#if PROFILE_TIME_CONSUMING
            if (s_ProfileTimeConsuming)
            {
                uint32_t delta = os::Time::GetTicksMillisecondsMonotonic() - startTime;
                const char* klassName = Liveness::GetClassName(klass);
                if (klassName != NULL)
                {
                    std::map<const char*, uint32_t>::iterator it = liveness_state->collectStaticCostMap.find(klassName);
                    if (it == liveness_state->collectStaticCostMap.end() || it->second < delta)
                    {
                        liveness_state->collectStaticCostMap[klassName] = delta;
                    }
                }
            }
#endif
        }
    }

    void Liveness::ProcessObjects(void* state)
    {
        LivenessState* liveness_state = (LivenessState*)state;
        liveness_state->TraverseObjects();
        liveness_state->FilterObjects();
    }

    void Liveness::FromStaticsByWorker(void* state, int worker_index, int worker_count)
    {
        LivenessState* liveness_state = (LivenessState*)state;
        const il2cpp::utils::dynamic_array<Il2CppClass*>& classesWithStatics = Class::GetStaticFieldData();

        liveness_state->Reset();

        int count = 0;
        for (il2cpp::utils::dynamic_array<Il2CppClass*>::const_iterator iter = classesWithStatics.begin();
             iter != classesWithStatics.end();
             iter++)
        {
            if (++count % worker_count != worker_index) continue;
            Il2CppClass* klass = *iter;
            FieldInfo *field;
            if (!klass)
                continue;
            if (klass->image == il2cpp_defaults.corlib)
                continue;
            if (klass->size_inited == 0)
                continue;

            void* fieldIter = NULL;
            while ((field = Class::GetFields(klass, &fieldIter)))
            {
                if (!(field->type->attrs & FIELD_ATTRIBUTE_STATIC))
                    continue;
                if (!LivenessState::FieldCanContainReferences(field))
                    continue;
                // shortcut check for thread-static field
                if (field->offset == THREAD_STATIC_FIELD_OFFSET)
                    continue;

                if (Type::IsStruct(field->type))
                {
                    char* offseted = (char*)klass->static_fields;
                    offseted += field->offset;
                    if (Type::IsGenericInstance(field->type))
                    {
                        IL2CPP_ASSERT(field->type->data.generic_class->cached_class);
                        LivenessState::TraverseObjectInternal((Il2CppObject*)offseted, true, field->type->data.generic_class->cached_class, liveness_state);
                    }
                    else
                    {
                        LivenessState::TraverseObjectInternal((Il2CppObject*)offseted, true, Type::GetClass(field->type), liveness_state);
                    }
                }
                else
                {
                    Il2CppObject* val = NULL;

                    Field::StaticGetValue(field, &val);

                    if (val)
                    {
                        LivenessState::AddProcessObject(val, liveness_state);
                    }
                }
            }
        }
        liveness_state->TraverseObjects();
        //Filter objects and call callback to register found objects
        liveness_state->FilterObjects();
    }

    int Liveness::TryStealTraverseJob(void* state, void* state_to_steal, int worker_count, int min_steal_count)
    {
        const int kMaxStealCount = 32;
        int successCount = 0;
        LivenessState* liveness_state = (LivenessState*)state;
        LivenessState* liveness_state_to_steal = (LivenessState*)state_to_steal;
        CustomGrowableBlockArray* process_array_to_steal = liveness_state_to_steal->process_array;
        if (!process_array_to_steal->IsEmpty() && process_array_to_steal->count >= min_steal_count * 2)
        {
            CustomGrowableBlockArray* process_array = liveness_state->process_array;
            int steal_count = process_array_to_steal->count / worker_count;
            if (steal_count < min_steal_count)
            {
                steal_count = min_steal_count;
            }
            else if (steal_count > kMaxStealCount)
            {
                steal_count = kMaxStealCount;
            }

            process_array_to_steal->mutex.Acquire();

            for (int i = 0; i < steal_count; ++i)
            {
                Il2CppObject* obj = process_array_to_steal->PopBackNoLock();
                if (obj != NULL)
                {
                    process_array->PushBackNoLock(obj, liveness_state);
                    ++successCount;
                }
                else if (process_array_to_steal->IsEmpty())
                {
                    break;
                }
            }

            process_array_to_steal->mutex.Release();
            
            liveness_state->TraverseObjects();
            liveness_state->FilterObjects();
        }

        return successCount;
    }

    void Liveness::ResetTimeConsumingReport(bool profileOn)
    {
#if PROFILE_TIME_CONSUMING
        s_ProfileTimeConsuming = profileOn;
#endif
    }

    const char* Liveness::GetClassName(Il2CppClass* klass)
    {
        if (klass == NULL)
            return "_";
        if (!klass->rank)
            return klass->name ? klass->name : "_";
        return il2cpp::utils::StringUtils::Printf("%s*", GetClassName(klass->element_class)).c_str();
    }

    const char* Liveness::GenerateTimeConsumingReport(void* state, uint32_t threshold)
    {
#if PROFILE_TIME_CONSUMING
        if (s_ProfileTimeConsuming)
        {
            LivenessState* liveness_state = (LivenessState*)state;

            std::string report;
            report += "ProcessObjectCount:";
            report += std::to_string(liveness_state->processObjectCount);
            report += "\nCollectStaticsByWorker: ";
            AppendTimeConsumingReport(report, liveness_state->collectStaticCostMap, threshold);
            report += "\nTraverseGenericObject: ";
            AppendTimeConsumingReport(report, liveness_state->traverseObjCostMap, threshold);

            return report.c_str();
        }
        else
        {
            return "profile off";
        }
#else
        return "feature unsupported";
#endif
    }


    static bool SortTimeConsumingRecord(std::pair<const char*, uint32_t>& a, std::pair<const char*, uint32_t>& b)
    {
        return a.second > b.second;
    }
    
    void Liveness::AppendTimeConsumingReport(std::string& report, std::map<const char*, uint32_t>& records, uint32_t threshold)
    {
#if PROFILE_TIME_CONSUMING
        std::vector<std::pair<const char*, uint32_t>> validRecords;
        for (std::map<const char*, uint32_t>::iterator i = records.begin(); i != records.end(); ++i)
        {
            if (i->first == NULL || i->second < threshold)
                continue;
            validRecords.push_back(std::pair<const char*, uint32_t>(i->first, i->second));
        }
        std::sort(validRecords.begin(), validRecords.end(), SortTimeConsumingRecord);

        char buffer[256];
        for (std::vector<std::pair<const char*, uint32_t>>::iterator i = validRecords.begin(); i != validRecords.end(); ++i)
        {
            std::snprintf(buffer, sizeof(buffer),"%s:%u ", i->first, i->second);
            report += buffer;
        }
#endif
    }

} /* namespace vm */
} /* namespace il2cpp */
