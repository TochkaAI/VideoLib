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

#include "shared/clife_base.h"
#include "shared/clife_ptr.h"

#include <QtGlobal>
#include <vector>

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

/**
  Координаты камеры
*/
struct Coordinates : clife_base
{
    typedef clife_ptr<Coordinates> Ptr;

    // Глобальные координаты TCP кобота. Формат: x, y, z, rx, ry, rz
    std::vector<double> globalPose;

    // Признак наличия локальных координат для TCP кобота
    bool localExists = {false};

    // Локальные координаты TCP кобота в пользовательской системе координат.
    // Формат: x, y, z, rx, ry, rz
    std::vector<double> localPose;

    // Время получения координат (UTC)
    timespec time = {0, 0};
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
