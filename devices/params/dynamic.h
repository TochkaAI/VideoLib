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

#include "devices/type.h"

#include "shared/defmac.h"
#include "shared/clife_base.h"
#include "shared/clife_ptr.h"
#include "shared/config/yaml_config.h"

#ifdef PPROTO_QBINARY_SERIALIZE
#include "pproto/serialize/qbinary.h"
#endif

#ifdef PPROTO_JSON_SERIALIZE
#include "pproto/serialize/json.h"
#endif

namespace video::device::param {

using namespace std;

/**
  Статус присвоения/получения нового значения для параметра
*/
enum class Status : quint32
{
    Unknown     = 0,
    Success     = 1,
    Failed      = 2,
    Unsupported = 3
};

namespace detail {

#ifdef PPROTO_QBINARY_SERIALIZE
DEFINE_B_SERIALIZE_STREAM_OPERATORS
#endif

/**
  Вспомогательная структура, содержит информацию по перечисляемым типам
*/
struct EnumInfo : clife_base
{
    typedef clife_ptr<EnumInfo> Ptr;

    static Ptr create();

    struct Item
    {
        qint64  value = {0};
        QString description;

#ifdef PPROTO_QBINARY_SERIALIZE
        DECLARE_B_SERIALIZE_FUNC
#endif

#ifdef PPROTO_JSON_SERIALIZE
        J_SERIALIZE_BEGIN
            J_SERIALIZE_ITEM( value       )
            J_SERIALIZE_ITEM( description )
        J_SERIALIZE_END
#endif
    };

    QVector<Item> items;

#ifdef PPROTO_QBINARY_SERIALIZE
        DECLARE_B_SERIALIZE_FUNC
#endif

#ifdef PPROTO_JSON_SERIALIZE
        J_SERIALIZE_ONE( items )
#endif

};

/**
  Определяет значения параметра и статус его присвоения/получения

  Примечание: для корректной работы  потоковых  операторов  определенных
  в макросе DEFINE_B_SERIALIZE_STREAM_OPERATORS структура  Param  должна
  быть описана внутри дополнительного пространства имен, в данном случае
  в 'namespace detail'
*/
template<typename U, bool RO = false> struct Param : clife_base
{
    typedef clife_ptr<Param<U>> Ptr;

    Param () = default;
    DISABLE_DEFAULT_COPY(Param)

    Status status = {Status::Unknown};

    // В режиме Mode::Set поле должно содержать новое (присваиваемое)
    // значение.
    // В режиме Mode::Get поле содержит текущее значение камеры
    U value = U{};

    // В режиме Mode::Set поле содержит предыдущее значение камеры.
    // В режиме Mode::Get поле не используется
    U preValue = U{};

    // Информация о перечисляемом типе
    EnumInfo::Ptr enumInfo;

    // Признак параметра "только для чтения"
    const bool readOnly = {RO};

    Ptr clone() const
    {
        Ptr p {new Param<U>};
        p->value = value;
        p->preValue = preValue;
        p->enumInfo = enumInfo;
        return p;
    }

#ifdef PPROTO_QBINARY_SERIALIZE
    DECLARE_B_SERIALIZE_FRIENDS
    void toRaw(bserial::DataStream& stream) const
    {
        B_SERIALIZE_V1
        stream << status;
        stream << value;
        stream << preValue;
        stream << enumInfo;
        B_SERIALIZE_RETURN
    }
    void fromRaw(const bserial::RawVector& vect)
    {
        B_DESERIALIZE_V1(vect)
        stream >> status;
        stream >> value;
        stream >> preValue;
        stream >> enumInfo;
        B_DESERIALIZE_END
    }
#endif

#ifdef PPROTO_JSON_SERIALIZE
    J_SERIALIZE_BEGIN
        J_SERIALIZE_MAP_ITEM( "status"    , status   )
        J_SERIALIZE_MAP_ITEM( "value"     , value    )
        J_SERIALIZE_MAP_ITEM( "pre_value" , preValue )
        J_SERIALIZE_MAP_OPT ( "enum_info" , enumInfo )
    J_SERIALIZE_END
#endif
};

} // namespace detail

/**
  Базовая структура для динамических параметров видеоустройства
*/
struct Dynamic : clife_base
{
    typedef clife_ptr<Dynamic> Ptr;

    // Режим использования параметров:
    //   Get - Dynamic-структура использовалась для получения параметров с камеры;
    //   Set - Dynamic-структура использовалась для присвоения параметров камере;
    //   SubTypeError - ошибка преобразования входящей Dynamic-структуры к типу
    //                  параметров камеры.
    enum class Mode {Unknown, Get, Set, SubTypeError};

    template<typename T>
    static void initParam(clife_ptr<detail::Param<T>>& param)
    {
        param = clife_ptr<detail::Param<T>>(new detail::Param<T>);
    }

    // Тип видеоустройства
    const Type type = {video::device::Type::Unknown};

    // Режим использования
    Mode mode = {Mode::Unknown};

    Dynamic(Type t) : type(t) {}
    DISABLE_DEFAULT_FUNC(Dynamic)

    virtual bool load(int deviceIndex, const yaml::Config&) = 0;
    virtual bool save(int deviceIndex, yaml::Config&, std::string = {}) = 0;

    virtual Dynamic::Ptr clone() const = 0;
};

Dynamic::Ptr loadDynamic(int deviceIndex, device::Type, const yaml::Config&);

} // namespace video::device::param
