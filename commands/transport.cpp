/* clang-format off */
/*****************************************************************************
  The MIT License

  Copyright © 2026 TochkaAI

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*****************************************************************************/

#include "commands/transport.h"
#include "pproto/commands/pool.h"

namespace pproto {
namespace command {

#define REGISTRY_COMMAND_SINGLPROC(COMMAND, UUID) \
    const QUuidEx COMMAND = command::Pool::Registry{UUID, #COMMAND, false};

#define REGISTRY_COMMAND_MULTIPROC(COMMAND, UUID) \
    const QUuidEx COMMAND = command::Pool::Registry{UUID, #COMMAND, true};

REGISTRY_COMMAND_MULTIPROC(VideoSenderSocket, "9127959b-2a4f-40b6-897f-ae864ad9b5a8")
REGISTRY_COMMAND_MULTIPROC(VideoSenderStart,  "152fe2dc-2511-4ab2-a32e-5f4bee104560")
REGISTRY_COMMAND_MULTIPROC(VideoSenderStop,   "81d55737-c662-425f-8164-56a01013a4ed")
REGISTRY_COMMAND_MULTIPROC(VideoStreamInfo,   "56c50366-03a6-4c82-85c8-2c373dbcbd7d")
REGISTRY_COMMAND_MULTIPROC(VideoFeedbackPort, "3b139e78-206d-4619-b1fe-1fb22de33684")
REGISTRY_COMMAND_MULTIPROC(VideoSenderFrame,  "2cb8b7dc-4917-4d8e-8873-78a633ebd619")

#undef REGISTRY_COMMAND_SINGLPROC
#undef REGISTRY_COMMAND_MULTIPROC

} // namespace command

namespace data {

#ifdef PPROTO_QBINARY_SERIALIZE

void VideoSenderStart::toRaw(bserial::DataStream& stream) const
{
    B_SERIALIZE_V1
    stream << terminalId;
    stream << deviceIndex;
    stream << fps;
    stream << frameScale;
    stream << chunkStep;
    stream << compression;
    stream << sendPort;
    stream << sizeScale;
    stream << sizeSide;
    B_SERIALIZE_RETURN
}

void VideoSenderStart::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect)
    stream >> terminalId;
    stream >> deviceIndex;
    stream >> fps;
    stream >> frameScale;
    stream >> chunkStep;
    stream >> compression;
    stream >> sendPort;
    stream >> sizeScale;
    stream >> sizeSide;
    B_DESERIALIZE_END
}

void VideoSenderStop::toRaw(bserial::DataStream& stream) const
{
    B_SERIALIZE_V1
    stream << terminalId;
    stream << deviceIndex;
    B_SERIALIZE_RETURN
}

void VideoSenderStop::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect)
    stream >> terminalId;
    stream >> deviceIndex;
    B_DESERIALIZE_END
}

void VideoStreamInfo::toRaw(bserial::DataStream& stream) const
{
    B_SERIALIZE_V1
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
    B_DESERIALIZE_V1(vect)
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

void VideoSenderError::toRaw(bserial::DataStream& stream) const
{
    B_SERIALIZE_V1
    stream << deviceIndex;
    B_SERIALIZE_RETURN
}

void VideoSenderError::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect)
    stream >> deviceIndex;
    B_DESERIALIZE_END
}

void FrameHeader::toRaw(bserial::DataStream& stream) const
{
    B_SERIALIZE_V1
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
    B_DESERIALIZE_V1(vect)
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

void VideoSenderFrame::toRaw(bserial::DataStream& stream) const
{
    B_SERIALIZE_V1
    stream << frameHeader;
    stream << data;
    B_SERIALIZE_RETURN
}

void VideoSenderFrame::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect)
    stream >> frameHeader;
    stream >> data;
    B_DESERIALIZE_END
}

#endif // PPROTO_QBINARY_SERIALIZE

} // namespace data
} // namespace pproto
