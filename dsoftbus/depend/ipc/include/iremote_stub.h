#ifndef _IPC_IREMOTE_STUB_H_
#define _IPC_IREMOTE_STUB_H_

#include "ipc_object_stub.h"

namespace OHOS {

template< typename INTERFACE >
class IRemoteStub : public IPCObjectStub, public INTERFACE {
public:
	IRemoteStub();
	virtual ~IRemoteStub() = default;
	sptr< IRemoteObject > AsObject() override;
};

template< typename INTERFACE >
IRemoteStub< INTERFACE >::IRemoteStub() : IPCObjectStub() {}

template< typename INTERFACE >
sptr< IRemoteObject > IRemoteStub< INTERFACE >::AsObject()
{
	return this;
}

} // namespace OHOS

#endif // _IPC_IREMOTE_STUB_H_