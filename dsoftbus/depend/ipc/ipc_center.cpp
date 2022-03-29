#include <thread>
#include <cstring>

#include "ipc_base.h"
#include "ipc_center.h"
#include "ipc_skeleton.h"

namespace OHOS {

IpcCenter::IpcCenter() : threadNum_(0), needStop_(false) {}

bool IpcCenter::ShmInit(key_t shmKey)
{
	IpcShmData *shmPtr = OpenShm(shmKey);
	if (shmPtr == nullptr) {
		IPC_LOG("Create shm with key=0x%x\n", shmKey);
		return false;
	}
	shmPtr->needReply = false;
	shmPtr->containFd = false;
	shmdt((void *)shmPtr);
	return true;
}

bool IpcCenter::Init(bool isServer, IPCObjectStub *stub)
{
	if (stub == nullptr) {
		IPC_LOG("Invalid stub\n");
		return false;
	}

	if (isServer && (!ShmInit(g_send_shm_key) || !ShmInit(g_receive_shm_key))) {
		IPC_LOG("Shm inti failed\n");
		return false;
	}

	if (isServer) {
		std::swap(g_send_shm_key, g_receive_shm_key);
	}

	ipcStub_ = stub;

	if (!IPCSkeleton::SocketListening(isServer)) {
		IPC_LOG("Starting socket listen failed\n");
		return false;
	}

	return ThreadCreate();
}

void IpcCenter::ProcessHandle()
{
	do {
		IpcShmData *shmPtr = OpenShm(g_receive_shm_key);
		if (shmPtr == nullptr) {
			return;
		}
		while (!shmPtr->needReply) {
			usleep(10);
		}
		MessageParcel data, reply;
		MessageOption option;
		data.WriteUnpadBuffer(shmPtr->inputData, shmPtr->inputSz);
		if (shmPtr->containFd) {
			shmPtr->containFd = false;
			if (!data.WriteFileDescriptor(IPCSkeleton::SocketReadFd())) {
				IPC_LOG("Process file descriptor failed");
				shmdt((void *)shmPtr);
				return;
			}
		}
		ipcStub_->OnRemoteRequest(shmPtr->requestCode, data, reply, option);
		shmPtr->outputSz = reply.GetDataSize();
		memcpy(shmPtr->outputData, (void*)reply.GetData(), shmPtr->outputSz);
		if (reply.ContainFileDescriptors()) {
			if (!IPCSkeleton::SocketWriteFd(reply.ReadFileDescriptor())) {
				IPC_LOG("Send file descriptor in reply failed\n")
				shmdt((void *)shmPtr);
				return;
			}
			shmPtr->containFd = true;
		}
		shmPtr->needReply = false;
		shmdt((void*)shmPtr);
	} while (!needStop_);
}

bool IpcCenter::ThreadCreate()
{
	if (!threadNum_) {
		++threadNum_;
		std::thread new_thread(std::bind(&IpcCenter::ProcessHandle, this));
		new_thread.detach();
		return true;
	}
	return false;
}

} // namespace OHOS
