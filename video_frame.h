#pragma once

#include "video_types.h"
#include "shared/list.h"
#include "shared/clife_base.h"
#include "shared/clife_ptr.h"
#include "shared/clife_alloc.h"
#include "shared/container_ptr.h"

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
