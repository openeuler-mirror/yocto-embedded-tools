#include <stdio.h>
#include <string.h>
#include <curses.h>
#include <unistd.h>
/*
void PublishSuccess(int publishId)
{
	printf("CB: publish %d done", publishId);
}

void PublishFailed(int publishId, PublishFailReason reason)
{
	printf("CB: publish %d failed, reason=%d\n", publishId, (int)reason);
}

void DeviceFound(const DeviceInfo *device)
{
	printf("CB: a device found id=%s name=%s\n", device->devId, device->devName);
}

void DiscoverySuccess(int subscribeId)
{
	printf("CB: discover subscribeId=%d\n", subscribeId);
}

void DiscoveryFailed(int subscribeId, DiscoveryFailReason reason)
{
	printf("CB: discover subscribeId=%d failed, reason=%d\n", subscribeId, (int)reason);
}

void SessionOpened(int sessionId, int result)
{
	printf("CB: session %d open ret=%d\n", subscribeId, result);
}

void SessionClosed(int sessionId)
{
	printf("CB: session $d closed\n", sessionId);
}

void ByteRecived(int sessionId, const void *data, unsigned int dataLen)
{
	printf("CB: session %d received %u bytes data=%s\n", sessionId, dataLen, (const char *)data);
}

void MessageReceived(int sessionId, const void *data, unsigned int dataLen)
{
	printf("CB: session %d received %u bytes message=%s\n", sessionId, dataLen, (const char *)data);
}

unsigned char cData[] = "My Client Test";

int PublishServiceInterface()
{
	PublishInfo info = {
		.publishId = 123,
		.mode = DISCOVER_MODE_ACTIVE,
		.medium = COAP,
		.freq = LOW,
		.capability = "hicall",
		capabilityData = cData,
		dataLen = sizeof(cData),
	};
	IPublishCallback cb = {
		.OnPublishSucess = PublishSuccess,
		.OnPublishFail = PublishFailed,
	};
	return PublishService(clientName, &info, &cb);
}

int DiscoveryInterface()
{
	SubscribeInfo info = {
		.subscribeId = 123,
		.mode = DISCOVER_MODE_ACTIVE,
		.medium = COAP,
		.freq = LOW,
		.isSameAccount = false,
		.isWakeRemote = false,
		.capability = "hicall",
		.capabilityData = cData,
		.dataLen = sizeof(cData),
	};
	IDiscoveryCallback cb = {
		.OnDeviceFound = DeviceFound,
		.OnDiscoverFailed = DiscoveryFailed,
		.OnDiscoverySuccesss = DiscoverySuccess,
	};
	return StartDiscovery(clientName, &info, &cb);
}

int CreateSessionServerInterface(const char *SessionName)
{
	ISessionListener cb = {
		.OnSessionOpened = SessionOpened,
		.OnSessionClosed = SessionClosed,
		.OnBytesReceived = ByteRecived,
		.OnMessageReceived = MessageReceived,
	};
	return CreateSessionServer(clientName, SessionName, &cb);
}

int OpenSessionInterface(const char *SessionName, const char *peerName, const char *peerId)
{
	SessionAttrbute attr = {
		.dataType = TYPE_BYTES,
		.lintTypeNum = 1,
		attr = {RAW_STREAM},
	};
	int lt = LINK_TYPE_WIFI_P2P;
	attr.lintType = &lt;
	return OpenSession(SessionName, peerName, peerId, "MyGroup", &attr);
}
*/

bool hasPublish = false;
bool hasDiscovery = false;
const size_t LINE_WIDTH = 80;

void PrintMargin()
{
	for (size_t i = 0; i < LINE_WIDTH; ++i) {
		printw("#");
	}
	printw("\n");
}

void PrintTitle(const char *msg)
{
	size_t len = strlen(msg);
	if (len + 2 <= LINE_WIDTH) {
		size_t emptyLen = LINE_WIDTH - len - 2;
		size_t preEmptyLen = emptyLen / 2;
		size_t postEmptyLen = emptyLen - preEmptyLen;
		printw("#");
		while (preEmptyLen--) {
			printw(" ");
		}
		printw("%s", msg);
		while (postEmptyLen--) {
			printw(" ");
		}
		printw("#\n");
	} else {
		printw("#%s#\n", msg);
	}
}

void PrintMessage(const char *msg)
{
	size_t len = strlen(msg);
	if (len + 6 <= LINE_WIDTH) {
		size_t emptyLen = LINE_WIDTH - len - 6;
		printw("#  ");
		printw("%s", msg);
		while (emptyLen--) {
			printw(" ");
		}
		printw("  #\n");
	} else {
		printw("#%s#\n", msg);
	}
}

void DisplayMain()
{
	char msg[256];
	erase();
	PrintMargin();
	PrintTitle("openEuler");
	PrintTitle("SoftBus");
	PrintTitle("");
	PrintTitle("");
	PrintTitle("");
	PrintTitle("");
	PrintMessage("Press key:");
	strcpy(msg, hasPublish ? "[U]UnpublishService" : "[P]PublishService  ");
	strcat(msg, "      ");
	strcat(msg, hasDiscovery ? "[S]StopDiscovery   " : "[D]DiscoveryService");
	PrintMessage(msg);
	PrintMessage("[T]DataTransmission");
	PrintTitle("");
	PrintTitle("");
	PrintMargin();
}

void DataTransmission()
{
	NodeBasicInfo *dev;
	int32_t dev_num;
	GetAllNodeDeviceInfo(clientName, &dev, &dev_num);
	CreateSessionServerInterface("SessionTest1");
	char op;
	while (true) {
		DisplayDevList();
		op = getch();
	}
	OpenSession("SessionTest1", "SessionTest2", peerId)
}

void ProcessKeyMain()
{
	char op = getch();
	switch(op) {
	case 'P':
	case 'p':
		hasPublish = true;
		break;
	case 'U':
	case 'u':
		hasPublish = false;
		break;
	case 'D':
	case 'd':
		hasDiscovery = true;
		break;
	case 'S':
	case 's':
		hasDiscovery = false;
		break;
	case 'T':
	case 't':
		DataTransmission();
	}
}

int main(int argc, char **argv)
{
	pid_t pid = fork();
	if (pid < 0) {
		puts("Fork failed");
	} else if (pid > 0) {
		initscr();
		while (true) {
			DisplayMain();
			ProcessKey();
		}
		endwin();
		/*if (argc > 1) {
			CreateSessionServerInterface("SessionTest2");
		} else {
			
			

		}*/
	} else {
		/*InitSoftBusServer();
		while (true) {
			pause();
		}*/
	}
	return 0;
}