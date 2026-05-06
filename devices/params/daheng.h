#pragma once

#include "devices/params/dynamic.h"

#include "shared/clife_base.h"
#include "shared/clife_ptr.h"
#include "shared/container_ptr.h"
#include "shared/type_name.h"
#include "shared/prog_abort.h"
#include "shared/logger/logger.h"
#include "shared/logger/format.h"
#include "shared/qt/logger_operators.h"

#include <GxIAPI.h>
#include <utility>
#include <vector>

namespace video::device::param {

using namespace std;

/**
   Структура содержит динамические параметры для камеры Daheng
*/
struct Daheng : Dynamic
{
    typedef clife_ptr<Daheng> Ptr;

    Daheng() : Dynamic(Type::Daheng) {}

    // Режим автонастройки экспозиции
    detail::Param<GX_EXPOSURE_AUTO_ENTRY>::Ptr exposureAuto;

    // Время экспозиции (в микросекундах). Параметр учитывается
    // когда exposureAuto равно off
    detail::Param<qint64>::Ptr exposureTime;

    // Параметр включает принудительное ограничение по fps (флаг: true/false).
    // Когда параметр включен (true) текущее значение fps не будет превышать
    // величину fpsLimit
    detail::Param<GX_ACQUISITION_FRAME_RATE_MODE_ENTRY>::Ptr requireFpsLimit;

    // Числовое ограничение текущего значения fps. Параметр работает, когда
    // requireFpsLimit включен
    detail::Param<qint64>::Ptr fpsLimit;

    // Усиление
    detail::Param<double>::Ptr gain;

    // Режим автонастройки усиления
    detail::Param<GX_GAIN_AUTO_ENTRY>::Ptr gainAuto;

    // Диапазон автонастройки усиления
    detail::Param<double>::Ptr gainAutoMin;
    detail::Param<double>::Ptr gainAutoMax;

    // Режим автонастройки баланса белого
    detail::Param<GX_BALANCE_WHITE_AUTO_ENTRY>::Ptr balanceWhiteAuto;

    // Параметр включает ручную настройку насыщения цвета (флаг: on/off). Когда
    // параметр включен (on) уровень  цветового  насыщения  будет  определяться
    // значением параметра saturation
    detail::Param<GX_ENUM_SATURATION_MODE_ENTRY>::Ptr saturationMode;

    // Уровень цветового насыщения
    detail::Param<qint64>::Ptr saturation;

    // Пресет источника света
    detail::Param<GX_LIGHT_SOURCE_PRESET_ENTRY>::Ptr lightSourcePreset;

    // Текущее значение температуры камеры в градусах Цельсия
    // detail::Param<double>::Ptr currentTemperature;

    // Определяет, какой бит пользовательского выходного регистра будет установлен
    // с помощью userOutputValue
    detail::Param<GX_USER_OUTPUT_SELECTOR_ENTRY>::Ptr userOutputSelector;

    // Устанавливает значение бита, выбранного с помощью userOutputSelector.
    detail::Param<bool>::Ptr userOutputValue;

    // Физическая линия (или pin) разъема внешнего устройства для настройки
    detail::Param<GX_LINE_SELECTOR_ENTRY>::Ptr lineSelector;

    // Определяет, используется ли физическая линия для ввода или вывода сигнала
    detail::Param<GX_LINE_MODE_ENTRY>::Ptr lineMode;

    //Управляет инверсией сигнала выбранной входной или выходной линии
    detail::Param<bool>::Ptr lineInverter;

    // Определяет, какой внутренний сигнал сбора данных или источник ввода-вывода
    // будет выводиться на выбранную линию
    detail::Param<GX_LINE_SOURCE_ENTRY>::Ptr lineSource;

    enum {ReadOnly = true};

    //--- Параметры только для чтения ---

    // Текущее значение fps
    detail::Param<double, ReadOnly>::Ptr currentFps;

    // Текущее значение глубины пикселя. Может принимать значения 8, 16
    detail::Param<qint8, ReadOnly>::Ptr pixelDepth;

    // Текущий размер GEV пакета, синоним MTU (только для GigE камер)
    detail::Param<qint64, ReadOnly>::Ptr packetSize;

    // Текущая межпакетная задержка GEV (GevSCPD, только для GigE камер)
    detail::Param<qint64, ReadOnly>::Ptr packetDelay;

    // Оценочная пропускная способность потока от камеры (только для GigE камер)
    detail::Param<qint64, ReadOnly>::Ptr estimatedBandwidth;

    bool load(int deviceIndex, const yaml::Config&) override;
    bool save(int deviceIndex, yaml::Config&, std::string = {}) override;

    Dynamic::Ptr clone() const override;

    template<typename T> using EnumStrMap = vector<pair<T, const char*>>;

    static EnumStrMap<GX_EXPOSURE_AUTO_ENTRY>                GX_EXPOSURE_AUTO_ENTRY_MAP;
    static EnumStrMap<GX_ACQUISITION_FRAME_RATE_MODE_ENTRY>  GX_ACQUISITION_FRAME_RATE_MODE_ENTRY_MAP;
    static EnumStrMap<GX_GAIN_AUTO_ENTRY>                    GX_GAIN_AUTO_ENTRY_MAP;
    static EnumStrMap<GX_BALANCE_WHITE_AUTO_ENTRY>           GX_BALANCE_WHITE_AUTO_ENTRY_MAP;
    static EnumStrMap<GX_ENUM_SATURATION_MODE_ENTRY>         GX_ENUM_SATURATION_MODE_ENTRY_MAP;
    static EnumStrMap<GX_LIGHT_SOURCE_PRESET_ENTRY>          GX_LIGHT_SOURCE_PRESET_ENTRY_MAP;
    static EnumStrMap<GX_USER_OUTPUT_SELECTOR_ENTRY>         GX_USER_OUTPUT_SELECTOR_ENTRY_MAP;
    static EnumStrMap<GX_LINE_SELECTOR_ENTRY>                GX_LINE_SELECTOR_ENTRY_MAP;
    static EnumStrMap<GX_LINE_MODE_ENTRY>                    GX_LINE_MODE_ENTRY_MAP;
    static EnumStrMap<GX_LINE_SOURCE_ENTRY>                  GX_LINE_SOURCE_ENTRY_MAP;

    template<typename EnumType>
    static const char* enumToString(EnumType value);

    template<typename EnumType>
    static EnumType stringToEnum(const string& value, EnumType defaultVal);

#ifdef PPROTO_QBINARY_SERIALIZE
    DECLARE_B_SERIALIZE_FUNC
#endif

#ifdef PPROTO_JSON_SERIALIZE
    J_SERIALIZE_BEGIN
        J_SERIALIZE_MAP_OPT( "exposure_auto"        , exposureAuto       )
        J_SERIALIZE_MAP_OPT( "exposure_time"        , exposureTime       )
        J_SERIALIZE_MAP_OPT( "require_fps_limit"    , requireFpsLimit    )
        J_SERIALIZE_MAP_OPT( "fps_limit"            , fpsLimit           )
        J_SERIALIZE_MAP_OPT( "gain"                 , gain               )
        J_SERIALIZE_MAP_OPT( "gain_auto"            , gainAuto           )
        J_SERIALIZE_MAP_OPT( "gain_auto_min"        , gainAutoMin        )
        J_SERIALIZE_MAP_OPT( "gain_auto_max"        , gainAutoMax        )
        J_SERIALIZE_MAP_OPT( "balance_white_auto"   , balanceWhiteAuto   )
        J_SERIALIZE_MAP_OPT( "saturation_mode"      , saturationMode     )
        J_SERIALIZE_MAP_OPT( "saturation"           , saturation         )
        J_SERIALIZE_MAP_OPT( "light_source_preset"  , lightSourcePreset  )
        J_SERIALIZE_MAP_OPT( "user_output_selector" , userOutputSelector )
        J_SERIALIZE_MAP_OPT( "user_output_value"    , userOutputValue    )
        J_SERIALIZE_MAP_OPT( "line_selector"        , lineSelector       )
        J_SERIALIZE_MAP_OPT( "line_mode"            , lineMode           )
        J_SERIALIZE_MAP_OPT( "line_inverter"        , lineInverter       )
        J_SERIALIZE_MAP_OPT( "line_source"          , lineSource         )
        J_SERIALIZE_MAP_OPT( "current_fps"          , currentFps         )
        J_SERIALIZE_MAP_OPT( "pixel_depth"          , pixelDepth         )
        J_SERIALIZE_MAP_OPT( "packet_size"          , packetSize         )
        J_SERIALIZE_MAP_OPT( "packet_delay"         , packetDelay        )
        J_SERIALIZE_MAP_OPT( "estimated_bandwidth"  , estimatedBandwidth )
    J_SERIALIZE_END
#endif
};

template<typename EnumType>
const char* Daheng::enumToString(EnumType value)
{
    (void) value;

    #define ENUM_TO_STRING(GX_ENUM) \
        if constexpr(std::is_same<GX_ENUM, EnumType>::value) { \
            for (const auto& p : GX_ENUM##_MAP) \
                if (p.first == value) return p.second; \
        }

    // if constexpr(std::is_same<GX_EXPOSURE_AUTO_ENTRY, EnumType>::value)
    // {
    //     for (const auto& p : GX_EXPOSURE_AUTO_ENTRY_MAP)
    //         if (p.first == val) return p.second;
    // }

    ENUM_TO_STRING(GX_EXPOSURE_AUTO_ENTRY)
    ENUM_TO_STRING(GX_ACQUISITION_FRAME_RATE_MODE_ENTRY)
    ENUM_TO_STRING(GX_GAIN_AUTO_ENTRY)
    ENUM_TO_STRING(GX_BALANCE_WHITE_AUTO_ENTRY)
    ENUM_TO_STRING(GX_ENUM_SATURATION_MODE_ENTRY)
    ENUM_TO_STRING(GX_LIGHT_SOURCE_PRESET_ENTRY)
    ENUM_TO_STRING(GX_USER_OUTPUT_SELECTOR_ENTRY)
    ENUM_TO_STRING(GX_LINE_SELECTOR_ENTRY)
    ENUM_TO_STRING(GX_LINE_MODE_ENTRY)
    ENUM_TO_STRING(GX_LINE_SOURCE_ENTRY)

    #undef ENUM_TO_STRING

    alog::logger().error(alog_line_location, "DevParam") << log_format(
        "String equivalent not found for enum value %?. See enum type %?",
        static_cast<int>(value), abi_type_name<EnumType>());

    prog_abort();
    return "";
}

template<typename EnumType>
EnumType Daheng::stringToEnum(const string& value, EnumType defaultVal)
{
    (void) value;

    #define STRING_TO_ENUM(GX_ENUM) \
        if constexpr(std::is_same<GX_ENUM, EnumType>::value) { \
            for (const auto& p : GX_ENUM##_MAP) \
                if (p.second == value) return p.first; \
        }

    // if constexpr(std::is_same<GX_EXPOSURE_AUTO_ENTRY, EnumType>::value)
    // {
    //     for (const auto& p : GX_EXPOSURE_AUTO_ENTRY_MAP)
    //         if (p.second == val) return p.first;
    // }

    STRING_TO_ENUM(GX_EXPOSURE_AUTO_ENTRY)
    STRING_TO_ENUM(GX_ACQUISITION_FRAME_RATE_MODE_ENTRY)
    STRING_TO_ENUM(GX_GAIN_AUTO_ENTRY)
    STRING_TO_ENUM(GX_BALANCE_WHITE_AUTO_ENTRY)
    STRING_TO_ENUM(GX_ENUM_SATURATION_MODE_ENTRY)
    STRING_TO_ENUM(GX_LIGHT_SOURCE_PRESET_ENTRY)
    STRING_TO_ENUM(GX_USER_OUTPUT_SELECTOR_ENTRY)
    STRING_TO_ENUM(GX_LINE_SELECTOR_ENTRY)
    STRING_TO_ENUM(GX_LINE_MODE_ENTRY)
    STRING_TO_ENUM(GX_LINE_SOURCE_ENTRY)

    #undef STRING_TO_ENUM

    const char* defaultStr = enumToString(defaultVal);
    alog::logger().error(alog_line_location, "DevParam") << log_format(
        "No matching enum value for string '%?'"
        ". Will be used string '%?' corresponding to default enum value %?"
        ". See enum type %?",
        value, defaultStr, static_cast<int>(defaultVal), abi_type_name<EnumType>());

    return defaultVal;
}

} // namespace video::device::param
