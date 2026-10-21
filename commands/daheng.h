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

#include "pproto/commands/base.h"
#include "devices/params/daheng.h"

namespace pproto {
namespace command {

//----------------------------- Список команд --------------------------------

/**
  Команда возвращает/устанавливает параметры для камеры Daheng
*/
extern const QUuidEx DahengSettings;

} // namespace command

namespace data {

struct DahengSettings : Data<&command::DahengSettings,
                              Message::Type::Command,
                              Message::Type::Answer,
                              Message::Type::Event>
{
    video::device::param::Daheng::Ptr params;

#ifdef PPROTO_QBINARY_SERIALIZE
    DECLARE_B_SERIALIZE_FUNC
#endif

#ifdef PPROTO_JSON_SERIALIZE
    J_SERIALIZE_BEGIN
        J_SERIALIZE_MAP_OPT( "params" , params )
    J_SERIALIZE_END
#endif
};

} // namespace data
} // namespace pproto
