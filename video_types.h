#pragma once

#include <QtGlobal>

namespace video {

enum class Source : quint32
{
    Unknown   = 0,
    WebCamera = 1, // web-камера
    Stream    = 2, // ip-камера
    File      = 3, // файловый поток
    Basler    = 4, // камера Basler
    DeckLink  = 5, // оборудование DeckLink
    Daheng    = 6  // камера Daheng
};

enum class CaptureMode : quint32
{
    BGR  = 0,
    RGB  = 1,
    YUYV = 2,
    GRAY = 3
};

inline const char* toString(CaptureMode mode)
{
    switch (mode)
    {
        case CaptureMode::BGR  : return "BGR";
        case CaptureMode::RGB  : return "RGB";
        case CaptureMode::YUYV : return "YUYV";
        case CaptureMode::GRAY : return "GRAY";
    }
    return "";
}

} // namespace video
