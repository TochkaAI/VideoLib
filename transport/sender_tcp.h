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
#include "commands/transport.h"

#include "shared/list.h"
#include "shared/defmac.h"
#include "shared/clife_base.h"
#include "shared/clife_ptr.h"
#include "shared/clife_alloc.h"
#include "shared/steady_timer.h"
#include "shared/qt/qthreadex.h"

#include "pproto/message.h"
#include "pproto/transport/base.h"
#include "pproto/transport/tcp.h"

#include <QtCore>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>

namespace video::transport::tcp {

using namespace pproto;

class SendPool;

class Sender : public QThreadEx
{
private:
    Sender(SendPool* sendPool) : _sendPool(sendPool) {}
    bool init(quint8 deviceIndex, quint8 fps, quint8 frameScale, quint16 sizeScale,
              pproto::SizeSide sizeSide, VideoFrameCompression compression);

    bool isFit(quint8 deviceIndex) const;
    bool isFit(quint8 deviceIndex, quint8 fps, quint8 frameScale, quint16 sizeScale,
               VideoFrameCompression compression) const;

    void addTerminal(pproto::transport::base::Socket::Ptr,
                     SerializeFormat messageContentFormat);

    // Удаляет терминал с указанным SocketDescriptor
    void removeTerminal(SocketDescriptor = -1);

    bool terminalsIsEmpty() const;

    void setFrame(const video::Frame::Ptr&);

    bool frameAnswerReceived(const QUuidEx&);

private:
    DISABLE_DEFAULT_FUNC(Sender)
    void run() override;

private:
    SendPool* const _sendPool;

    // Сессионный идентификатор сендера
    const quint64 _id = {hash64(QUuidEx::createUuid())};

    // Пять следующих параметра устанавливаются в функции init() и более
    // не меняются
    quint8  _deviceIndex = {0};
    quint8  _fps = {25};
    quint8  _frameScale = {100};
    quint16 _sizeScale = {0};
    pproto::SizeSide _sizeSide = {pproto::SizeSide::Width};

    // Признак сжатия кадра
    VideoFrameCompression _compression = {VideoFrameCompression::None};

    struct Terminal : clife_base
    {
        typedef clife_ptr<Terminal> Ptr;
        typedef clife_alloc<Terminal> Alloc;
        typedef lst::List<Terminal, lst::CompareItemDummy, Alloc> List;

        // [27.03.23]
        // Для TCP соединения роль сессионного идентификатора
        // выполняет socketDescriptor.
        // // Сессионный идентификатор терминала
        // // quint64 id;

        // Признак только что добавленного терминала
        volatile bool isNew = {true};

        pproto::transport::base::Socket::Ptr socket;
        SocketDescriptor socketDescriptor = {-1};

        SerializeFormat messageContentFormat = {SerializeFormat::Json};

        // Идентификатор отправленной команды VideoSenderFrame, используется
        // для подтверждения доставки фрейма
        QUuidEx answerId;
        QMutex  answerIdLock;
    };

    Terminal::List _terminals;
    volatile bool  _terminalsChanged;
    mutable QMutex _terminalsLock;

    Frame::Ptr _frame;
    QMutex _frameLock;
    QWaitCondition _frameCond;

    friend class SendPool;
};

class SendPool : public QObject
{
public:
    SendPool() = default;
    bool init(pproto::transport::tcp::Listener* = nullptr);
    void deinit();

public slots:
    void message(const pproto::Message::Ptr&);
    void socketConnected(pproto::SocketDescriptor);
    void socketDisconnected(pproto::SocketDescriptor);

    void setFrame(const video::Frame::Ptr&);
    bool terminalsIsEmpty() const;

private:
    Q_OBJECT
    DISABLE_DEFAULT_COPY(SendPool)

private:
    lst::List<Sender> _senders;

    pproto::transport::tcp::Listener* _listener = {nullptr};

    friend class Sender;
    template<typename T, int> friend T& safe::singleton();
};

SendPool& sendPool();
SendPool& sendPool2();

} // namespace video::transport::tcp
