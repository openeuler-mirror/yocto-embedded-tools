#include "ipc_base.h"
#include "iremote_object.h"
#include "ipc_skeleton.h"

key_t g_send_shm_key = 0x544F53;
key_t g_receive_shm_key = 0x52544F;

namespace OHOS {

const int32_t GET_SA_REQUEST_CODE = 2;

bool IRemoteObject::AddDeathRecipient(const sptr< DeathRecipient > &recipient)
{
	return true;
}

bool IRemoteObject::RemoveDeathRecipient(const sptr< DeathRecipient > &recipient)
{
	return true;
}

int IRemoteObject::SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
	IpcShmData *shmPtr = NULL;

	if (code == GET_SA_REQUEST_CODE) {
		return 0;
	}

	shmPtr = OpenShm(g_send_shm_key);
	if (shmPtr == nullptr) {
		return -1;
	}

	// waiting previous ipc
	while (shmPtr->needReply);

	shmPtr->requestCode = code;
	shmPtr->inputSz = data.GetDataSize();
	memcpy(shmPtr->inputData, (void *)data.GetData(), shmPtr->inputSz);
	if (data.ContainFileDescriptors()) {
		shmPtr->containFd = true;
		if (!IPCSkeleton::SocketWriteFd(data.ReadFileDescriptor())) {
			IPC_LOG("Send File Descriptor failed\n");
			shmdt((void*)shmPtr);
			return -1;
		}
	}
	shmPtr->needReply = true;

	while (shmPtr->needReply);
	reply.WriteUnpadBuffer(shmPtr->outputData, shmPtr->outputSz);
	if (shmPtr->containFd) {
		if (!reply.WriteFileDescriptor(IPCSkeleton::SocketReadFd())) {
			IPC_LOG("Reveive reply fd failed");
			shmdt((void *)shmPtr);
			return -1;
		}
		shmPtr->containFd = false;
	}
	shmdt((void *)shmPtr);
	return 0;
}

bool IRemoteObject::Marshalling(Parcel &parcel) const
{
	return true;
}

} // namespace OHOS
