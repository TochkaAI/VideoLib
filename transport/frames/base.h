#pragma once

#include "shared/list.h"
#include "shared/clife_base.h"
#include "shared/clife_ptr.h"
#include "shared/clife_alloc.h"
#include "shared/container_ptr.h"
#include <QtCore>

namespace video::transport {

/**
  Заголовок передаваемого видеофрейма
  Код фрейма: 0
*/
struct FrameHeaderBase
{
    quint8  deviceIndex;   // Идентификатор устройства
    quint64 frameNumber;   // Номер кадра

    qint32  cvType;        // Тип матрицы изображения в представлении OpenCV

    quint16 realWidth;     // Реальная ширина кадра
    quint16 realHeight;    // Реальная высота кадра
    quint16 realStep;      // Размер одной записи (one-row) изображения

    quint8  scale = {100}; // Коэффициент  уменьшения  кадра  (в процентах),
                           // 100% означает, что кадр не будет масштабироваться
    quint16 scaleWidth;    // Ширина кадра для запрашиваемого масштаба
    quint16 scaleHeight;   // Высота кадра для запрашиваемого масштаба
    quint16 scaleStep;     // Размер одной записи (one-row) изображения

    quint8  fps = {25};    // Запрашиваемый FPS видеопотока
    quint8  realFps = {25};// Реальный FPS видеопотока

    quint16 chunkCount;    // Количество кусков на которые разбит кадр
    quint16 chunkStep;     // Шаг (размер) одного куска
    quint32 dataSize;      // Полный размер передаваемого кадра. Если исполь-
                           // зуется сжатие, то размер сжатого кадра

    quint8  compress = {0};// Признак сжатия кадра (см. описание VideoFrameCompression)
                           //   0 - без сжатия (формат cv::Mat);
                           //   1 - zip-сжатие (формат cv::Mat);
                           //   5 - jpeg 50%;
                           //   6 - jpeg 60%;
                           //   7 - jpeg 70%;
                           //   8 - jpeg 80%;
                           //   9 - jpeg 90%;
                           //  10 - jpeg 100%  (без сжатия, формат jpeg);
};

} // namespace video::transport
