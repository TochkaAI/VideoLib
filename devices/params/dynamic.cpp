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

#include "devices/params/dynamic.h"

#ifdef DAHENG_SUPPORT
#include "devices/params/daheng.h"
#endif

#include "shared/utils.h"
#include "shared/logger/logger.h"
#include "shared/logger/format.h"
#include "shared/qt/logger_operators.h"

#define log_error_m   alog::logger().error   (alog_line_location, "DevParam")
#define log_warn_m    alog::logger().warn    (alog_line_location, "DevParam")
#define log_info_m    alog::logger().info    (alog_line_location, "DevParam")
#define log_verbose_m alog::logger().verbose (alog_line_location, "DevParam")
#define log_debug_m   alog::logger().debug   (alog_line_location, "DevParam")
#define log_debug2_m  alog::logger().debug2  (alog_line_location, "DevParam")

namespace video::device::param {
namespace detail {

EnumInfo::Ptr EnumInfo::create()
{
    return Ptr(new EnumInfo);
}

#ifdef PPROTO_QBINARY_SERIALIZE
void EnumInfo::Item::toRaw(bserial::DataStream& stream) const
{
    B_SERIALIZE_V1
    stream << value;
    stream << description;
    B_SERIALIZE_RETURN
}

void EnumInfo::Item::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect)
    stream >> value;
    stream >> description;
    B_DESERIALIZE_END
}

void EnumInfo::toRaw(bserial::DataStream& stream) const
{
    B_SERIALIZE_V1
    stream << items;
    B_SERIALIZE_RETURN
}

void EnumInfo::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect)
    stream >> items;
    B_DESERIALIZE_END
}
#endif

} // namespace detail

Dynamic::Ptr loadDynamic(
               int deviceIndex, device::Type type, const yaml::Config& config)
{
    (void) deviceIndex;
    (void) type;
    (void) config;

#ifdef DAHENG_SUPPORT
    if (type == device::Type::Daheng)
    {
        Daheng::Ptr daheng {new Daheng};
        if (!daheng->load(deviceIndex, config))
            return {};

        return daheng;
    }
#endif
    return {};
}

} // namespace video::device::param
