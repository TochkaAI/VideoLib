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

#include "commands/daheng.h"
#include "pproto/commands/pool.h"

namespace pproto {
namespace command {

REGISTRY_PPROTO_COMMAND(DahengSettings, "2518b3f5-1042-4c94-8cae-00dbe1c910d1", MULTIPROC)

} // namespace command

namespace data {

#ifdef PPROTO_QBINARY_SERIALIZE

void DahengSettings::toRaw(bserial::DataStream& stream) const
{
    B_SERIALIZE_V1
    stream << params;
    B_SERIALIZE_RETURN
}

void DahengSettings::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect)
    stream >> params;
    B_DESERIALIZE_END
}

#endif // PPROTO_QBINARY_SERIALIZE

} // namespace data
} // namespace pproto
