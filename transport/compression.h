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

#include <QtCore>

namespace pproto {

enum VideoFrameCompression : quint32
{
    None    =  0, // без сжатия, структура данных изображения cv::Mat;
    Zip     =  1, // zip-сжатие, структура данных изображения cv::Mat,
                  // по уровню сжатия соответствует jpeg 95%;
    Jpeg50  =  5, // jpeg-сжатие 50%;
    Jpeg60  =  6, // jpeg-сжатие 60%;
    Jpeg70  =  7, // jpeg-сжатие 70%;
    Jpeg80  =  8, // jpeg-сжатие 80%;
    Jpeg90  =  9, // jpeg-сжатие 90%;
    Jpeg100 = 10, // без сжатия, структура данных изображения jpeg;
};

inline const char* toString(VideoFrameCompression compression)
{
    switch (compression)
    {
        case VideoFrameCompression::None    : return "None";
        case VideoFrameCompression::Zip     : return "Zip";
        case VideoFrameCompression::Jpeg50  : return "Jpeg50";
        case VideoFrameCompression::Jpeg60  : return "Jpeg60";
        case VideoFrameCompression::Jpeg70  : return "Jpeg70";
        case VideoFrameCompression::Jpeg80  : return "Jpeg80";
        case VideoFrameCompression::Jpeg90  : return "Jpeg90";
        case VideoFrameCompression::Jpeg100 : return "Jpeg100";
    }
    return "";
}

} // namespace pproto
