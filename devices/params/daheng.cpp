#include "devices/params/daheng.h"

#include "shared/utils.h"

#define log_error_m   alog::logger().error   (alog_line_location, "DevParam")
#define log_warn_m    alog::logger().warn    (alog_line_location, "DevParam")
#define log_info_m    alog::logger().info    (alog_line_location, "DevParam")
#define log_verbose_m alog::logger().verbose (alog_line_location, "DevParam")
#define log_debug_m   alog::logger().debug   (alog_line_location, "DevParam")
#define log_debug2_m  alog::logger().debug2  (alog_line_location, "DevParam")

namespace video::device::param {

Daheng::EnumStrMap<GX_EXPOSURE_AUTO_ENTRY> Daheng::GX_EXPOSURE_AUTO_ENTRY_MAP =
{
    {GX_EXPOSURE_AUTO_OFF        , "off"       },
    {GX_EXPOSURE_AUTO_CONTINUOUS , "continuous"},
    {GX_EXPOSURE_AUTO_ONCE       , "once"      },
};

Daheng::EnumStrMap<GX_ACQUISITION_FRAME_RATE_MODE_ENTRY> Daheng::GX_ACQUISITION_FRAME_RATE_MODE_ENTRY_MAP =
{
    {GX_ACQUISITION_FRAME_RATE_MODE_OFF , "off"},
    {GX_ACQUISITION_FRAME_RATE_MODE_ON  , "on" },
};

Daheng::EnumStrMap<GX_GAIN_AUTO_ENTRY> Daheng::GX_GAIN_AUTO_ENTRY_MAP =
{
    {GX_GAIN_AUTO_OFF        , "off"       },
    {GX_GAIN_AUTO_CONTINUOUS , "continuous"},
    {GX_GAIN_AUTO_ONCE       , "once"      },
};

Daheng::EnumStrMap<GX_BALANCE_WHITE_AUTO_ENTRY> Daheng::GX_BALANCE_WHITE_AUTO_ENTRY_MAP =
{
    {GX_BALANCE_WHITE_AUTO_OFF        , "off"       },
    {GX_BALANCE_WHITE_AUTO_CONTINUOUS , "continuous"},
    {GX_BALANCE_WHITE_AUTO_ONCE       , "once"      },
};

Daheng::EnumStrMap<GX_ENUM_SATURATION_MODE_ENTRY> Daheng::GX_ENUM_SATURATION_MODE_ENTRY_MAP =
{
    {GX_ENUM_SATURATION_OFF , "off"},
    {GX_ENUM_SATURATION_ON  , "on"},
};

Daheng::EnumStrMap<GX_LIGHT_SOURCE_PRESET_ENTRY> Daheng::GX_LIGHT_SOURCE_PRESET_ENTRY_MAP =
{
    {GX_LIGHT_SOURCE_PRESET_OFF                     , "off"                    },
    {GX_LIGHT_SOURCE_PRESET_CUSTOM                  , "custom"                 },
    {GX_LIGHT_SOURCE_PRESET_DAYLIGHT_6500K          , "daylight_6500K"         },
    {GX_LIGHT_SOURCE_PRESET_DAYLIGHT_5000K          , "daylight_5000K"         },
    {GX_LIGHT_SOURCE_PRESET_COOL_WHITE_FLUORESCENCE , "cool_white_fluorescence"},
    {GX_LIGHT_SOURCE_PRESET_INCA                    , "inca"                   },
};

Daheng::EnumStrMap<GX_USER_OUTPUT_SELECTOR_ENTRY> Daheng::GX_USER_OUTPUT_SELECTOR_ENTRY_MAP =
{
    {GX_USER_OUTPUT_SELECTOR_OUTPUT0 , "output0"},
    {GX_USER_OUTPUT_SELECTOR_OUTPUT1 , "output1"},
    {GX_USER_OUTPUT_SELECTOR_OUTPUT2 , "output2"},
    {GX_USER_OUTPUT_SELECTOR_OUTPUT3 , "output3"},
    {GX_USER_OUTPUT_SELECTOR_OUTPUT4 , "output4"},
    {GX_USER_OUTPUT_SELECTOR_OUTPUT5 , "output5"},
    {GX_USER_OUTPUT_SELECTOR_OUTPUT6 , "output6"},
};

Daheng::EnumStrMap<GX_LINE_SELECTOR_ENTRY> Daheng::GX_LINE_SELECTOR_ENTRY_MAP =
{
    {GX_ENUM_LINE_SELECTOR_LINE0       , "line0"      },
    {GX_ENUM_LINE_SELECTOR_LINE1       , "line1"      },
    {GX_ENUM_LINE_SELECTOR_LINE2       , "line2"      },
    {GX_ENUM_LINE_SELECTOR_LINE3       , "line3"      },
    {GX_ENUM_LINE_SELECTOR_LINE4       , "line4"      },
    {GX_ENUM_LINE_SELECTOR_LINE5       , "line5"      },
    {GX_ENUM_LINE_SELECTOR_LINE6       , "line6"      },
    {GX_ENUM_LINE_SELECTOR_LINE7       , "line7"      },
    {GX_ENUM_LINE_SELECTOR_LINE8       , "line8"      },
    {GX_ENUM_LINE_SELECTOR_LINE9       , "line9"      },
    {GX_ENUM_LINE_SELECTOR_LINE10      , "line10"     },
    {GX_ENUM_LINE_SELECTOR_LINE_STROBE , "line_strobe"},
    {GX_ENUM_LINE_SELECTOR_LINE11      , "line11"     },
    {GX_ENUM_LINE_SELECTOR_LINE12      , "line12"     },
    {GX_ENUM_LINE_SELECTOR_LINE13      , "line13"     },
    {GX_ENUM_LINE_SELECTOR_LINE14      , "line14"     },
    {GX_ENUM_LINE_SELECTOR_TRIGGER     , "trigger"    },
    {GX_ENUM_LINE_SELECTOR_IO1         , "io1"        },
    {GX_ENUM_LINE_SELECTOR_IO2         , "io2"        },
    {GX_ENUM_LINE_SELECTOR_FLASH_P     , "flash_p"    },
    {GX_ENUM_LINE_SELECTOR_FLASH_W     , "flash_w"    },
};

Daheng::EnumStrMap<GX_LINE_MODE_ENTRY> Daheng::GX_LINE_MODE_ENTRY_MAP =
{
    {GX_ENUM_LINE_MODE_INPUT  , "input" },
    {GX_ENUM_LINE_MODE_OUTPUT , "output"},
};

Daheng::EnumStrMap<GX_LINE_SOURCE_ENTRY> Daheng::GX_LINE_SOURCE_ENTRY_MAP =
{
    {GX_ENUM_LINE_SOURCE_OFF                      , "off"                     },
    {GX_ENUM_LINE_SOURCE_STROBE                   , "strobe"                  },
    {GX_ENUM_LINE_SOURCE_USEROUTPUT0              , "useroutput0"             },
    {GX_ENUM_LINE_SOURCE_USEROUTPUT1              , "useroutput1"             },
    {GX_ENUM_LINE_SOURCE_USEROUTPUT2              , "useroutput2"             },
    {GX_ENUM_LINE_SOURCE_EXPOSURE_ACTIVE          , "exposure_active"         },
    {GX_ENUM_LINE_SOURCE_FRAME_TRIGGER_WAIT       , "frame_trigger_wait"      },
    {GX_ENUM_LINE_SOURCE_ACQUISITION_TRIGGER_WAIT , "acquisition_trigger_wait"},
    {GX_ENUM_LINE_SOURCE_TIMER1_ACTIVE            , "timer1_active"           },
    {GX_ENUM_LINE_SOURCE_USEROUTPUT3              , "useroutput3"             },
    {GX_ENUM_LINE_SOURCE_USEROUTPUT4              , "useroutput4"             },
    {GX_ENUM_LINE_SOURCE_USEROUTPUT5              , "useroutput5"             },
    {GX_ENUM_LINE_SOURCE_USEROUTPUT6              , "useroutput6"             },
    {GX_ENUM_LINE_SOURCE_TIMER2_ACTIVE            , "timer2_active"           },
    {GX_ENUM_LINE_SOURCE_TIMER3_ACTIVE            , "timer3_active"           },
    {GX_ENUM_LINE_SOURCE_FRAME_TRIGGER            , "frame_trigger"           },
    {GX_ENUM_LINE_SOURCE_Flash_W                  , "flash_w"                 },
    {GX_ENUM_LINE_SOURCE_Flash_P                  , "flash_p"                 },
    {GX_ENUM_LINE_SOURCE_SERIAL_PORT_0            , "serial_port_0"           },
};

bool Daheng::load(int deviceIndex, const yaml::Config& config)
{
    string fullParamName;
    string paramSection = "device" + utl::toString(deviceIndex) + ".daheng.";

    auto getParam = [&paramSection, &config, &fullParamName]
                    (const char* paramName, auto& param) -> bool
    {
        // Читаем параметр из целевой секции устройства
        fullParamName = paramSection + paramName;
        if (config.getValue(fullParamName, param, false))
            return true;

        // Если параметра нет в целевой секции - пробуем прочитать общую
        // секцию для всех устройств
        fullParamName = "device." + string(paramName);
        if (config.getValue(fullParamName, param, false))
            return true;

        // Если параметр не найден - вновь читаем целевую секцию для того
        // чтобы вывести сообщение в лог
        // fullParamName = paramSection + paramName;
        // typename remove_reference<decltype(param)>::type dummy; (void) dummy;
        // config.getValue(fullParamName, dummy, true /*пишем в лог*/);

        return false;
    };

    #define LOG_LOAD_PARAM(PARAM) \
        log_verbose_m << log_format("Load dynamic param %?: %?", \
                                    fullParamName, PARAM);
    string exposureAuto;
    if (getParam("exposure_auto", exposureAuto))
    {
        initParam(this->exposureAuto);
        LOG_LOAD_PARAM(exposureAuto)
        this->exposureAuto->value = stringToEnum(exposureAuto, GX_EXPOSURE_AUTO_OFF);
    }

    int64_t exposureTime;
    if (getParam("exposure_time", exposureTime))
    {
        initParam(this->exposureTime);
        LOG_LOAD_PARAM(exposureTime)
        this->exposureTime->value = exposureTime;
    }

    string requireFpsLimit;
    if (getParam("require_fps_limit", requireFpsLimit))
    {
        if (requireFpsLimit == "true")
            requireFpsLimit = "on";

        initParam(this->requireFpsLimit);
        LOG_LOAD_PARAM(requireFpsLimit)
        this->requireFpsLimit->value = stringToEnum(requireFpsLimit, GX_ACQUISITION_FRAME_RATE_MODE_OFF);
    }

    int64_t fpsLimit;
    if (getParam("fps_limit", fpsLimit))
    {
        initParam(this->fpsLimit);
        LOG_LOAD_PARAM(fpsLimit)
        this->fpsLimit->value = fpsLimit;
    }

    double gain;
    if (getParam("gain", gain))
    {
        initParam(this->gain);
        LOG_LOAD_PARAM(gain)
        this->gain->value = gain;
    }

    string gainAuto;
    if (getParam("gain_auto", gainAuto))
    {
        initParam(this->gainAuto);
        LOG_LOAD_PARAM(gainAuto)
        this->gainAuto->value = stringToEnum(gainAuto, GX_GAIN_AUTO_OFF);
    }

    double gainAutoMin;
    if (getParam("gain_auto_min", gainAutoMin))
    {
        initParam(this->gainAutoMin);
        LOG_LOAD_PARAM(gainAutoMin)
        this->gainAutoMin->value = gainAutoMin;
    }

    double gainAutoMax;
    if (getParam("gain_auto_max", gainAutoMax))
    {
        initParam(this->gainAutoMax);
        LOG_LOAD_PARAM(gainAutoMax)
        this->gainAutoMax->value = gainAutoMax;
    }

    string balanceWhiteAuto;
    if (getParam("balance_white_auto", balanceWhiteAuto))
    {
        initParam(this->balanceWhiteAuto);
        LOG_LOAD_PARAM(balanceWhiteAuto)
        this->balanceWhiteAuto->value = stringToEnum(balanceWhiteAuto, GX_BALANCE_WHITE_AUTO_OFF);
    }

    string saturationMode;
    if (getParam("saturation_mode", saturationMode))
    {
        if (saturationMode == "true")
            saturationMode = "on";

        initParam(this->saturationMode);
        LOG_LOAD_PARAM(saturationMode)
        this->saturationMode->value = stringToEnum(saturationMode, GX_ENUM_SATURATION_OFF);
    }

    int64_t saturation;
    if (getParam("saturation", saturation))
    {
        initParam(this->saturation);
        LOG_LOAD_PARAM(saturation)
        this->saturation->value = saturation;
    }

    string lightSourcePreset;
    if (getParam("light_source_preset", lightSourcePreset))
    {
        initParam(this->lightSourcePreset);
        LOG_LOAD_PARAM(lightSourcePreset)
        this->lightSourcePreset->value = stringToEnum(lightSourcePreset, GX_LIGHT_SOURCE_PRESET_OFF);
    }    

    string userOutputSelector;
    if (getParam("user_output_selector", userOutputSelector))
    {
        initParam(this->userOutputSelector);
        LOG_LOAD_PARAM(userOutputSelector)
        this->userOutputSelector->value = stringToEnum(userOutputSelector, GX_USER_OUTPUT_SELECTOR_OUTPUT0);
    }

    bool userOutputValue = false;
    if (getParam("user_output_value", userOutputValue))
    {
        initParam(this->userOutputValue);
        LOG_LOAD_PARAM(userOutputValue)
        this->userOutputValue->value = userOutputValue;
    }

    string lineSelector;
    if (getParam("line_selector", lineSelector))
    {
        initParam(this->lineSelector);
        LOG_LOAD_PARAM(lineSelector)
        this->lineSelector->value = stringToEnum(lineSelector, GX_ENUM_LINE_SELECTOR_LINE0);
    }

    string lineMode;
    if (getParam("line_mode", lineMode))
    {
        initParam(this->lineMode);
        LOG_LOAD_PARAM(lineMode)
        this->lineMode->value = stringToEnum(lineMode, GX_ENUM_LINE_MODE_INPUT);
    }

    bool lineInverter = false;
    if (getParam("line_inverter", lineInverter))
    {
        initParam(this->lineInverter);
        LOG_LOAD_PARAM(lineInverter)
        this->lineInverter->value = lineInverter;
    }

    string lineSource;
    if (getParam("line_source", lineSource))
    {
        initParam(this->lineSource);
        LOG_LOAD_PARAM(lineSource)
        this->lineSource->value = stringToEnum(lineSource, GX_ENUM_LINE_SOURCE_OFF);
    }

    #undef LOG_LOAD_PARAM

    return true;
}

bool Daheng::save(int deviceIndex, yaml::Config& config, string section)
{
    string paramSection = "device" + utl::toString(deviceIndex) + ".daheng.";

    if (!section.empty())
        paramSection = section;

    auto setParam = [&paramSection, &config]
                    (const char* paramName, auto& param) -> bool
    {
        string fullParamName = paramSection + paramName;
        return config.setValue(fullParamName, param);
    };

    if (exposureAuto)
    {
        string value = enumToString(exposureAuto->value);
        setParam("exposure_auto", value);
    }

    if (exposureTime)
    {
        int64_t value = exposureTime->value;
        setParam("exposure_time", value);
    }

    if (requireFpsLimit)
    {
        string value = enumToString(requireFpsLimit->value);
        setParam("require_fps_limit", value);
    }

    if (fpsLimit)
    {
        int64_t value = fpsLimit->value;
        setParam("fps_limit", value);
    }

    if (gain)
    {
        double value = gain->value;
        setParam("gain", value);
    }

    if (gainAuto)
    {
        string value = enumToString(gainAuto->value);
        setParam("gain_auto", value);
    }

    if (gainAutoMin)
    {
        double value = gainAutoMin->value;
        setParam("gain_auto_min", value);
    }

    if (gainAutoMax)
    {
        double value = gainAutoMax->value;
        setParam("gain_auto_max", value);
    }

    if (balanceWhiteAuto)
    {
        string value = enumToString(balanceWhiteAuto->value);
        setParam("balance_white_auto", value);
    }

    if (saturationMode)
    {
        string value = enumToString(saturationMode->value);
        setParam("saturation_mode", value);
    }

    if (saturation)
    {
        int64_t value = saturation->value;
        setParam("saturation", value);
    }

    if (lightSourcePreset)
    {
        string value = enumToString(lightSourcePreset->value);
        setParam("light_source_preset", value);
    }

    if (userOutputSelector)
    {
        string value = enumToString(userOutputSelector->value);
        setParam("user_output_selector", value);
    }

    if (userOutputValue)
    {
        bool value = userOutputValue->value;
        setParam("user_output_value", value);
    }

    if (lineSelector)
    {
        string value = enumToString(lineSelector->value);
        setParam("line_selector", value);
    }

    if (lineMode)
    {
        string value = enumToString(lineMode->value);
        setParam("line_mode", value);
    }

    if (lineInverter)
    {
        bool value = lineInverter->value;
        setParam("line_inverter", value);
    }

    if (lineSource)
    {
        string value = enumToString(lineSource->value);
        setParam("line_source", value);
    }

    return config.saveFile();
}

Dynamic::Ptr Daheng::clone() const
{
    Daheng::Ptr p {new Daheng};

    if (exposureAuto)       p->exposureAuto       = exposureAuto->clone();
    if (exposureTime)       p->exposureTime       = exposureTime->clone();
    if (requireFpsLimit)    p->requireFpsLimit    = requireFpsLimit->clone();
    if (fpsLimit)           p->fpsLimit           = fpsLimit->clone();
    if (gain)               p->gain               = gain->clone();
    if (gainAuto)           p->gainAuto           = gainAuto->clone();
    if (gainAutoMin)        p->gainAutoMin        = gainAutoMin->clone();
    if (gainAutoMax)        p->gainAutoMax        = gainAutoMax->clone();
    if (balanceWhiteAuto)   p->balanceWhiteAuto   = balanceWhiteAuto->clone();
    if (saturationMode)     p->saturationMode     = saturationMode->clone();
    if (saturation)         p->saturation         = saturation->clone();
    if (lightSourcePreset)  p->lightSourcePreset  = lightSourcePreset->clone();
    if (userOutputSelector) p->userOutputSelector = userOutputSelector->clone();
    if (userOutputValue)    p->userOutputValue    = userOutputValue->clone();
    if (lineSelector)       p->lineSelector       = lineSelector->clone();
    if (lineMode)           p->lineMode           = lineMode->clone();
    if (lineInverter)       p->lineInverter       = lineInverter->clone();
    if (lineSource)         p->lineSource         = lineSource->clone();

    p->currentFps = currentFps;
    p->pixelDepth = pixelDepth;
    return p;
}

#ifdef PPROTO_QBINARY_SERIALIZE
void Daheng::toRaw(bserial::DataStream& stream) const
{
    B_SERIALIZE_V1
    bserial::putToStream(stream, mode); // stream <- mode;
    stream << exposureAuto;
    stream << exposureTime;
    stream << requireFpsLimit;
    stream << fpsLimit;
    stream << gain;
    stream << gainAuto;
    stream << gainAutoMin;
    stream << gainAutoMax;
    stream << balanceWhiteAuto;
    stream << saturationMode;
    stream << saturation;
    stream << lightSourcePreset;
    stream << currentFps;
    stream << pixelDepth;
    stream << userOutputSelector;
    stream << userOutputValue;
    stream << lineSelector;
    stream << lineMode;
    stream << lineInverter;
    stream << lineSource;
    B_SERIALIZE_RETURN
}

void Daheng::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect)
    bserial::getFromStream(stream, mode); // stream -> mode;
    stream >> exposureAuto;
    stream >> exposureTime;
    stream >> requireFpsLimit;
    stream >> fpsLimit;
    stream >> gain;
    stream >> gainAuto;
    stream >> gainAutoMin;
    stream >> gainAutoMax;
    stream >> balanceWhiteAuto;
    stream >> saturationMode;
    stream >> saturation;
    stream >> lightSourcePreset;
    stream >> currentFps;
    stream >> pixelDepth;
    stream >> userOutputSelector;
    stream >> userOutputValue;
    stream >> lineSelector;
    stream >> lineMode;
    stream >> lineInverter;
    stream >> lineSource;
    B_DESERIALIZE_END
}
#endif

} // namespace video::device::param
