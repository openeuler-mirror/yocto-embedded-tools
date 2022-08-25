#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "ipc_base.h"
#include "message_parcel.h"
#include "ipc_skeleton.h"
#include "iremote_object.h"

namespace OHOS {

MessageParcel::MessageParcel() : Parcel(), fd_(-1) {}

MessageParcel::~MessageParcel() {}

bool MessageParcel::WriteFileDescriptor(int fd)
{
	if (fd < 0) {
		return false;
	}

	int dupFd = dup(fd);
	if (dupFd < 0) {
		return false;
	}

	fd_ = dupFd;

	return true;
}

int MessageParcel::ReadFileDescriptor()
{
	return fd_;
}

bool MessageParcel::ContainFileDescriptors() const
{
	return fd_ >= 0;
}

bool MessageParcel::WriteRawData(const void *data, size_t size)
{
	if (data == nullptr || size > MAX_RAWDATA_SIZE) {
		return false;
	}
	if (!WriteInt32(size)) {
		return false;
	}
	rawDataSize_ = size;
	return WriteUnpadBuffer(data, size);
}

const void *MessageParcel::ReadRawData(size_t size)
{
	int32_t bufferSize = ReadInt32();
	if (static_cast< unsigned int >(bufferSize) != size) {
		return nullptr;
	}
	rawDataSize_ = size;
	return ReadUnpadBuffer(size);
}

sptr< IRemoteObject > MessageParcel::ReadRemoteObject()
{
	return IPCSkeleton::GetContextObject();
}

bool MessageParcel::WriteRemoteObject(const sptr< IRemoteObject > &object)
{
	return true;
}

bool MessageParcel::WriteInterfaceToken(std::u16string name) {
	return WriteString16(name);
}

std::u16string MessageParcel::ReadInterfaceToken()
{
	return ReadString16();
}

size_t MessageParcel::GetRawDataSize() const
{
	return rawDataSize_;
}

} //namespace OHOS
