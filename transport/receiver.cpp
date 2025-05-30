#include "receiver.h"

#include "shared/break_point.h"
#include "shared/spin_locker.h"
#include "shared/steady_timer.h"
#include "shared/logger/logger.h"
#include "shared/network/logger_operators.h"
#include "shared/qt/logger_operators.h"
#include "pproto/transport/tcp.h"

//#include <opencv2/opencv.hpp>
#ifdef Q_OS_WIN
#include <ws2tcpip.h>
#else
#include <sys/poll.h>
#include <arpa/inet.h>
#endif
#include <unistd.h>

#ifdef Q_OS_WIN
#define POLLRDNORM 0x0100
#define POLLRDBAND 0x0200
#define POLLIN    (POLLRDNORM | POLLRDBAND)
#define POLLPRI    0x0400

typedef struct pollfd {
        SOCKET  fd;
        SHORT   events;
        SHORT   revents;
} WSAPOLLFD, *PWSAPOLLFD, FAR *LPWSAPOLLFD;
extern "C" WINSOCK_API_LINKAGE int WSAAPI WSAPoll(LPWSAPOLLFD fdArray, ULONG fds, INT timeout);
#endif

#ifdef Q_OS_WIN
#define RECV_ERRNO WSAGetLastError()
#else
#define RECV_ERRNO errno
#endif

#define log_error_m   alog::logger().error  (alog_line_location, "VideoRecv")
#define log_warn_m    alog::logger().warn   (alog_line_location, "VideoRecv")
#define log_info_m    alog::logger().info   (alog_line_location, "VideoRecv")
#define log_verbose_m alog::logger().verbose(alog_line_location, "VideoRecv")
#define log_debug_m   alog::logger().debug  (alog_line_location, "VideoRecv")
#define log_debug2_m  alog::logger().debug2 (alog_line_location, "VideoRecv")

namespace video::transport {

using namespace std;

bool Receiver::isFit(quint64 terminalId, quint8 deviceIndex, quint8 fps) const
{
    return _terminalId == terminalId
           && _deviceIndex == deviceIndex
           && _fps == fps;
}

bool Receiver::init(quint64 terminalId, quint8 deviceIndex, quint8 fps,
                    const HostPoint& feedbackPoint)
{
    if (isRunning())
    {
        log_error_m << "Impossible execute a initialization "
                       "because Receiver thread is running";
        return false;
    }

    _terminalId = terminalId;
    _deviceIndex = deviceIndex;
    _fps = fps;

    memset(&_feedbackAddr, 0, sizeof(_feedbackAddr));

    // Отладка. Порт пока захардкодили
    //_feedbackAddr.sin_port = htons(55006);
    //inet_aton("127.0.0.1", &_feedbackAddr.sin_addr);

    _feedbackAddr.sin_port = htons(feedbackPoint.port());
    _feedbackAddr.sin_family = AF_INET;
    _feedbackAddr.sin_addr.s_addr = htonl(feedbackPoint.address().toIPv4Address());

#ifdef Q_OS_WIN
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)
    {
        log_error_m << "Failed call WSAStartup()";
        return false;
    }
#endif

    _socket = socket(AF_INET, SOCK_DGRAM /*| SOCK_NONBLOCK*/, 0);
    if (_socket == -1)
    {
#ifdef Q_OS_WIN
        WSACleanup();
#endif
        log_error_m << "Can't create socket";
        return false;
    }

    struct sockaddr_in bindAddr;
    memset(&bindAddr, 0, sizeof(bindAddr));

    bindAddr.sin_family = AF_INET;
    bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    quint16 port = 55006;
    bindAddr.sin_port = htons(port);

    // Ищем доступный порт в диапазоне 55006-55056
    while (-1 == ::bind(_socket, (sockaddr*) &bindAddr, sizeof(bindAddr)))
    {
        log_verbose_m << "Can't bind socket with address " << bindAddr
                      << " (errno " << RECV_ERRNO << ")";

        bindAddr.sin_port = htons(++port);
        if (port > 55056)
        {
#ifdef Q_OS_WIN
            WSACleanup();
#endif
            log_error_m << "Failed bind UDP socket"
                        << ". Pool of ports (55006-55056) is exhausted";
            return false;
        }
    }
    _port = port;
    log_info_m << "Socket successfully bound with address " << bindAddr;

    return true;
}

void Receiver::run()
{
    log_info_m << "Receiver started. Device index: " << int(_deviceIndex);

    // Временной интервал эмитирования кадров (в микросекундах)
    const double emitInterval = 1000000. / _fps * 0.98;
    steady_timer emitTimer;

    struct Frame
    {
        // Номер кадра
        quint64 number = quint64 (-1);

        // Заголовок кадка
        FrameHeader::Ptr header;

        // Полученные куски данных
        QVector<bool> chunks;

        // Признак полного получения кадра
        bool received = {false};

        // Буфер данных
        FrameBuff::Ptr data;

        // Признак выставляется в TRUE, когда кадр пропустил время для эмитиро-
        // вания (один интервал), если ко времени  следующего  интервала  кадр
        // все еще не будет готов к отображению - он будет отброшен
        bool emitSkip = {false};

        // Стратегия поиска
        struct Find
        {
            int operator() (const quint64* frameNumber, const Frame* f) const
                {return LIST_COMPARE_ITEM(*frameNumber, f->number);}
        };
        typedef lst::List<Frame, Frame::Find> List;
    };
    Frame::List frames;

    FrameBuff::Ptr buff = FrameBuff::alloc(65536);

    sockaddr_in remoteAddr;
    socklen_t remoteAddrLen = sizeof(remoteAddr);
    memset(&remoteAddr, 0, remoteAddrLen);

    int ret;
#ifdef Q_OS_WIN
    WSAPOLLFD fds[1];
#else
    pollfd fds[1];
#endif
    fds[0].fd = _socket;
    fds[0].events = POLLIN;

#ifdef VIDEO_TRANSPORT_DEBUG
    steady_timer recvTimer;
#endif

    while (true)
    {
        CHECK_QTHREADEX_STOP

        if (emitTimer.elapsed<chrono::microseconds>() > emitInterval)
        {
            emitTimer.reset();
            if (!frames.empty())
            {
                Frame* f = frames.item(0);
                if (f->received)
                {
#ifdef VIDEO_TRANSPORT_DEBUG
                    log_debug2_m << "Frame emit " << f->number;
#endif
                    //QString fileName = QString("/tmp/1/%1.png").arg(f->number);
                    //cv::Mat image {f->header->realHeight, f->header->realWidth, f->header->cvType, (void*)f->data.constData()};
                    //cv::imwrite(fileName.toUtf8().constData(), image);

                    emit frameReceived(f->header, f->data);
                    frames.remove(0);
                }
                else
                {
                    if (f->emitSkip)
                    {
#ifdef VIDEO_TRANSPORT_DEBUG
                        log_debug2_m << "Frame drop " << f->number;
#endif
                        // Эмитирование кадра один раз уже было пропущено,
                        // третьего шанса у него не будет...
                        frames.remove(0);
                    }
                    else
                    {
#ifdef VIDEO_TRANSPORT_DEBUG
                        log_debug2_m << "Emit skip " << f->number;
#endif
                        // Даем кадру второй шанс...
                        f->emitSkip = true;
                    }
                }
            }
        }

        int pollDelay = (frames.empty()) ? 10 : 1;

        // Ждем данных в сокете (таймаут pollDelay в миллисекундах)
#ifdef Q_OS_WIN
        ret = WSAPoll((LPWSAPOLLFD)&fds, 1, pollDelay);
#else
        ret = poll(fds, 1, pollDelay);
#endif
        if (0 == ret)
        {
            // В эту секцию попадаем если интервал ожидания функции poll()
            // истек и не было получено новых пакетов.
            continue;
        }

        if (-1 == ret)
        {
            log_error_m << "Error waiting of socket (errno " << RECV_ERRNO << ")";
            sleep(1);
            continue;
        }

        ret = recvfrom(_socket, buff->ptr(), buff->size(), 0,
                       (struct sockaddr*) &remoteAddr, &remoteAddrLen);
        if (-1 == ret)
        {
            log_error_m << "Can't read from socket (errno " << RECV_ERRNO << ")"
                        << ". Remote address " << remoteAddr;
            sleep(1);
            continue;
        }
        if (ret >= int(buff->size()))
        {
            log_error_m << "Can't handle packet. Datagram too big"
                        << ". Remote address " << remoteAddr;
            continue;
        }

        char* buffCur = buff->ptr();
        if (!checkUdpSignature(buffCur))
        {
            log_error_m << "Can't handle packet. Incompatible signature"
                        << ". Remote address " << remoteAddr;
            continue;
        }

        // Код фрейма
        quint8 frameCode = readFrameCode(buffCur);

        if (frameCode == 0 /*FrameHeader*/)
        {
            FrameHeader::Ptr header = FrameHeader::readFromBuffer(buffCur);
            if (_deviceIndex != header->deviceIndex)
            {
                if (alog::logger().level() == alog::Level::Debug2)
                    log_debug2_m << "Incompatible header-frame device index"
                                 << ". Expected: " << _deviceIndex
                                 << ". Received: " << header->deviceIndex;
                continue;
            }

            // Отбрасываем данные для уже эмитированных кадров
            if (!frames.empty() && (frames.item(0)->number > header->frameNumber))
                continue;

#ifdef VIDEO_TRANSPORT_DEBUG
            log_debug2_m << "Receive FrameHeader. frameNumber: " << header->frameNumber
                         << ". chunkCount: " << header->chunkCount;
#endif
            Frame* f = 0;
            if (lst::FindResult fr = frames.findRef(header->frameNumber, {lst::BruteForce::Yes}))
            {
                f = frames.item(fr.index());
                if (f->received)
                    continue;
            }
            else
            {
#ifdef VIDEO_TRANSPORT_DEBUG
                recvTimer.reset();
#endif
                f = frames.add();
                f->number = header->frameNumber;
            }

            if (f->header.empty())
            {
                f->header = std::move(header);
                f->chunks.resize(f->header->chunkCount);

                if (f->data.empty())
                {
                    f->data = FrameBuff::alloc(f->header->dataSize);
                }
            }
        }

        else if (frameCode == 1 /*FrameItem*/)
        {
            FrameItem item;
            item.readFromBuffer(buffCur);
            if (_deviceIndex != item.deviceIndex)
            {
                if (alog::logger().level() == alog::Level::Debug2)
                    log_debug2_m << "Incompatible item-frame device index"
                                 << ". Expected: " << _deviceIndex
                                 << ". Received: " << item.deviceIndex;
                continue;
            }

            // Отбрасываем данные для уже эмитированных кадров
            if (!frames.empty() && (frames.item(0)->number > item.frameNumber))
                continue;

#ifdef VIDEO_TRANSPORT_DEBUG
            //log_debug2_m << "Receive FrameItem. frameNumber: " << item.frameNumber
            //             << ". chunkNumber: " << item.chunkNumber;
#endif
            Frame* f = 0;
            if (lst::FindResult fr = frames.findRef(item.frameNumber, {lst::BruteForce::Yes}))
            {
                f = frames.item(fr.index());
                if (f->received)
                {
#ifdef VIDEO_TRANSPORT_DEBUG
                    log_debug2_m << "f->received. frameNumber: " << f->number;
#endif
                    continue;
                }

                if (item.chunkNumber < f->chunks.count()
                    && f->chunks[item.chunkNumber] == true)
                {
#ifdef VIDEO_TRANSPORT_DEBUG
                    log_debug2_m << "Chunk exists (chunks[item.chunkNumber] == true)"
                                 << ". frameNumber: " << f->number
                                 << ". chunkNumber: " << item.chunkNumber;
#endif
                    continue;
                }
            }
            else
            {
                f = frames.add();
                f->number = item.frameNumber;
            }

            if (f->header.empty())
            {
#ifdef VIDEO_TRANSPORT_DEBUG
                log_debug2_m << "Header empty. frameNumber: " << f->number
                             << ". chunkNumber: " << item.chunkNumber
                             << ". dataSize: " << item.dataSize;
#endif
                if (f->data.empty())
                    f->data = FrameBuff::alloc(item.dataSize);
            }

            // Пишем кусок данных в буфер кадра
            //char* dataBegin = f->data.data();
            char* dataBegin = f->data->ptr();
            char* chunkDest = dataBegin + (item.chunkNumber * item.chunkStep);
            memcpy(chunkDest, buffCur, item.chunkSize);

            if (item.chunkNumber >= f->chunks.count())
                f->chunks.resize(item.chunkNumber + 1);

            // Помечаем данные как записанные
            f->chunks[item.chunkNumber] = true;
        }

        else if (frameCode == 2 /*FrameFinish*/)
        {
            FrameFinish finish;
            finish.readFromBuffer(buffCur);
            if (_deviceIndex != finish.deviceIndex)
            {
                if (alog::logger().level() == alog::Level::Debug2)
                    log_debug2_m << "Incompatible finish-frame device index"
                                 << ". Expected: " << _deviceIndex
                                 << ". Received: " << finish.deviceIndex;
                continue;
            }

            // Отбрасываем данные для уже эмитированных кадров
            if (!frames.empty() && (frames.item(0)->number > finish.frameNumber))
                continue;

#ifdef VIDEO_TRANSPORT_DEBUG
            log_debug2_m << "Receive FrameFinish. frameNumber: " << finish.frameNumber;
#endif
            lst::FindResult fr = frames.findRef(finish.frameNumber, {lst::BruteForce::Yes});
            if (fr.failed())
            {
#ifdef VIDEO_TRANSPORT_DEBUG
                if (alog::logger().level() == alog::Level::Debug2)
                    log_debug2_m << "Finish command. Frame number "
                                 << finish.frameNumber << " not found";
#endif
                continue;
            }

            Frame* f = frames.item(fr.index());

            FrameFeedback frameFeedback;
            frameFeedback.terminalId  = _terminalId;
            frameFeedback.senderId    = finish.senderId;
            frameFeedback.deviceIndex = _deviceIndex;
            frameFeedback.frameNumber = f->number;

            auto sendToSocket = [this](char* buff, int writeSize, const struct sockaddr_in& addr)
            {
                if (-1 == sendto(_socket, buff, writeSize, 0,
                                 (struct sockaddr*) &addr, sizeof(addr)))
                {
                    log_error_m << "Can't send datagramm to socket (errno " << RECV_ERRNO << ")"
                                << ". Host: " << addr;
                }
            };

            if (f->received)
            {
#ifdef VIDEO_TRANSPORT_DEBUG
                log_debug2_m << "Frame received (repeat). frameNumber: " << f->number
                             << ". Time: " << recvTimer.elapsed<chrono::microseconds>();
#endif
                frameFeedback.status = FrameFeedback::Status::Success;

                int writeSize = frameFeedback.writeToBuffer(buff->ptr());
                sendToSocket(buff->ptr(), writeSize, _feedbackAddr);
                continue;
            }

            // Если от отправляющей стороны получаем уведомление о том, что
            // она больше не может  отправлять  куски  кадра, так как время
            // на отправку  этого  кадра  истекло,  то в этом  случае  кадр
            // отбрасываем
            if (finish.status == FrameFinish::Status::Timeout)
            {
#ifdef VIDEO_TRANSPORT_DEBUG
                log_debug2_m << "Frame timeout. frameNumber: " << f->number;
#endif
                frames.remove(fr.index());
                continue;
            }

            // Если нет заголовка - отправляем об этом сообщение
            if (f->header.empty())
            {
#ifdef VIDEO_TRANSPORT_DEBUG
                log_debug2_m << "Header empty. frameNumber: " << f->number;
#endif
                frameFeedback.status = FrameFeedback::Status::RepeateHead;

                int writeSize = frameFeedback.writeToBuffer(buff->ptr());
                sendToSocket(buff->ptr(), writeSize, _feedbackAddr);
            }

            // Заполняем номера потерянных кусков
            for (int i = 0; i < f->chunks.count(); ++i)
                if (f->chunks[i] == false)
                    frameFeedback.chunkNumbers.append(i);

            // Если есть потерянные куски, то отправляем об этом сообщение
            if (!frameFeedback.chunkNumbers.isEmpty())
            {
#ifdef VIDEO_TRANSPORT_DEBUG
                log_debug2_m << "FrameFeedback chunkNumbers NOT empty"
                             << ". frameNumber: " << f->number
                             << ". chunkNumbers: " << frameFeedback.chunkNumbers.count();
                { //Block alog::Line
                    alog::Line logLine = log_debug2_m << "Chunk List: ";
                    for (quint16 i : frameFeedback.chunkNumbers)
                        logLine << i << ", ";
                }
#endif
                frameFeedback.status = FrameFeedback::Status::RepeatItem;

                int writeSize = frameFeedback.writeToBuffer(buff->ptr());
#ifdef VIDEO_TRANSPORT_DEBUG
                log_debug2_m << "FrameFeedback chunkNumbers write size: " << writeSize;
#endif
                sendToSocket(buff->ptr(), writeSize, _feedbackAddr);
            }

            // Если заголовок получен, и нет потерянных кусков, то отправляем
            // сообщение о том, что кадр получен
            if (f->header && frameFeedback.chunkNumbers.isEmpty())
            {
#ifdef VIDEO_TRANSPORT_DEBUG
                log_debug2_m << "Frame received. frameNumber: " << f->number
                             << ". Time: " << recvTimer.elapsed<chrono::microseconds>();
#endif
                f->received = true;
                frameFeedback.status = FrameFeedback::Status::Success;

                int writeSize = frameFeedback.writeToBuffer(buff->ptr());
                sendToSocket(buff->ptr(), writeSize, _feedbackAddr);
                usleep(20);
                sendToSocket(buff->ptr(), writeSize, _feedbackAddr);
            }
        } // FrameFinish

    } // while (true)

    close(_socket);
    _socket = -1;

#ifdef Q_OS_WIN
    WSACleanup();
#endif

    log_info_m << "Receiver stopped. Device index: " << int(_deviceIndex);
}

//--------------------------------- RecvPool ---------------------------------

RecvPool::RecvPool()
{
    static bool first {true};
    if (first)
    {
        qRegisterMetaType<video::transport::FrameHeader::Ptr>("video::transport::FrameHeader::Ptr");
        qRegisterMetaType<video::transport::FrameBuff::Ptr>  ("video::transport::FrameBuff::Ptr");
        first = false;
    }
}

bool RecvPool::init()
{
    return true;
}

bool RecvPool::stop(unsigned long /*timeout*/)
{
    QMutexLocker locker {&_threadLock}; (void) locker;

    for (Receiver* recv : _receivers)
        if (!recv->stop(5*1000 /*5 сек*/))
        {
            log_info_m << "Stop thread receiver. Timeout expired, thread will be terminated";
            recv->terminate();
        }

    return true;
}

void RecvPool::terminate()
{}

bool RecvPool::exists(quint64 terminalId, quint8 deviceIndex, quint8 fps) const
{
    QMutexLocker locker {&_threadLock}; (void) locker;

    for (Receiver* r : _receivers)
        if (r->isFit(terminalId, deviceIndex, fps))
            return true;

    return false;
}

Receiver::Ptr RecvPool::add(quint64 terminalId, quint8 deviceIndex, quint8 fps,
                            const HostPoint& feedbackPoint)
{
    QMutexLocker locker {&_threadLock}; (void) locker;

    Receiver::Ptr recv {new Receiver};
    if (!recv->init(terminalId, deviceIndex, fps, feedbackPoint))
        return {};

    recv->start();
    recv->add_ref();
    _receivers.add(recv);

    return recv;
}

Receiver::Ptr RecvPool::item(quint64 terminalId, quint8 deviceIndex, quint8 fps)
{
    QMutexLocker locker {&_threadLock}; (void) locker;

    for (Receiver* r : _receivers)
        if (r->isFit(terminalId, deviceIndex, fps))
            return Receiver::Ptr(r);

    return {};
}

void RecvPool::remove(quint64 terminalId)
{
    QMutexLocker locker {&_threadLock}; (void) locker;

    for (int i = 0; i < _receivers.count(); ++i)
        if (_receivers.item(i)->terminalId() == terminalId)
        {
            _receivers.item(i)->stop();
            _receivers.remove(i--);
        }
}

void RecvPool::remove(quint64 terminalId, quint8 deviceIndex)
{
    QMutexLocker locker {&_threadLock}; (void) locker;

    for (int i = 0; i < _receivers.count(); ++i)
        if (_receivers.item(i)->terminalId() == terminalId
            && _receivers.item(i)->deviceIndex() == deviceIndex)
        {
            _receivers.item(i)->stop();
            _receivers.remove(i--);
        }
}

void RecvPool::removeByPort(quint16 port)
{
    QMutexLocker locker {&_threadLock}; (void) locker;

    for (int i = 0; i < _receivers.count(); ++i)
        if (_receivers.item(i)->port() == port)
        {
            _receivers.item(i)->stop();
            _receivers.remove(i--);
        }
}

void RecvPool::clear()
{
    QMutexLocker locker {&_threadLock}; (void) locker;

    for (Receiver* r : _receivers)
        r->stop();
    _receivers.clear();
}

RecvPool& recvPool()
{
    return safe::singleton<RecvPool>();
}

} // namespace video::transport
