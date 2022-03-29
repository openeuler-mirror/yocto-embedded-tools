#ifndef OHOS_IPC_IREMOTE_PROXY_H
#define OHOS_IPC_IREMOTE_PROXY_H
 
#include "ipc_skeleton.h"
 
namespace OHOS {
template <typename INTERFACE> class IRemoteProxy : public INTERFACE {
public:
	explicit IRemoteProxy(const sptr<IRemoteObject> &object);
	~IRemoteProxy() override = default;
	sptr<IRemoteObject> Remote();

protected:
	sptr<IRemoteObject> AsObject() override;

private:
	const sptr<IRemoteObject> remoteObj_;
};

template <typename INTERFACE>
sptr<IRemoteObject> IRemoteProxyremoteObj_ = nullptr;

template <typename INTERFACE>
IRemoteProxy<INTERFACE>::IRemoteProxy(const sptr<IRemoteObject> &object) : remoteObj_(object) {}

template <typename INTERFACE>
sptr<IRemoteObject> IRemoteProxy<INTERFACE>::AsObject()
{
	if (remoteObj_ != nullptr) {
		return remoteObj_;
	}
	return IPCSkeleton::GetContextObject();
}

template <typename INTERFACE>
sptr<IRemoteObject> IRemoteProxy<INTERFACE>::Remote()
{
	return AsObject();
}

} // namespece OHOS
#endif // OHOS_IPC_IREMOTE_PROXY_H