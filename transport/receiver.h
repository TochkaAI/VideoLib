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

  В модуле реализован механизм по приему видео-фреймов.

*****************************************************************************/

#pragma once

#include "transport/frames/udp.h"

#include "shared/list.h"
#include "shared/defmac.h"
#include "shared/clife_base.h"
#include "shared/clife_alloc.h"
#include "shared/clife_ptr.h"
#include "shared/safe_singleton.h"
#include "shared/qt/qthreadex.h"
#include "pproto/host_point.h"

#include <QtCore>
#include <atomic>

#ifdef Q_OS_WIN
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

namespace video::transport {

using namespace pproto;

class Receiver : public QThreadEx, public clife_base
{
public:
    typedef clife_ptr<Receiver> Ptr;
    typedef clife_alloc<Receiver> Alloc;
    typedef lst::List<Receiver, lst::CompareItemDummy, Alloc> List;

    quint64 terminalId()  const {return _terminalId;}
    quint8  deviceIndex() const {return _deviceIndex;}
    quint8  fps()  const {return _fps;}
    quint16 port() const {return _port;}

    bool isFit(quint64 terminalId, quint8 deviceIndex, quint8 fps) const;

signals:
    // Эмитирует полученный кадр
    void frameReceived(const video::transport::FrameHeader::Ptr&,
                       const video::transport::FrameBuff::Ptr&);

private:
    Receiver() = default;
    bool init(quint64 terminalId, quint8 deviceIndex, quint8 fps,
              const HostPoint& feedbackPoint);
    void run() override;

private:
    Q_OBJECT
    DISABLE_DEFAULT_COPY(Receiver)

    quint64 _terminalId = quint64(-1);
    quint8  _deviceIndex = quint8(-1);
    quint8  _fps = {25};
    quint16 _port = quint16(-1);

    // Адрес отправляющей стороны, нужен для работы механизма обратной связи
    struct sockaddr_in _feedbackAddr;

    // Сокет для приема входящих пакетов
    int _socket = {-1};

    friend class RecvPool;
};

class RecvPool : public QObject
{
public:
    RecvPool();
    bool init();

    bool stop(unsigned long timeout);
    void terminate();

    bool exists(quint64 terminalId, quint8 deviceIndex, quint8 fps) const;

    Receiver::Ptr add(quint64 terminalId, quint8 deviceIndex, quint8 fps,
                      const HostPoint& feedbackPoint);

    Receiver::Ptr item(quint64 terminalId, quint8 deviceIndex, quint8 fps);

    void remove(quint64 terminalId);
    void remove(quint64 terminalId, quint8 deviceIndex);
    void removeByPort(quint16 port);

    void clear();

private:
    Q_OBJECT
    DISABLE_DEFAULT_COPY(RecvPool)

private:
    Receiver::List _receivers;
    mutable QMutex _threadLock;

    template<typename T, int> friend T& safe::singleton();
};

RecvPool& recvPool();

} // namespace video::transport
