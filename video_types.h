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
