#ifndef _DEPEND_CPP_H
#define _DEPEND_CPP_H

//IRemoteBroker
#ifdef __cplusplus
#include <unordered_map>
#include <functional>
#include <string>
#include <memory>
#include "parcel.h"

typedef enum TypePermissionState {
    PERMISSION_NOT_GRANTED = -1,
    PERMISSION_GRANTED = 0,
} PermissionState;

#define DECLARE_SYSTEM_ABILITY(className)               \
    public:                                             \
        virtual std::string GetClassName() override {   \
        return #className;                              \
    }

#define DECLARE_BASE_SYSTEM_ABILITY(className)          \
    public:                                             \
        virtual std::string GetClassName() = 0;

namespace OHOS {
namespace Security {
namespace Permission {
};
};

#define REGISTER_SYSTEM_ABILITY_BY_ID(abilityClassName, systemAbilityId, runOnCreate)   \
    const bool abilityClassName##_##RegisterResult =                                    \
        SystemAbility::MakeAndRegisterAbility(new abilityClassName(systemAbilityId, runOnCreate));

enum {
    SOFTBUS_SERVER_SA_ID = 4700,
};

template <typename INTERFACE> class BrokerDelegator {
};

class IRemoteObject;

class MessageParcel : public Parcel {
public:
    bool WriteFileDescriptor(int fd) {return true;}
    bool WriteRawData(const void *data, size_t size) {return true;}
    void *ReadRawData(size_t size) {return NULL;}
    sptr<IRemoteObject> ReadRemoteObject() {return NULL;}
    int ReadFileDescriptor() {return 0;}
};

class MessageOption {
};

class IRemoteBroker {
#define DECLARE_INTERFACE_DESCRIPTOR(DESCRIPTOR)                        \
    static inline const std::u16string metaDescriptor_ = {DESCRIPTOR};   \
    static inline const std::u16string &GetDescriptor()                 \
    {                                                                   \
        return metaDescriptor_;                                         \
    }
};

class IRemoteObject : public virtual Parcelable {
    public:
    virtual int SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) {return 0;}
    class DeathRecipient : public RefBase {
        public:
        virtual void OnRemoteDied(const wptr<IRemoteObject> &object) {};
    };
    virtual bool RemoveDeathRecipient(const sptr<DeathRecipient> &recipient) {return true;};
    virtual bool AddDeathRecipient(const sptr<DeathRecipient> &recipient) {return true;};
    virtual bool Marshalling(Parcel &parcel) const override {return true;}
};

template <typename INTERFACE> class IRemoteProxy: public INTERFACE, public virtual RefBase {
    public:
    IRemoteProxy(const sptr<IRemoteObject> &object) {
    }
    sptr<IRemoteObject> Remote() {return NULL;}
};

class IPCObjectStub : public IRemoteObject {
    public:
    virtual int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) {return 0;}
};

template <typename INTERFACE> class IRemoteStub: public IPCObjectStub, public INTERFACE {
    public:
    virtual int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) {return 0;}
};

class IPCSkeleton {
    public:
    static sptr<IRemoteObject> GetContextObject() {return NULL;}
    static pid_t GetCallingPid() {return 0;}
    static pid_t GetCallingUid() {return 0;}
};

class SystemAbility {
    DECLARE_BASE_SYSTEM_ABILITY(SystemAbility);
    public:
    static bool MakeAndRegisterAbility(SystemAbility* systemAbility) {return true;}
    bool Publish(sptr<IRemoteObject> systemAbility) {return true;}
    protected:
    SystemAbility(bool runOnCreate = false) {}
    SystemAbility(int32_t systemAbilityId, bool runOnCreate = false) {}
    virtual void OnStart() = 0;
    virtual void OnStop() = 0;
};

};
#endif

#endif
