#include "udp.h"

#ifdef Q_OS_WIN
#include <malloc.h>
#include <winsock2.h>
#else
#include <byteswap.h>
#include <netinet/in.h>
#endif

#include <type_traits>

namespace video::transport {

const quint32 udpSignature = *((quint32*)"VSND");

template <typename T>
constexpr typename std::enable_if_t<sizeof(T) == 1, T> bswapT (T value) noexcept
{
    return value;
}

#ifdef Q_OS_WIN
template <typename T>
/*constexpr*/ typename std::enable_if_t<sizeof(T) == 2, T> bswapT (T value) noexcept
{
    return ntohs(value);
}

template <typename T>
constexpr typename std::enable_if_t<sizeof(T) == 4, T> bswapT (T value) noexcept
{
    return ntohl(value);
}

template <typename T>
constexpr typename std::enable_if_t<sizeof(T) == 8, T> bswapT (T value) noexcept
{
    return ((((uint64_t)ntohl(value)) << 32) + ntohl((value) >> 32));
}
#else
template <typename T>
/*constexpr*/ typename std::enable_if_t<sizeof(T) == 2, T> bswapT (T value) noexcept
{
    return bswap_16(value);
}
template <typename T>
constexpr typename std::enable_if_t<sizeof(T) == 4, T> bswapT (T value) noexcept
{
    return bswap_32(value);
}
template <typename T>
constexpr typename std::enable_if_t<sizeof(T) == 8, T> bswapT (T value) noexcept
{
    return bswap_64(value);
}
#endif // Q_OS_WIN

#if (BYTE_ORDER == LITTLE_ENDIAN) || defined(Q_OS_WIN)
#  define ntoh(x) bswapT (x)
#  define hton(x) bswapT (x)
#else // BYTE_ORDER == BIG_ENDIAN
   /* The host byte order is the same as network byte order,
   so these functions are all just identity. */
#  define ntoh(x) (x)
#  define hton(x) (x)
#endif

bool checkUdpSignature(char*& buff)
{
    quint32 udpSign = *((quint32*) buff);
    udpSign = ntoh(udpSign);
    buff += sizeof(quint32);
    return (udpSignature == udpSign);
}

quint8 readFrameCode(char*& buff)
{
    quint8 code = *((quint8*) buff);
    buff += sizeof(quint8);
    return code;
}

#define WRITE_VALUE(VAL) \
    *((std::add_pointer_t<std::remove_const_t<decltype(VAL)>>) buff) = hton(VAL); \
    buff += sizeof(VAL); \
    writeSize += sizeof(VAL);

#define READ_VALUE(VAL) \
    VAL = ntoh(*((std::add_pointer_t<std::remove_const_t<decltype(VAL)>>) buff)); \
    buff += sizeof(VAL);

/*
#define WRITE_VALUE(VAL) \
    *reinterpret_cast<std::add_pointer_t<std::remove_const_t<decltype(VAL)>>>(buff) = hton(VAL); \
    buff += sizeof(VAL); \
    writeSize += sizeof(VAL);

#define READ_VALUE(VAL) \
    VAL = ntoh(*reinterpret_cast<std::add_pointer_t<std::remove_const_t<decltype(VAL)>>>(buff)); \
    buff += sizeof(VAL);
*/

qint32 FrameHeader::writeToBuffer(char* buff)
{
    (void) buff;
    qint32 writeSize = 0;
    quint8 frameCode = 0;

    WRITE_VALUE( udpSignature )
    WRITE_VALUE( frameCode    )

    WRITE_VALUE( deviceIndex  )
    WRITE_VALUE( frameNumber  )
    WRITE_VALUE( cvType       )
    WRITE_VALUE( realWidth    )
    WRITE_VALUE( realHeight   )
    WRITE_VALUE( realStep     )
    WRITE_VALUE( scale        )
    WRITE_VALUE( scaleWidth   )
    WRITE_VALUE( scaleHeight  )
    WRITE_VALUE( scaleStep    )
    WRITE_VALUE( fps          )
    WRITE_VALUE( realFps      )
    WRITE_VALUE( chunkCount   )
    WRITE_VALUE( chunkStep    )
    WRITE_VALUE( dataSize     )
    WRITE_VALUE( compress     )

    return writeSize;
}

FrameHeader::Ptr FrameHeader::readFromBuffer(char* buff)
{
    Ptr fh {new FrameHeader};

    READ_VALUE( fh->deviceIndex )
    READ_VALUE( fh->frameNumber )
    READ_VALUE( fh->cvType      )
    READ_VALUE( fh->realWidth   )
    READ_VALUE( fh->realHeight  )
    READ_VALUE( fh->realStep    )
    READ_VALUE( fh->scale       )
    READ_VALUE( fh->scaleWidth  )
    READ_VALUE( fh->scaleHeight )
    READ_VALUE( fh->scaleStep   )
    READ_VALUE( fh->fps         )
    READ_VALUE( fh->realFps     )
    READ_VALUE( fh->chunkCount  )
    READ_VALUE( fh->chunkStep   )
    READ_VALUE( fh->dataSize    )
    READ_VALUE( fh->compress    )

    return fh;
}

qint32 FrameItem::writeToBuffer(char* buff, char* data)
{
    (void) buff;
    qint32 writeSize = 0;
    quint8 frameCode = 1;

    WRITE_VALUE( udpSignature )
    WRITE_VALUE( frameCode    )

    WRITE_VALUE( deviceIndex  )
    WRITE_VALUE( frameNumber  )
    WRITE_VALUE( chunkNumber  )
    WRITE_VALUE( chunkStep    )
    WRITE_VALUE( chunkSize    )
    WRITE_VALUE( dataSize     )

    // Запись куска кадра
    memcpy(buff, data, chunkSize);

    return (writeSize + chunkSize);
}

void FrameItem::readFromBuffer(char*& buff)
{
    READ_VALUE( deviceIndex )
    READ_VALUE( frameNumber )
    READ_VALUE( chunkNumber )
    READ_VALUE( chunkStep   )
    READ_VALUE( chunkSize   )
    READ_VALUE( dataSize    )
}

qint32 FrameFinish::writeToBuffer(char* buff)
{
    (void) buff;
    qint32 writeSize = 0;
    quint8 frameCode = 2;

    WRITE_VALUE( udpSignature )
    WRITE_VALUE( frameCode    )

    WRITE_VALUE( senderId     )
    WRITE_VALUE( deviceIndex  )
    WRITE_VALUE( frameNumber  )
    WRITE_VALUE( static_cast<qint32>(status) )

    return writeSize;
}

void FrameFinish::readFromBuffer(char* buff)
{
    READ_VALUE( senderId    )
    READ_VALUE( deviceIndex )
    READ_VALUE( frameNumber )

    qint32 stat;
    READ_VALUE( stat )
    status = static_cast<FrameFinish::Status>(stat);
}

qint32 FrameFeedback::writeToBuffer(char* buff)
{
    (void) buff;
    qint32 writeSize = 0;

    WRITE_VALUE( udpSignature )

    WRITE_VALUE( terminalId  )
    WRITE_VALUE( senderId    )
    WRITE_VALUE( deviceIndex )
    WRITE_VALUE( frameNumber )
    WRITE_VALUE( static_cast<qint32>(status) )

    quint16 chunkCount = chunkNumbers.count();
    WRITE_VALUE( chunkCount )
    for (quint16 i = 0; i < chunkCount; ++i)
    {
        quint16 index = chunkNumbers.at(i);
        WRITE_VALUE( index )
    }

    return writeSize;
}

FrameFeedback::Ptr FrameFeedback::readFromBuffer(char* buff)
{
    Ptr ff {new FrameFeedback};

    READ_VALUE( ff->terminalId  )
    READ_VALUE( ff->senderId    )
    READ_VALUE( ff->deviceIndex )
    READ_VALUE( ff->frameNumber )

    qint32 stat;
    READ_VALUE( stat )
    ff->status = static_cast<FrameFeedback::Status>(stat);

    quint16 chunkCount;
    READ_VALUE( chunkCount )
    for (quint16 i = 0; i < chunkCount; ++i)
    {
        quint16 index;
        READ_VALUE( index )
        ff->chunkNumbers.append(index);
    }

    return ff;
}

#undef WRITE_VALUE
#undef READ_VALUE

FrameBuff::~FrameBuff()
{
    if (_ptr)
        free(_ptr);
}

FrameBuff::Ptr FrameBuff::alloc(size_t size)
{
    FrameBuff::Ptr buff {new FrameBuff};
#ifdef Q_OS_WIN
    buff->_ptr = (char*) malloc(size);
#else
    buff->_ptr = (char*) aligned_alloc(32, size);
#endif
    buff->_size = int(size);

    return buff;
}

} // namespace video::transport
