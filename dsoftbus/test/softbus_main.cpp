#include <stdio.h>
#include <iostream>
#include <vector>
#include <string.h>
#include <curses.h>
#include <unistd.h>

#include "dsoftbus/discovery_service.h"
#include "dsoftbus/softbus_bus_center.h"
#include "dsoftbus/session.h"

std::vector< std::string > devList;
char receivedData[256];

bool hasPublish = false;
bool hasDiscovery = false;
const size_t LINE_WIDTH = 80;

// #define ImReceive

#define clientName "Client"

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

void DisplayTitle()
{
	erase();
	PrintMargin();
	PrintTitle("openEuler");
	PrintTitle("SoftBus");
	PrintTitle("");
	PrintTitle("");
}

void DisplayMain()
{
	DisplayTitle();
	PrintTitle("");
	PrintTitle("");
	PrintTitle("");
	PrintMessage("Press key:");
	PrintMessage("[D]DiscoveryService       [T]DataTransmission");
	PrintTitle("");
	PrintTitle("");
	PrintMargin();
}

void DisplayDevList()
{
	char msg[256];
	DisplayTitle();
	PrintTitle("");
	PrintTitle("");
	PrintMessage("Press key:");
	PrintMessage("[S]StopDiscovery");
	PrintMessage("Devices list:");
	for (size_t i = 0; i < devList.size(); ++i) {
		sprintf(msg, "  %lu. %s", i, devList[i].c_str()); 
		PrintMessage(msg);
	}
	PrintTitle("");
	PrintMargin();
}

void DisplayReceivedData()
{
	DisplayTitle();
	PrintTitle("");
	PrintTitle("");
	PrintTitle("");
	PrintTitle("");
//	PrintMessage("Press key:");
//	PrintMessage("[Esc]Back");
	PrintMessage("Received data:");
	PrintMessage(receivedData);
	PrintTitle("");
	PrintTitle("");
	PrintMargin();
}

void PublishSuccess(int publishId)
{
	// printf("CB: publish %d done", publishId);
}

void PublishFailed(int publishId, PublishFailReason reason)
{
	// printf("CB: publish %d failed, reason=%d\n", publishId, (int)reason);
}

void DeviceFound(const DeviceInfo *device)
{
	devList.push_back(device->devName);
	//DisplayDevList();
}

void DiscoverySuccess(int subscribeId)
{
	// printf("CB: discover subscribeId=%d\n", subscribeId);
}

void DiscoveryFailed(int subscribeId, DiscoveryFailReason reason)
{
	// printf("CB: discover subscribeId=%d failed, reason=%d\n", subscribeId, (int)reason);
}

int SessionOpened(int sessionId, int result)
{
	// printf("CB: session %d open ret=%d\n", sessionId, result);
	return 0;
}

void SessionClosed(int sessionId)
{
	// printf("CB: session %d closed\n", sessionId);
}

void ByteRecived(int sessionId, const void *data, unsigned int dataLen)
{
	memset(receivedData, 0, sizeof(receivedData));
	memcpy(receivedData, data, dataLen);
	DisplayReceivedData();
	// printf("CB: session %d received %u bytes data=%s\n", sessionId, dataLen, (const char *)data);
}

void MessageReceived(int sessionId, const void *data, unsigned int dataLen)
{
	// printf("CB: session %d received %u bytes message=%s\n", sessionId, dataLen, (const char *)data);
}

unsigned char cData[] = "My Client Test";

int PublishServiceInterface()
{
	PublishInfo info = {
		.publishId = 123,
		.mode = DISCOVER_MODE_PASSIVE,
		.medium = COAP,
		.freq = LOW,
		.capability = "hicall",
		.capabilityData = cData,
		.dataLen = sizeof(cData),
	};
	IPublishCallback cb = {
		.OnPublishSuccess = PublishSuccess,
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
		.OnDiscoverySuccess = DiscoverySuccess,
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
	SessionAttribute attr = {
		.dataType = TYPE_BYTES,
		.lintTypeNum = 1,
		.attr = {RAW_STREAM},
	};
	int lt = LINK_TYPE_WIFI_P2P;
	attr.lintType = &lt;
	return OpenSession(SessionName, peerName, peerId, "MyGroup", &attr);
}

void Discovering()
{
	devList.clear();
	DiscoveryInterface();
	while (true) {
		DisplayDevList();
		char op = getch();
		if (op == 'S' || op == 's') {
			break;
		}
	}
	StopDiscovery(clientName, 123);
}

void SendData()
{
	NodeBasicInfo *dev;
	int32_t dev_num;
	GetAllNodeDeviceInfo(clientName, &dev, &dev_num);
	CreateSessionServerInterface("SessionTest1");
	char op;
	int32_t selectId = 0;
	char input[128];
	while (true) {
		DisplayTitle();
		PrintTitle("");
		PrintTitle("");
		PrintMessage("Press key:");
		PrintMessage("[U]Up    [D]Down    [S]Select");
		PrintTitle("");
		for (int32_t i = 0; i < dev_num; ++i) {
			strcpy(input, i == selectId ? "*" : " ");
			strcat(input, dev->deviceName);
			PrintMessage(input);
		}
		PrintTitle("");
		PrintMargin();
		op = getch();
		if (op == 'U' || op == 'u') {
			--selectId;
			if (selectId < 0) {
				selectId = dev_num - 1;
			}
		} else if (op == 'D' || op == 'd') {
			++selectId;
			if (selectId == dev_num) {
				selectId = 0;
			}
		} else if (op == 'S' || op == 's') {
			break;
		}
	}
	size_t inputLen = 0;
	if (dev_num) {
		int sessionId = OpenSessionInterface("SessionTest1", "SessionTest2", dev[selectId].networkId);
		while (sessionId >= 0) {
			input[inputLen] = '\0';
			DisplayTitle();
			PrintTitle("");
			PrintTitle("");
			PrintMessage("Press key:");
			PrintMessage("[Enter]Send      [Esc]Back");
			PrintMessage("Message:");
			PrintMessage(input);
			PrintTitle("");
			PrintMargin();
			op = getch();
			if (op == 10) {
				SendBytes(sessionId, input, strlen(input));
				inputLen = 0;
			} else if (op == 8) {
				if (inputLen) {
					--inputLen;
				}
			} else if (op == 27) {
				break;
			} else {
				input[inputLen] = op;
				++inputLen;
			}
		}
		CloseSession(selectId);
	}
}

void ReceiveData()
{
	DisplayReceivedData();
	CreateSessionServerInterface("SessionTest2");
	while (true) {
#if 0
		char op = getch();
		if (op == 27) {
			break;
		}
#endif
	}
}

int main(int argc, char **argv)
{
	bool needContinue = true;
	initscr();
#ifdef ImReceive
	PublishServiceInterface();
	ReceiveData();
//	UnPublishService(clientName, 123);
#else
	while (needContinue) {
		DisplayMain();
		char op = getch();
		switch(op) {
		case 'D':
		case 'd':
			Discovering();
			break;
		case 'T':
		case 't':
			SendData();
		case 27:
			needContinue = false;
		}
	}
#endif
	endwin();
	return 0;
}
