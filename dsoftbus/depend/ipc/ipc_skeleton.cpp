#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "ipc_base.h"
#include "iremote_object.h"
#include "ipc_skeleton.h"

namespace OHOS {

const char *IPC_SERVER_SOCKET_ADDR = "/tmp/ipc.socket.server";
const char *IPC_CLIENT_SOCKET_ADDR = "/tmp/ipc.socket.client";

sptr< IRemoteObject > IPCSkeleton::obj_ = nullptr;
int IPCSkeleton::socketFd_ = -1;
bool IPCSkeleton::isServer_ = true;

pid_t IPCSkeleton::GetCallingPid()
{
	return getpid();
}

uid_t IPCSkeleton::GetCallingUid()
{
	return getuid();
}

bool IPCSkeleton::SetContextObject(sptr< IRemoteObject > &object)
{
	obj_ = object;
	return true;
}

sptr< IRemoteObject > IPCSkeleton::GetContextObject()
{
	if (obj_ == nullptr) {
		obj_ = new IRemoteObject();
	}
	return obj_;
}

bool IPCSkeleton::SocketListening(bool isServer)
{
	if (socketFd_ >= 0) {
		IPC_LOG("Socket is opened\n");
		return false;
	}

	isServer_ = isServer;

	const char *ipcPath = isServer ? IPC_SERVER_SOCKET_ADDR : IPC_CLIENT_SOCKET_ADDR;
	unlink(ipcPath);

	socketFd_ = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (socketFd_ < 0) {
		IPC_LOG("Socket failed errno=%d\n", errno);
		return false;
	}

	struct sockaddr_un socketAddr;
	memset(&socketAddr, 0, sizeof(socketAddr));
	socketAddr.sun_family = AF_UNIX;
	strcpy(socketAddr.sun_path, ipcPath);
	int ret = bind(socketFd_, (struct sockaddr *)&socketAddr, sizeof(socketAddr));
	if (ret < 0) {
		IPC_LOG("Bind socket failed errno=%d\n", errno);
		close(socketFd_);
		socketFd_ = -1;
		return false;
	}

	ret = listen(socketFd_, 3);
	if (ret < 0) {
		IPC_LOG("listen socket failed errno=%d\n", errno);
		close(socketFd_);
		socketFd_ = -1;
		return false;
	}
	return true;
}

int IPCSkeleton::SocketReadFd()
{
	if (socketFd_ < 0) {
		IPC_LOG("Read fd from an uninitialized socket\n");
		return -1;
	}

	struct sockaddr_un acceptAddr;
	socklen_t sockLen = sizeof(acceptAddr);
	int recvFd = accept(socketFd_, (struct sockaddr *)&acceptAddr, &sockLen);
	if (recvFd < 0) {
		IPC_LOG("Accept failed errno=%d\n", errno);
		return -1;
	}

	struct msghdr msg;
	struct iovec iov[1];
	char buf[100] = "";
	msg.msg_name = nullptr;
	msg.msg_namelen = 0;
	iov[0].iov_base = buf;
	iov[0].iov_len = 100;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;
	char cm[CMSG_LEN(sizeof(int))];
	msg.msg_control = cm;
	msg.msg_controllen = CMSG_LEN(sizeof(int));
	int ret = recvmsg(recvFd, &msg, 0);
	if (ret < 0) {
		IPC_LOG("Receive error, errno=%d\n", errno);
		close(recvFd);
		return -1;
	}

	struct cmsghdr *cmsgPtr = CMSG_FIRSTHDR(&msg);
	if (cmsgPtr == nullptr || cmsgPtr->cmsg_len != CMSG_LEN(sizeof(int)) ||
		cmsgPtr->cmsg_level != SOL_SOCKET ||
		cmsgPtr->cmsg_type != SCM_RIGHTS) {
		IPC_LOG("Received wrong data\n");
		close(recvFd);
		return -1;
	}
	close(recvFd);
	return *((int *)CMSG_DATA(cmsgPtr));
}

bool IPCSkeleton::SocketWriteFd(int fd)
{
	int socketFd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (socketFd < 0) {
		IPC_LOG("Socket failed errno=%d\n", errno);
		return false;
	}

	struct sockaddr_un socketAddr;
	memset(&socketAddr, 0, sizeof(socketAddr));
	socketAddr.sun_family = AF_UNIX;
	strcpy(socketAddr.sun_path, isServer_ ? IPC_CLIENT_SOCKET_ADDR : IPC_SERVER_SOCKET_ADDR);
	int ret = connect(socketFd, (struct sockaddr *)&socketAddr, sizeof(socketAddr));
	if (ret < 0) {
		IPC_LOG("Connect failed errno=%d\n", errno);
		close(socketFd);
		return false;
	}

	struct msghdr msg;
	struct iovec iov[1];
	char buf[100] = "IPC Socket Data with File Descriptor";
	msg.msg_name = nullptr;
	msg.msg_namelen = 0;
	iov[0].iov_base = buf;
	iov[0].iov_len = 100;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;
	char cm[CMSG_LEN(sizeof(int))];
	msg.msg_control = cm;
	msg.msg_controllen = CMSG_LEN(sizeof(int));
	struct cmsghdr *cmsgPtr = CMSG_FIRSTHDR(&msg);
	cmsgPtr->cmsg_len = CMSG_LEN(sizeof(int));
	cmsgPtr->cmsg_level = SOL_SOCKET;
	cmsgPtr->cmsg_type = SCM_RIGHTS;
	*((int *)CMSG_DATA(cmsgPtr)) = fd;
	ret = sendmsg(socketFd, &msg, 0);
	if (ret < 0) {
		IPC_LOG("Send failed errno=%d\n", errno);
	}
	close(socketFd);
	return ret >= 0;
}

} //namespace OHOS
