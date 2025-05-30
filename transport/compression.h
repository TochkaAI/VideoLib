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
