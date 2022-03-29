#ifndef _IPC_BASE_H_
#define _IPC_BASE_H_

#include <cstdio>
#include <cstring>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <mutex>
#include <condition_variable>

#define IPC_LOG(fmt, args...) \
	printf("[IPC LOG %s:%u]" fmt, __FILE__, __LINE__, ##args);

extern key_t g_send_shm_key;
extern key_t g_receive_shm_key;

const int IPC_SHM_FLAG = IPC_CREAT | 0666;

const size_t DATA_SIZE = 0x20000;

struct IpcShmData {
	size_t inputSz;
	size_t outputSz;
	char inputData[DATA_SIZE];
	char outputData[DATA_SIZE];
	volatile bool needReply;
	uint32_t requestCode;
	volatile bool containFd;
};

static inline IpcShmData *OpenShmCommon(key_t shmKey, int flag)
{
	int shmFd = shmget(shmKey, sizeof(IpcShmData), flag);
	if (shmFd < 0) {
		IPC_LOG("Get shm failed\n");
		return nullptr;
	}
	void *shmPtr = shmat(shmFd, 0, 0);
	if (shmPtr == (void *)-1) {
		IPC_LOG("Map shm failed\n");
		return nullptr;
	}
	return (IpcShmData *)shmPtr;
}

static inline IpcShmData *OpenShm(key_t shmKey)
{
	return OpenShmCommon(shmKey, IPC_SHM_FLAG);
}

static inline IpcShmData *OpenShmExcl(key_t shmKey)
{
	return OpenShmCommon(shmKey, IPC_SHM_FLAG | IPC_EXCL);
}

#endif // _IPC_BASE_H_
