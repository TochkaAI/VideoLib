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
