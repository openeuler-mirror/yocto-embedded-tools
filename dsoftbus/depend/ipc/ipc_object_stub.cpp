#include "ipc_base.h"
#include "ipc_object_stub.h"

namespace OHOS {

IPCObjectStub::IPCObjectStub() {}

IPCObjectStub::~IPCObjectStub() {}

int IPCObjectStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
	IPC_LOG("IPCObjectStub::OnRemoteRequest Called\n");
	return -1;
}

} // namespace OHOS
