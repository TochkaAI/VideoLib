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
  ---

  В модуле реализован механизм по отправке видео-фреймов на удаленный
  визуальный терминал.

*****************************************************************************/

#pragma once

#include "video_frame.h"
#include "transport/commands.h"
#include "transport/frames/udp.h"

#include "shared/list.h"
#include "shared/defmac.h"
#include "shared/clife_base.h"
#include "shared/clife_alloc.h"
#include "shared/clife_ptr.h"
#include "shared/steady_timer.h"
#include "shared/qt/qthreadex.h"
#include "pproto/message.h"

#include <QtCore>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>

namespace video::transport {

using namespace pproto;

class SendPool;

/**
  Используется для подсчета потерь UDP пакетов для терминалов
*/
struct LostStat : clife_base
{
    typedef clife_ptr<LostStat> Ptr;
    typedef clife_alloc<LostStat> Alloc;
    typedef lst::List<LostStat, lst::CompareItemDummy, Alloc> List;

    // Сессионный идентификатор терминала
    quint64 terminalId;
    struct sockaddr_in terminalAddr;

    quint8  deviceIndex;
    quint8  fps;
    quint8  frameScale;

    quint32 lostChunks;
    quint32 totalChunks;
};

class Sender : public QThreadEx
{
private:
    Sender(SendPool* sendPool) : _sendPool(sendPool) {}
    bool init(quint8 deviceIndex, quint8 fps, quint8 frameScale,
              quint16 chunkStep, VideoFrameCompression compression);

    bool isFit(quint8 deviceIndex) const;
    bool isFit(quint8 deviceIndex, quint8 fps, quint8 frameScale,
               quint16 chunkStep, VideoFrameCompression compression) const;

    void addTerminal(SocketDescriptor, quint64 terminalId,
                     const HostPoint& remotePoint);

    // Удаляет терминал с указанным terminalId
    void removeTerminal(quint64 terminalId);

    // Удаляет терминал с указанным SocketDescriptor
    void removeTerminalBySocket(SocketDescriptor);

    bool terminalsIsEmpty() const;

    void setFrame(const video::Frame::Ptr&);
    void addFeedback(const FrameFeedback::Ptr&);

private:
    DISABLE_DEFAULT_FUNC(Sender)
    void run() override;

private:
    SendPool* const _sendPool;

    // Сессионный идентификатор сендера
    const quint64 _id = {hash64(QUuidEx::createUuid())};

    // Пять следующих параметра устанавливаются в функции init() и более
    // не меняются
    quint8 _deviceIndex = {0};
    quint8 _fps = {25};
    quint8 _frameScale = {100};

    // Размер блока данных отправляемого в UDP-сокет. Оптимальный размер блока
    // для передачи по локальной сети 65456 байт
    quint16 _chunkStep = {65456};

    // Признак сжатия кадра
    VideoFrameCompression _compression = {VideoFrameCompression::None};

    struct Terminal : clife_base
    {
        typedef clife_ptr<Terminal> Ptr;
        typedef clife_alloc<Terminal> Alloc;
        typedef lst::List<Terminal, lst::CompareItemDummy, Alloc> List;

        // Дескриптор TCP подключения терминала
        SocketDescriptor socketDescriptor = {-1};

        // Сессионный идентификатор терминала
        quint64 id;

        // Адрес терминала для отправки UDP пакетов
        struct sockaddr_in remoteAddr;

        // Признак полной доставки кадра до терминала, выставляется в TRUE,
        // когда от терминала получен  ответ о том,  что все  куски  кадра
        // доставлены
        bool frameReceived = {false};

        // Таймер повторной отправки команды FrameFinish
        steady_timer resendFinishTimer;

        // Параметр для вычисления статистики потерянных кусков
        quint32 lostChunks;

        Terminal() {
            memset(&remoteAddr, 0, sizeof(remoteAddr));
        }
    };

    // Список TCP-дескрипторов вновь добавленных терминалов
    SocketDescriptorSet _newTerminalDescriptors;

    Terminal::List _terminals;
    volatile bool  _terminalsChanged;
    mutable QMutex _terminalsLock;

    // Идентификатор UDP сокета
    int _socket = {-1};

    Frame::Ptr _frame;
    QMutex _frameLock;
    QWaitCondition _frameCond;

    FrameFeedback::List _feedbacks;
    QMutex _feedbacksLock;
    QWaitCondition _feedbacksCond;

    friend class SendPool;
};

class SendPool : public QThreadEx
{
public:
    SendPool() = default;
    bool init(int feedbackPort);

public slots:
    void message(const pproto::Message::Ptr&);
    void socketConnected(pproto::SocketDescriptor);
    void socketDisconnected(pproto::SocketDescriptor);

    void setFrame(const video::Frame::Ptr&);

private:
    Q_OBJECT
    DISABLE_DEFAULT_COPY(SendPool)
    void run() override;

    // Выполняет подсчет статистики потерянных кусков
    void addLostStat(const LostStat::Ptr&);

private:
    // Порт и сокет обратной связи, используются для получения FrameFeedback
    // пакетов
    quint16 _feedbackPort = {0};
    int _feedbackSocket = {-1};

    lst::List<Sender> _senders;
    QMutex _threadLock;

    LostStat::List _lostStatList;
    mutable std::atomic_flag _lostStatLock = ATOMIC_FLAG_INIT;

    friend class Sender;
    template<typename T, int> friend T& safe::singleton();
};

SendPool& sendPool();

} // namespace video::transport
