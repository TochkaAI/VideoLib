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

// Координаты камеры
struct Coordinates : clife_base
{
    typedef clife_ptr<Coordinates> Ptr;

    double x  = {0}; // Координата по оси  X (в метрах)
    double y  = {0}; // Координата по оси  Y (в метрах)
    double z  = {0}; // Координата по оси  Z (в метрах)
    double rx = {0}; // Поворот вокруг оси X (в радианах)
    double ry = {0}; // Поворот вокруг оси Y (в радианах)
    double rz = {0}; // Поворот вокруг оси Z (в радианах)

    timespec time = {0, 0}; // Время получения координат (UTC)
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
