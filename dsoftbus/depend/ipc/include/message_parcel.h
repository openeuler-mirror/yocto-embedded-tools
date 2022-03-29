#ifndef _MESSAGE_PARCEL_H_
#define _MESSAGE_PARCEL_H_

#include "parcel.h"
#include "refbase.h"

namespace OHOS {

class IRemoteObject;
class MessageParcel : public Parcel {
public:
	MessageParcel();
	~MessageParcel();
	bool WriteFileDescriptor(int fd);
	int ReadFileDescriptor();
	bool WriteRawData(const void *data, size_t size);
	const void *ReadRawData(size_t size);
	bool ContainFileDescriptors() const;
	sptr< IRemoteObject > ReadRemoteObject();
	bool WriteRemoteObject(const sptr< IRemoteObject > &object);
private:
	static constexpr size_t MAX_RAWDATA_SIZE = 128 * 1024 * 1024; // 128M
	int fd_;
};

} //namespace OHOS

#endif // _MESSAGE_PARCEL_H_
