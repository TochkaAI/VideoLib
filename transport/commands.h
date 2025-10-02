#pragma once

#include "transport/compression.h"
#include "transport/frames/base.h"

#include "pproto/commands/base.h"
#include "pproto/serialize/qbinary.h"

namespace pproto {
namespace command {

//----------------------------- Список команд --------------------------------

/**
  Команда информирует о том, что сокет используется для передачи видеопотока
*/
extern const QUuidEx VideoSenderSocket;

/**
  Старт передачи видеопотока
*/
extern const QUuidEx VideoSenderStart;

/**
  Остановка передачи видеопотока
*/
extern const QUuidEx VideoSenderStop;

/**
  Команда содержит информацию о геометрии видеопотока, рассылается в момент
  начала нового видеопотока или при подключении терминала
*/
extern const QUuidEx VideoStreamInfo;

/**
  Команда возвращает номер порта необходимый для работы механизма обратной
  связи при передаче видеопотока. Порт обратной связи должен быть  получен
  до вызова команды VideoSenderStart. Номер  порта  передается  в  нулевом
  индексе поля tag коммуникационного сообщения
*/
extern const QUuidEx VideoFeedbackPort;

/**
  Команда для отправки кадра
*/
extern const QUuidEx VideoSenderFrame;

} // namespace command

/**
  Значимая сторона (высота/ширина) масштабирования для параметра
  VideoSenderStart::sizeScale
*/
enum class SizeSide : quint32
{
    Height = 0,
    Width  = 1
};

namespace data {

struct VideoSenderStart : Data<&command::VideoSenderStart,
                                Message::Type::Command>
{
    // Сессионный идентификатор терминала (не используется при передаче
    // видеопотока через TCP-соединение)
    quint64 terminalId = {0};

    // Идентификатор видео-устройства с которого нужно передавать видео
    quint8 deviceIndex = {0};

    // Запрашиваемый FPS видеопотока
    quint8 fps = {25};

    // Коэффициент уменьшения фрейма (в процентах), 100% означает, что кадр
    // не будет масштабироваться
    quint8 frameScale = {100};

    // Параметр задает (высоту/ширину) масштабированного кадра, имеет приоритет
    // над frameScale
    quint16 sizeScale = {0};

    // Определяет значимую сторону (высота/ширина) масштабирования для sizeScale
    SizeSide sizeSide = {SizeSide::Width};

    // Размер блока данных отправляемого в UDP-сокет. Оптимальный размер
    // блока для передачи по локальной сети 65456 байт
    quint16 chunkStep = {65456};

    // Признак сжатия кадра
    VideoFrameCompression compression = {VideoFrameCompression::Jpeg60};

    // Порт терминала (порт назначения) для отправки UDP-пакетов данных.
    // IP-адрес терминала можно получить из входящего UDP-сообщения
    quint16 sendPort = {0};

#ifdef PPROTO_QBINARY_SERIALIZE
    DECLARE_B_SERIALIZE_FUNC
#endif

#ifdef PPROTO_JSON_SERIALIZE
    J_SERIALIZE_BEGIN
    #ifdef PPROTO_MESSAGE_NEW_JSON_FORMAT
        J_SERIALIZE_MAP_ITEM( "device_index" , deviceIndex )
        J_SERIALIZE_MAP_OPT ( "fps"          , fps         )
        J_SERIALIZE_MAP_OPT ( "frame_scale"  , frameScale  )
        J_SERIALIZE_MAP_OPT ( "size_scale"   , sizeScale   )
        J_SERIALIZE_MAP_OPT ( "size_side"    , sizeSide    )
        J_SERIALIZE_MAP_OPT ( "compression"  , compression )
    #else
      //J_SERIALIZE_ITEM( terminalId  )
        J_SERIALIZE_ITEM( deviceIndex )
        J_SERIALIZE_OPT ( fps         )
        J_SERIALIZE_OPT ( frameScale  )
        J_SERIALIZE_OPT ( sizeScale   )
        J_SERIALIZE_OPT ( sizeSide    )
        J_SERIALIZE_OPT ( compression )
    #endif
    J_SERIALIZE_END
#endif
};

struct VideoSenderStop : Data<&command::VideoSenderStop,
                               Message::Type::Command,
                               Message::Type::Answer>
{
    // Сессионный идентификатор терминала (не используется при передаче
    // видеопотока через TCP-соединение)
    quint64 terminalId = {0};

    // Идентификатор видео-устройства для которого нужно прервать передачу
    // видеопотока
    quint8 deviceIndex = {0};

#ifdef PPROTO_QBINARY_SERIALIZE
    DECLARE_B_SERIALIZE_FUNC
#endif

#ifdef PPROTO_JSON_SERIALIZE
    J_SERIALIZE_BEGIN
    #ifdef PPROTO_MESSAGE_NEW_JSON_FORMAT
      //J_SERIALIZE_MAP_ITEM( "terminal_id"  , terminalId  )
        J_SERIALIZE_MAP_ITEM( "device_index" , deviceIndex )
    #else
      //J_SERIALIZE_ITEM( terminalId  )
        J_SERIALIZE_ITEM( deviceIndex )
    #endif
    J_SERIALIZE_END
#endif
};

struct VideoStreamInfo : Data<&command::VideoStreamInfo,
                               Message::Type::Command>
{
    // Идентификатор видео-устройства
    quint8 deviceIndex = {0};

     // Номер кадра
    quint64 frameNumber = quint64(-1);

    // Тип матрицы изображения в представлении OpenCV
    qint32 cvType = {-1};

    // Реальные ширина и высота кадра
    quint16 realWidth  = {0};
    quint16 realHeight = {0};

    // Коэффициент уменьшения кадра (в процентах), 100% означает, что кадр
    // не будет масштабироваться
    quint8 scale = {100};

    // Ширина и высота кадра для запрашиваемого масштаба
    quint16 scaleWidth  = {0};
    quint16 scaleHeight = {0};

    // Запрашиваемый и реальный FPS видеопотока
    quint8 fps = {25};
    quint8 realFps = {25};

#ifdef PPROTO_QBINARY_SERIALIZE
    DECLARE_B_SERIALIZE_FUNC
#endif

#ifdef PPROTO_JSON_SERIALIZE
    J_SERIALIZE_BEGIN
    #ifdef PPROTO_MESSAGE_NEW_JSON_FORMAT
        J_SERIALIZE_MAP_ITEM( "device_index" , deviceIndex )
        J_SERIALIZE_MAP_ITEM( "frame_number" , frameNumber )
        J_SERIALIZE_MAP_ITEM( "cv_type"      , cvType      )
        J_SERIALIZE_MAP_ITEM( "real_width"   , realWidth   )
        J_SERIALIZE_MAP_ITEM( "real_height"  , realHeight  )
        J_SERIALIZE_MAP_ITEM( "scale"        , scale       )
        J_SERIALIZE_MAP_ITEM( "scale_width"  , scaleWidth  )
        J_SERIALIZE_MAP_ITEM( "scale_height" , scaleHeight )
        J_SERIALIZE_MAP_ITEM( "fps"          , fps         )
        J_SERIALIZE_MAP_ITEM( "real_fps"     , realFps     )
    #else
        J_SERIALIZE_ITEM( deviceIndex )
        J_SERIALIZE_ITEM( frameNumber )
        J_SERIALIZE_ITEM( cvType      )
        J_SERIALIZE_ITEM( realWidth   )
        J_SERIALIZE_ITEM( realHeight  )
        J_SERIALIZE_ITEM( scale       )
        J_SERIALIZE_ITEM( scaleWidth  )
        J_SERIALIZE_ITEM( scaleHeight )
        J_SERIALIZE_ITEM( fps         )
        J_SERIALIZE_ITEM( realFps     )
    #endif
    J_SERIALIZE_END
#endif
};

/**
  Возвращает расщиренную информацию по ошибке
*/
struct VideoSenderError : MessageError
{
    // Идентификатор видео-устройства
    quint8 deviceIndex = {0};

#ifdef PPROTO_QBINARY_SERIALIZE
    DECLARE_B_SERIALIZE_FUNC
#endif

#ifdef PPROTO_JSON_SERIALIZE
    J_SERIALIZE_BEGIN
    #ifdef PPROTO_MESSAGE_NEW_JSON_FORMAT
        J_SERIALIZE_BASE( MessageError )
        J_SERIALIZE_MAP_ITEM( "device_index" , deviceIndex )
    #else
        J_SERIALIZE_BASE( MessageError )
        J_SERIALIZE_ITEM( deviceIndex  )
    #endif
    J_SERIALIZE_END
#endif
};

struct FrameHeader : video::transport::FrameHeaderBase
{
    typedef container_ptr<FrameHeader> Ptr;

#ifdef PPROTO_QBINARY_SERIALIZE
    DECLARE_B_SERIALIZE_FUNC
#endif

#ifdef PPROTO_JSON_SERIALIZE
    J_SERIALIZE_BEGIN
    #ifdef PPROTO_MESSAGE_NEW_JSON_FORMAT
        J_SERIALIZE_MAP_ITEM( "device_index" , deviceIndex )
        J_SERIALIZE_MAP_ITEM( "frame_number" , frameNumber )
        J_SERIALIZE_MAP_ITEM( "cv_type"      , cvType      )
        J_SERIALIZE_MAP_ITEM( "real_width"   , realWidth   )
        J_SERIALIZE_MAP_ITEM( "real_height"  , realHeight  )
        J_SERIALIZE_MAP_ITEM( "real_step"    , realStep    )
        J_SERIALIZE_MAP_ITEM( "scale"        , scale       )
        J_SERIALIZE_MAP_ITEM( "scale_width"  , scaleWidth  )
        J_SERIALIZE_MAP_ITEM( "scale_height" , scaleHeight )
        J_SERIALIZE_MAP_ITEM( "scale_step"   , scaleStep   )
        J_SERIALIZE_MAP_ITEM( "fps"          , fps         )
        J_SERIALIZE_MAP_ITEM( "real_fps"     , realFps     )
        J_SERIALIZE_MAP_ITEM( "chunk_count"  , chunkCount  )
        J_SERIALIZE_MAP_ITEM( "chunk_step"   , chunkStep   )
        J_SERIALIZE_MAP_ITEM( "data_size"    , dataSize    )
        J_SERIALIZE_MAP_ITEM( "compress"     , compress    )
    #else
        J_SERIALIZE_ITEM( deviceIndex )
        J_SERIALIZE_ITEM( frameNumber )
        J_SERIALIZE_ITEM( cvType      )
        J_SERIALIZE_ITEM( realWidth   )
        J_SERIALIZE_ITEM( realHeight  )
        J_SERIALIZE_ITEM( realStep    )
        J_SERIALIZE_ITEM( scale       )
        J_SERIALIZE_ITEM( scaleWidth  )
        J_SERIALIZE_ITEM( scaleHeight )
        J_SERIALIZE_ITEM( scaleStep   )
        J_SERIALIZE_ITEM( fps         )
        J_SERIALIZE_ITEM( realFps     )
        J_SERIALIZE_ITEM( chunkCount  )
        J_SERIALIZE_ITEM( chunkStep   )
        J_SERIALIZE_ITEM( dataSize    )
        J_SERIALIZE_ITEM( compress    )
    #endif
    J_SERIALIZE_END
#endif
};

struct VideoSenderFrame : Data<&command::VideoSenderFrame,
                                Message::Type::Command>
{
    FrameHeader frameHeader;
    SByteArray  data;

#ifdef PPROTO_QBINARY_SERIALIZE
    DECLARE_B_SERIALIZE_FUNC
#endif

#ifdef PPROTO_JSON_SERIALIZE
    J_SERIALIZE_BEGIN
    #ifdef PPROTO_MESSAGE_NEW_JSON_FORMAT
        J_SERIALIZE_MAP_ITEM( "frame_header" , frameHeader )
        J_SERIALIZE_MAP_ITEM( "data"         , data        )
    #else
        J_SERIALIZE_ITEM( frameHeader )
        J_SERIALIZE_ITEM( data        )
    #endif
    J_SERIALIZE_END
#endif
};

} // namespace data
} // namespace pproto
