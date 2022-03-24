#ifndef _DEVICE_AUTH_H
#define _DEVICE_AUTH_H

#include <stdint.h>
#include <stdbool.h>

#define FIELD_PEER_CONN_DEVICE_ID "peerConnDeviceId"
#define FIELD_SERVICE_PKG_NAME "servicePkgName"
#define FIELD_IS_CLIENT "isClient"
#define FIELD_KEY_LENGTH "keyLength"
#define FIELD_CONFIRMATION "confirmation"

#define REQUEST_ACCEPTED 0x80000006

typedef struct {
    bool (*onTransmit)(int64_t requestId, const uint8_t *data, uint32_t dataLen);
    void (*onSessionKeyReturned)(int64_t requestId, const uint8_t *sessionKey, uint32_t sessionKeyLen);
    void (*onFinish)(int64_t requestId, int operationCode, const char *returnData);
    void (*onError)(int64_t requestId, int operationCode, int errorCode, const char *errorReturn);
    char *(*onRequest)(int64_t requestId, int operationCode, const char *reqParams);
} DeviceAuthCallback;

typedef struct {
    int32_t (*processData)(int64_t authReqId, const uint8_t *data, uint32_t dataLen, const DeviceAuthCallback *gaCallback);
    int32_t (*authDevice)(int64_t authReqId, const char *authParams, const DeviceAuthCallback *gaCallback);
} GroupAuthManager;

typedef struct {
    void (*onDeviceNotTrusted)(const char *peerUdid);
} DataChangeListener;

typedef struct {
    int32_t (*regDataChangeListener)(const char *appId, const DataChangeListener *listener);
} DeviceGroupManager;

int InitDeviceAuthService(void);
void DestroyDeviceAuthService(void);
const GroupAuthManager *GetGaInstance(void);
const DeviceGroupManager *GetGmInstance(void);

#endif
