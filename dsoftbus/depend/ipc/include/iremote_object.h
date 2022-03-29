#ifndef _IPC_REMOTE_OBJECT_H_
#define _IPC_REMOTE_OBJECT_H_

#include "message_parcel.h"
#include "message_option.h"

namespace OHOS {

class IRemoteBroker : public virtual RefBase {
public:
	IRemoteBroker() = default;
	virtual ~IRemoteBroker() override = default;
	virtual sptr< IRemoteObject > AsObject() = 0;
	static inline sptr< IRemoteBroker > AsImplement(const sptr< IRemoteObject > &object)
	{
		return nullptr;
	}
};

#define DECLARE_INTERFACE_DESCRIPTOR(DESCRIPTOR)				\
	static inline const std::u16string metaDescriptor_ = {DESCRIPTOR} ;	\
	static inline const std::u16string &GetDescriptor()			\
	{									\
		return metaDescriptor_;						\
	}

class IRemoteObject : public virtual Parcelable {
public:
	class DeathRecipient : public RefBase {
	public:
		virtual void OnRemoteDied(const wptr< IRemoteObject > &object) {}
	};

	virtual int SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option);
	virtual bool AddDeathRecipient(const sptr< DeathRecipient > &recipient);
	virtual bool RemoveDeathRecipient(const sptr< DeathRecipient > &recipient);
	virtual bool Marshalling(Parcel &parcel) const override;
};

} // namespace OHOS

#endif // _IPC_REMOTE_OBJECT_H_
