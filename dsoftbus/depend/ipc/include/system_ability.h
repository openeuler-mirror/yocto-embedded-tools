#ifndef _SYSTEM_ABILITY_H
#define _SYSTEM_ABILITY_H

#ifdef __cplusplus
//#include <unordered_map>
//#include <functional>
#include <string>
//#include <memory>
#include "parcel.h"

#define DECLARE_SYSTEM_ABILITY(className)               \
    public:                                             \
        virtual std::string GetClassName() override {   \
        return #className;                              \
    }

#define DECLARE_BASE_SYSTEM_ABILITY(className)          \
    public:                                             \
        virtual std::string GetClassName() = 0;

namespace OHOS {

#define REGISTER_SYSTEM_ABILITY_BY_ID(abilityClassName, systemAbilityId, runOnCreate)   \
    const bool abilityClassName##_##RegisterResult =                                    \
        SystemAbility::MakeAndRegisterAbility(new abilityClassName(systemAbilityId, runOnCreate));

class IRemoteObject;

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
