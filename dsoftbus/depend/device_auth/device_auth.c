#include <string.h>
#include "device_auth.h"

#define AUTH_START "auth_start"
#define AUTH_END   "auth_end"

#define KEY_LEN 33
const uint8_t sessionkey_default[KEY_LEN]="01234567890123456789012345678901";

static int32_t ProcessData(int64_t authReqId, const uint8_t *data, uint32_t datalen,
		const DeviceAuthCallback *callback)
{
	if (!strncmp((const char *)data, AUTH_START, strlen(AUTH_START))) {
		callback->onTransmit(authReqId, (const uint8_t *)AUTH_END, strlen(AUTH_END));
	}
	callback->onSessionKeyReturned(authReqId, sessionkey_default, KEY_LEN - 1);
	return 0;
}


static int32_t AuthDevice(int64_t authReqId, const char *authParams,
		const DeviceAuthCallback *callback)
{
	callback->onTransmit(authReqId, (const uint8_t *)AUTH_START, strlen(AUTH_START));
	return 0;
}

static int32_t RegDataChangeListener(const char* appId, const DataChangeListener *listener)
{
	return 0;
}

static GroupAuthManager g_groupAuthManager = {
	.processData = ProcessData,
	.authDevice = AuthDevice,
};

static DeviceGroupManager g_deviceGroupManager = {
	.regDataChangeListener = RegDataChangeListener,
};

const GroupAuthManager *GetGaInstance(void)
{
	return (const GroupAuthManager *)&g_groupAuthManager;
}

const DeviceGroupManager *GetGmInstance(void)
{
	return (const DeviceGroupManager *)&g_deviceGroupManager;
}

int InitDeviceAuthService(void)
{
	return 0;
}

void DestroyDeviceAuthService(void)
{
}
