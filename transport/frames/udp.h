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

#include "base.h"

#include "shared/list.h"
#include "shared/clife_base.h"
#include "shared/clife_alloc.h"
#include "shared/clife_ptr.h"
#include "shared/container_ptr.h"
#include <QtCore>

namespace video::transport {

bool checkUdpSignature(char*& buff);
quint8 readFrameCode(char*& buff);

/**
  Заголовок передаваемого видеофрейма
  Код фрейма: 0
*/
struct FrameHeader : FrameHeaderBase
{
    typedef container_ptr<FrameHeader> Ptr;

    qint32 writeToBuffer(char* buff);
    static Ptr readFromBuffer(char* buff);
};

/**
  Кусок видеокадра
  Код фрейма: 1
*/
struct FrameItem
{
    quint8  deviceIndex; // Идентификатор устройства
    quint64 frameNumber; // Номер кадра
    quint16 chunkNumber; // Порядковый номер передаваемого куска
    quint16 chunkStep;   // Шаг
    quint16 chunkSize;   // Размер передаваемого куска
    quint32 dataSize;    // Полный размер передаваемого кадра

    qint32 writeToBuffer(char* buff, char* data);
    void readFromBuffer(char*& buff);
};

/**
  Замыкающий фрейм. Используется для информирования принимающей стороны о том,
  что все куски отправлены.
  Код фрейма: 2
*/
struct FrameFinish
{
    enum class Status : qint32
    {
        Success = 0, // Все данные переданы.
        Timeout = 1, // Передача данных прервана, время отведенное на передачу
                     // кадра истекло.
    };

    quint64 senderId;    // Сессионный идентификатор сендера
    quint8  deviceIndex; // Идентификатор устройства
    quint64 frameNumber; // Номер кадра
    Status  status = {Status::Success};

    qint32 writeToBuffer(char* buff);
    void readFromBuffer(char* buff);
};

/**
  Фрейм обратной связи. Используется для уведомления отправляющей стороны о том,
  что все данные получены или о том, что требуется повторная пересылка данных
*/
struct FrameFeedback : clife_base
{
    typedef clife_ptr<FrameFeedback> Ptr;
    typedef clife_alloc<FrameFeedback> FeedbackAlloc;
    typedef lst::List<FrameFeedback, lst::CompareItemDummy, FeedbackAlloc> List;

    enum class Status : qint32
    {
        Undefined   = 0,
        Success     = 1, // Все данные получены
        RepeateHead = 2, // Требуется повторная пересылка заголовка
        RepeatItem  = 3  // Требуется повторная пересылка данных
    };

    quint64 terminalId;  // Сессионный идентификатор терминала
    quint64 senderId;    // Сессионный идентификатор сендера
    quint8  deviceIndex; // Идентификатор устройства
    quint64 frameNumber; // Номер кадра

    Status  status = {Status::Undefined};
    QVector<quint16> chunkNumbers; // Список номеров потерянных кусков данных

    qint32 writeToBuffer(char* buff);
    static Ptr readFromBuffer(char* buff);
};

/**
  Буфер для хранения данных кадра
*/
class FrameBuff : public clife_base
{
public:
    typedef clife_ptr<FrameBuff> Ptr;

    ~FrameBuff();
    static Ptr alloc(size_t size);

    char* ptr() const {return _ptr;}
    int size() const {return _size;}

private:
    char* _ptr = {0};
    int _size = {0};
};


} // namespace video::transport
