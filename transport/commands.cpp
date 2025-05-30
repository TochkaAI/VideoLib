#include "commands.h"
#include "pproto/commands/pool.h"

namespace pproto {
namespace command {

#define REGISTRY_COMMAND_SINGLPROC(COMMAND, UUID) \
    const QUuidEx COMMAND = command::Pool::Registry{UUID, #COMMAND, false};

#define REGISTRY_COMMAND_MULTIPROC(COMMAND, UUID) \
    const QUuidEx COMMAND = command::Pool::Registry{UUID, #COMMAND, true};

REGISTRY_COMMAND_MULTIPROC(VideoSenderStart,  "152fe2dc-2511-4ab2-a32e-5f4bee104560")
REGISTRY_COMMAND_MULTIPROC(VideoSenderStop,   "81d55737-c662-425f-8164-56a01013a4ed")
REGISTRY_COMMAND_MULTIPROC(VideoStreamInfo,   "56c50366-03a6-4c82-85c8-2c373dbcbd7d")
REGISTRY_COMMAND_MULTIPROC(VideoFeedbackPort, "3b139e78-206d-4619-b1fe-1fb22de33684")
REGISTRY_COMMAND_MULTIPROC(VideoSenderFrame,  "2cb8b7dc-4917-4d8e-8873-78a633ebd619")

#undef REGISTRY_COMMAND_SINGLPROC
#undef REGISTRY_COMMAND_MULTIPROC

} // namespace command

namespace data {

bserial::RawVector VideoSenderStart::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << terminalId;
    stream << deviceIndex;
    stream << fps;
    stream << frameScale;
    stream << chunkStep;
    stream << compression;
    stream << sendPort;
    stream << widthScale;
    B_SERIALIZE_RETURN
}

void VideoSenderStart::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> terminalId;
    stream >> deviceIndex;
    stream >> fps;
    stream >> frameScale;
    stream >> chunkStep;
    stream >> compression;
    stream >> sendPort;
    stream >> widthScale;
    B_DESERIALIZE_END
}

bserial::RawVector VideoSenderStop::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << terminalId;
    stream << deviceIndex;
    B_SERIALIZE_RETURN
}

void VideoSenderStop::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> terminalId;
    stream >> deviceIndex;
    B_DESERIALIZE_END
}

bserial::RawVector VideoStreamInfo::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << deviceIndex;
    stream << frameNumber;
    stream << cvType;
    stream << realWidth;
    stream << realHeight;
    stream << scale;
    stream << scaleWidth;
    stream << scaleHeight;
    stream << fps;
    stream << realFps;
    B_SERIALIZE_RETURN
}

void VideoStreamInfo::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> deviceIndex;
    stream >> frameNumber;
    stream >> cvType;
    stream >> realWidth;
    stream >> realHeight;
    stream >> scale;
    stream >> scaleWidth;
    stream >> scaleHeight;
    stream >> fps;
    stream >> realFps;
    B_DESERIALIZE_END
}

bserial::RawVector VideoSenderError::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << deviceIndex;
    B_SERIALIZE_RETURN
}

void VideoSenderError::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> deviceIndex;
    B_DESERIALIZE_END
}

bserial::RawVector FrameHeader::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << deviceIndex;
    stream << frameNumber;
    stream << cvType;
    stream << realWidth;
    stream << realHeight;
    stream << realStep;
    stream << scale;
    stream << scaleWidth;
    stream << scaleHeight;
    stream << scaleStep;
    stream << fps;
    stream << realFps;
    stream << chunkCount;
    stream << chunkStep;
    stream << dataSize;
    stream << compress;
    B_SERIALIZE_RETURN
}

void FrameHeader::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> deviceIndex;
    stream >> frameNumber;
    stream >> cvType;
    stream >> realWidth;
    stream >> realHeight;
    stream >> realStep;
    stream >> scale;
    stream >> scaleWidth;
    stream >> scaleHeight;
    stream >> scaleStep;
    stream >> fps;
    stream >> realFps;
    stream >> chunkCount;
    stream >> chunkStep;
    stream >> dataSize;
    stream >> compress;
    B_DESERIALIZE_END
}

bserial::RawVector VideoSenderFrame::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << frameHeader;
    stream << data;
    B_SERIALIZE_RETURN
}

void VideoSenderFrame::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> frameHeader;
    stream >> data;
    B_DESERIALIZE_END
}
} // namespace data
} // namespace pproto
