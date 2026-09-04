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
#include "shared/qt/logger_operators.h"

#include <opencv2/opencv.hpp>
#include <QtCore>
#include <functional>

namespace video {

using namespace cv;
using namespace std;

template<typename T>
void checkTooManyFrames(QList<T>& frames, std::function<bool(int)> markerNewStream,
                        const char* framesName, const char* moduleName)
{
    bool tooManyFrames = false;
    if (frames.count() > 30)
    {
        tooManyFrames = true;
        alog::logger().warn(alog_line_location, moduleName)
            << "Too many " << framesName << " in the inbound queue ("
            <<  frames.count()
            << "). The odd " << framesName << " will be skipped";
        for (int i = 0; i < frames.count(); ++i)
            if ((i % 2 == 1) && !markerNewStream(i))
                frames[i].reset();
    }
    for (int i = 0; i < frames.count(); ++i)
        if (frames.at(i).empty())
            frames.removeAt(i--);

    if (tooManyFrames)
        alog::logger().warn(alog_line_location, moduleName)
            << "New amount " << framesName << " in the inbound queue ("
            <<  frames.count() << ")";
}

} // namespace video
