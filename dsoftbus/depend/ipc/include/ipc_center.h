#ifndef _IPC_CENTER_H_
#define _IPC_CENTER_H_

#include "iremote_object.h"
#include "ipc_object_stub.h"

namespace OHOS {

class IpcCenter {
public:
	IpcCenter();
	bool Init(bool isServer, IPCObjectStub *stub);
	bool ThreadCreate();
	void ProcessHandle();
private:
	bool ShmInit(key_t ShmKey);
	size_t threadNum_;
	IPCObjectStub *ipcStub_;
	bool needStop_;
};

} // namespace OHOS

#endif //_IPC_CENTER_H_
