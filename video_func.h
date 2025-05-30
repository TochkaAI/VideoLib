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
