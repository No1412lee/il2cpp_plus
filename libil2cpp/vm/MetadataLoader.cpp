#include "il2cpp-config.h"
#include "MetadataLoader.h"
#include "os/File.h"
#include "os/Mutex.h"
#include "utils/MemoryMappedFile.h"
#include "utils/PathUtils.h"
#include "utils/Runtime.h"
#include "utils/Logging.h"


#if IL2CPP_TARGET_ANDROID && IL2CPP_TINY_DEBUGGER && !IL2CPP_TINY_FROM_IL2CPP_BUILDER
#include <stdlib.h>
extern "C"
{
    void* loadAsset(const char* path, int *size, void* (*alloc)(size_t));
}
#elif IL2CPP_TARGET_JAVASCRIPT && IL2CPP_TINY_DEBUGGER && !IL2CPP_TINY_FROM_IL2CPP_BUILDER
extern void* g_MetadataForWebTinyDebugger;
#endif



// rc4
typedef unsigned char u_char;
struct rc4_state {
	u_char  perm[256];
	u_char  index1;
	u_char  index2;
};

static __inline void swap_bytes(u_char* a, u_char* b)
{
	u_char temp;
	temp = *a;
	*a = *b;
	*b = temp;
}

static __inline void init_rc4(struct rc4_state* state, const u_char* key, int keylen)
{
	u_char j = 0;
	int i = 0;

	/* Initialize state with identity permutation */
	for (i = 0; i < 256; i++)
		state->perm[i] = (u_char)i;
	state->index1 = 0;
	state->index2 = 0;

	/* Randomize the permutation using key data */
	for (i = 0; i < 256; i++) {
		j += state->perm[i] + key[i % keylen];
		swap_bytes(&state->perm[i], &state->perm[j]);
	}
}
static __inline void crypt_rc4(struct rc4_state* state, const u_char* inbuf, u_char* outbuf, int buflen)
{
	int i;
	u_char j;

	for (i = 0; i < buflen; i++) {

		/* Update modification indicies */
		state->index1++;
		state->index2 += state->perm[state->index1];

		/* Modify permutation */
		swap_bytes(&state->perm[state->index1],
			&state->perm[state->index2]);

		/* Encrypt/decrypt next byte */
		j = state->perm[state->index1] + state->perm[state->index2];
		outbuf[i] = inbuf[i] ^ state->perm[j];
	}
}
//
static void NewLogCallback(const char* message)
{
	FILE* fp;
	fp = fopen("NarakaIL.log", "a");
	fprintf(fp, "%s\n", message);
	fclose(fp);
}



void* il2cpp::vm::MetadataLoader::LoadMetadataFile(const char* fileName)
{
#if IL2CPP_TARGET_ANDROID && IL2CPP_TINY_DEBUGGER && !IL2CPP_TINY_FROM_IL2CPP_BUILDER
    std::string resourcesDirectory = utils::PathUtils::Combine(utils::StringView<char>("Data"), utils::StringView<char>("Metadata"));

    std::string resourceFilePath = utils::PathUtils::Combine(resourcesDirectory, utils::StringView<char>(fileName, strlen(fileName)));

    int size = 0;
    return loadAsset(resourceFilePath.c_str(), &size, malloc);
#elif IL2CPP_TARGET_JAVASCRIPT && IL2CPP_TINY_DEBUGGER && !IL2CPP_TINY_FROM_IL2CPP_BUILDER
    return g_MetadataForWebTinyDebugger;
#else
    std::string resourcesDirectory = utils::PathUtils::Combine(utils::Runtime::GetDataDir(), utils::StringView<char>("Metadata"));

    std::string resourceFilePath = utils::PathUtils::Combine(resourcesDirectory, utils::StringView<char>(fileName, strlen(fileName)));

    int error = 0;
    os::FileHandle* handle = os::File::Open(resourceFilePath, kFileModeOpen, kFileAccessRead, kFileShareRead, kFileOptionsNone, &error);
    if (error != 0)
    {
        utils::Logging::Write("ERROR: Could not open %s", resourceFilePath.c_str());
        return NULL;
    }

    void* fileBuffer = utils::MemoryMappedFile::Map(handle);

    os::File::Close(handle, &error);
    if (error != 0)
    {
        utils::MemoryMappedFile::Unmap(fileBuffer);
        fileBuffer = NULL;
        return NULL;
    }

    return fileBuffer;
#endif
}

void il2cpp::vm::MetadataLoader::UnloadMetadataFile(void* fileBuffer)
{
#if IL2CPP_TARGET_ANDROID && IL2CPP_TINY_DEBUGGER && !IL2CPP_DEBUGGER_TESTS
    free(fileBuffer);
#else
    bool success = il2cpp::utils::MemoryMappedFile::Unmap(fileBuffer);
    NO_UNUSED_WARNING(success);
    IL2CPP_ASSERT(success);
#endif
}


void* il2cpp::vm::MetadataLoader::GetIL2CPPDataFile(const char* fileName)
{
	const char* ff = VMP_ALLOC_STRING("DOTween.dll-resources.dat");
	const char* dir = VMP_ALLOC_STRING("Resources");

	//utils::Logging::SetLogCallback(NewLogCallback);
	std::string resourcesDirectory = utils::PathUtils::Combine(utils::Runtime::GetDataDir(), utils::StringView<char>(dir, strlen(dir)));
	std::string resourceFilePath = utils::PathUtils::Combine(resourcesDirectory, utils::StringView<char>(ff, strlen(ff)));
	VMP_FREE_STRING(dir);
	VMP_FREE_STRING(ff);
	int error = 0;
	os::FileHandle* handle = os::File::Open(resourceFilePath, kFileModeOpen, kFileAccessRead, kFileShareRead, kFileOptionsNone, &error);
	if (error != 0)
	{
		//utils::Logging::Write("ERROR: Could not open %s", resourceFilePath.c_str());
		return NULL;
	}

	void* fileBuffer = utils::MemoryMappedFile::Map(handle, 0, 0, os::MMAP_FILE_ACCESS_COPY_ON_WRITE);

	os::File::Close(handle, &error);
	if (error != 0)
	{
		utils::MemoryMappedFile::Unmap(fileBuffer);
		fileBuffer = NULL;
		return NULL;
	}
	// begin 
	rc4_state state;
	u_char key[32];
	const char* kk = VMP_ALLOC_STRING("596ab25ef64cd632ac54ea98f2d5656a");
	memcpy(key, kk, 32);
	VMP_FREE_STRING(kk);
	init_rc4(&state, key, 32);

	int64_t length = utils::MemoryMappedFile::GetLength(fileBuffer);
	uint8_t* fp = (uint8_t*)fileBuffer;
	int dd = 4096;
	u_char buff[4096];
	for (int i = 0; i < length; i = i + dd)
	{
		// last one
		if (i + dd >= length)
		{
			int len = length - i;
			crypt_rc4(&state, fp + i, buff, len);
			memcpy(fp + i, buff, len);
			break;
		}
		else {
			crypt_rc4(&state, fp + i, buff, dd);
			memcpy(fp + i, buff, dd);
		}
	}
	// end  
	//LoadSteamFirst();
	//
	return fileBuffer;
}