#pragma once

#include "shared/logger/logger.h"

namespace video::device {

/**
  Тип устройства с которого будет производиться захват видеопотока
*/
enum class Type
{
    Unknown   = 0,
    WebCamera = 1, // Захват потока с web-камеры
    Stream    = 2, // Захват потока с ip-камеры
    File      = 3, // Получение потока из файла
    Basler    = 4, // Захват потока с камеры Basler
    DeckLink  = 5, // Захват потока c оборудования DeckLink
    Daheng    = 6  // Захват потока c камеры Daheng
};

inline const char* toString(Type t)
{
    switch (t)
    {
        case Type::Unknown   : return "Unknown"   ;
        case Type::WebCamera : return "WebCamera" ;
        case Type::Stream    : return "Stream"    ;
        case Type::File      : return "File"      ;
        case Type::Basler    : return "Basler"    ;
        case Type::DeckLink  : return "DeckLink"  ;
        case Type::Daheng    : return "Daheng"    ;
    }
    return "";
}

} // namespace video::device

namespace alog {

inline Line& operator<< (Line& line, video::device::Type t)
{
    if (line.toLogger())
        line << video::device::toString(t);
    return line;
}

} // namespace alog
