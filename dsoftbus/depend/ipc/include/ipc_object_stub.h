#ifndef _IPC_OBJECT_STUB_H_
#define _IPC_OBJECT_STUB_H_

#include "iremote_object.h"

namespace OHOS {

class IPCObjectStub : public IRemoteObject {
public:
	IPCObjectStub();
	~IPCObjectStub();
	virtual int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option);
};

} // namespace OHOS

#endif // _IPC_OBJECT_STUB_H_