#include <string.h>
#include "securec.h"
#include "parameter.h"
#include "sha256.h"

#define SN_FILE "/etc/SN"
#define SN_LEN 65
#define UDID_LEN 65
#define DEV_BUF_LENGTH 3
#define HASH_LENGTH 32
static char *deviceType = "UNKNOWN";

static int GetHash(const char *input, char *output, int size)
{
	char buf[DEV_BUF_LENGTH] = { 0 };
	unsigned char hash[HASH_LENGTH] = { 0 };
	mbedtls_sha256_context context;

	mbedtls_sha256_init(&context);
	mbedtls_sha256_starts_ret(&context, 0);
	mbedtls_sha256_update_ret(&context, (const unsigned char*)input, strlen(input));
	mbedtls_sha256_finish_ret(&context, hash);

	for (size_t i = 0; i < HASH_LENGTH; i++) {
		unsigned char value = hash[i];
		memset_s(buf, DEV_BUF_LENGTH, 0, DEV_BUF_LENGTH);
		sprintf_s(buf, sizeof(buf), "%02X", value);
		if (strcat_s(output, size, buf) != 0) {
			return -1;
		}
	}
	return 0;
}

int GetDevUdid(char *udid, int size)
{
	FILE *fp;
	char *realPath = NULL;
	char sn[SN_LEN] = {0};
	char out[UDID_LEN] = {0};
	int ret;

	realPath = realpath(SN_FILE, NULL);
	if (realPath == NULL) {
		printf("realpath fail.\n");
		goto err_realpath;
	}

	fp = fopen(realPath, "r");
	if (fp == NULL) {
		printf("open SN fail.\n");
		goto err_fopen;
	}

	ret = fscanf_s(fp, "%s", sn, SN_LEN);
	if (ret < 1) {
		printf("get sn fail.\n");
		goto err_out;
	}

	ret = GetHash(sn, out, UDID_LEN);
	if (ret < 0) {
		printf("get hash fail.\n");
		goto err_out;
	}

	ret = sprintf_s(udid, size, "%s", out);
	if (ret <= 0) {
		printf("sprintf_s error.\n");
		goto err_out;
	}

	fclose(fp);
	return 0;
err_out:
	fclose(fp);
err_fopen:
	free(realPath);
err_realpath:
	return -1;
}

char * GetDeviceType(void)
{
	return deviceType;
}
