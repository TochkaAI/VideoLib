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

#include "video_types.h"
#include "shared/list.h"
#include "shared/clife_base.h"
#include "shared/clife_ptr.h"
#include "shared/clife_alloc.h"

#include <opencv2/opencv.hpp>
#include <QtCore>
#include <ctime>

namespace video {

using namespace cv;

struct Frame : clife_base
{
    typedef clife_ptr<Frame> Ptr;
    typedef lst::List<Frame, lst::CompareItemDummy, clife_alloc<Frame>> List;

    int deviceIndex = {-1};   // Индекс устройства с которого получен кадр

    timespec time   = {0, 0}; // Время получения кадра (UTC)
    double   width  = {0};    // Ширина кадра
    double   height = {0};    // Высота кадра
    double   fps    = {0};    // FPS видеопотока
    quint64  number = quint64(-1); // Номер кадра

    Coordinates::Ptr coordinates;

    Source source = {Source::Unknown};
    CaptureMode captureMode = {CaptureMode::BGR};

    // Глубина пикселя. Может принимать значения 8, 16
    int pixelDepth = {8};

    Mat data;      // Кадр в формате CaptureMode
    Mat dataGray;  // Кадр в формате CV_8UC1 (GRAY)

    // Абстрактная структура, используется для передачи произвольных данных
    struct Something : clife_base
    {
        typedef clife_ptr<Something> Ptr;
    };
    Something::Ptr something;
};

} // namespace video
