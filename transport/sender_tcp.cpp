#include "sender_tcp.h"

#include "shared/break_point.h"
#include "shared/prog_abort.h"
//#include "shared/spin_locker.h"
#include "shared/logger/logger.h"
#include "shared/logger/format.h"
#include "shared/qt/logger_operators.h"

#include <sys/poll.h>
#include <arpa/inet.h>
#include <unistd.h>

#ifdef WIN32
#define SND_ERRNO WSAGetLastError()
#else
#define SND_ERRNO errno
#endif

#define log_error_m   alog::logger().error  (alog_line_location, "VideoSend")
#define log_warn_m    alog::logger().warn   (alog_line_location, "VideoSend")
#define log_info_m    alog::logger().info   (alog_line_location, "VideoSend")
#define log_verbose_m alog::logger().verbose(alog_line_location, "VideoSend")
#define log_debug_m   alog::logger().debug  (alog_line_location, "VideoSend")
#define log_debug2_m  alog::logger().debug2 (alog_line_location, "VideoSend")

namespace video::transport::tcp {

using namespace std;

bool Sender::init(quint8 deviceIndex, quint8 fps, quint8 frameScale,
                  quint16 widthScale, VideoFrameCompression compression)
{
    _deviceIndex = deviceIndex;
    _fps = fps;
    _frameScale = frameScale;
    _widthScale = 0;
    _compression = compression;

    if (widthScale)
    {
        if (widthScale > 100)
        {
            quint16 remainder = widthScale % 100;
            _widthScale = widthScale - remainder + (remainder <= 50 ? 50 : 100);
        }
        else
            _widthScale = 100;
    }
    return true;
}

bool Sender::isFit(quint8 deviceIndex) const
{
    return (_deviceIndex == deviceIndex);
}

bool Sender::isFit(quint8 deviceIndex, quint8 fps, quint8 frameScale,
                   quint16 widthScale, VideoFrameCompression compression) const
{
    if (widthScale && _widthScale)
    {
        if (widthScale > 100)
        {
            quint16 remainder = widthScale % 100;
            widthScale = widthScale - remainder + (remainder <= 50 ? 50 : 100);
        }
        else
            widthScale = 100;

        return (_deviceIndex == deviceIndex
                && _fps == fps
                && _widthScale == widthScale
                && _compression == compression);
    }
    if (!widthScale && !_widthScale)
    {
        return (_deviceIndex == deviceIndex
                && _fps == fps
                && _frameScale == frameScale
                && _compression == compression);
    }
    return false;
}

void Sender::addTerminal(pproto::transport::base::Socket::Ptr socket,
                         SerializeFormat messageContentFormat)
{
    QMutexLocker locker {&_terminalsLock}; (void) locker;

    if (socket.empty())
    {
        log_error_m << "Unable create terminal for null socket";
        return;
    }

    if (!socket->isConnected())
    {
        log_error_m << "Unable create terminal for non-connected socket";
        return;
    }

    Terminal* t {new Terminal};
    t->socket = socket;
    t->socketDescriptor = socket->socketDescriptor();
    t->messageContentFormat = messageContentFormat;
    if (messageContentFormat == SerializeFormat::Json)
        t->socket->setMessageWebFlags(true);

    t->add_ref();
    _terminals.add(t);
    _terminalsChanged = true;

    pproto::transport::tcp::Socket::Ptr soctcp =
        socket.dynamic_cast_to<pproto::transport::tcp::Socket::Ptr>();

    QString contentFormat =
        (messageContentFormat == SerializeFormat::Json) ? "Json" : "Binary";

    log_verbose_m << "Terminal added"
                  << ". Remote host: " << soctcp->peerPoint()
                  << ". Socket descriptor: " << t->socketDescriptor
                  << ". Content format: " << contentFormat;
}

void Sender::removeTerminal(SocketDescriptor socketDescriptor)
{
    QMutexLocker locker {&_terminalsLock}; (void) locker;

    _terminals.removeCond([socketDescriptor, this](Terminal* t) {
        if (socketDescriptor == -1
            || socketDescriptor == t->socketDescriptor)
        {
            alog::Line logLine = log_verbose_m << "Terminal removed";
            if (t->socket->isConnected())
            {
                pproto::transport::tcp::Socket::Ptr soctcp =
                    t->socket.dynamic_cast_to<pproto::transport::tcp::Socket::Ptr>();
                logLine << ". Remote host: " << soctcp->peerPoint();
                //logLine << ". Disconnected from host: " << soctcp->peerPoint();
                //soctcp->disconnect();
            }
            logLine << ". Socket descriptor: " << t->socketDescriptor;
            _terminalsChanged = true;
            return true;
        }
        return false;
    });

    if (_terminals.empty())
    {
        QMutexLocker locker {&_frameLock}; (void) locker;
        _frame.reset();
    }
}

bool Sender::terminalsIsEmpty() const
{
    QMutexLocker locker {&_terminalsLock}; (void) locker;
    return _terminals.empty();
}

void Sender::setFrame(const video::Frame::Ptr& frame)
{
    if (terminalsIsEmpty())
        return;

    QMutexLocker locker {&_frameLock}; (void) locker;

    if (_deviceIndex == frame->deviceIndex)
    {
        _frame = frame;
        _frameCond.wakeAll();
    }
}

bool Sender::frameAnswerReceived(const QUuidEx& id)
{
    QMutexLocker locker {&_terminalsLock}; (void) locker;

    for (Terminal* t : _terminals)
    {
        QMutexLocker locker {&t->answerIdLock}; (void) locker;
        if (t->answerId == id)
        {
            t->answerId = QUuidEx();
            return true;
        }
    }
    return false;
}

void Sender::run()
{
    { //Block for alog::Line
        alog::Line logLine = log_info_m
                    << "Sender started"
                    << ". Device index: " << int(_deviceIndex)
                    << ". Fps: "          << int(_fps);

        if (_widthScale)
            logLine << ". Frame width: "  << int(_widthScale);
        else
            logLine << ". Frame scale: "  << int(_frameScale) << "%";

        logLine     << ". Compression: "  << toString(_compression);
    }

    { //Block for QMutexLocker
        QMutexLocker locker {&_terminalsLock}; (void) locker;
        _terminalsChanged = true;
    }

    steady_timer sendTimer;
    Terminal::List terminals;

    double sendInterval = {1000000. / _fps * 0.98};

    while (true)
    {
        CHECK_QTHREADEX_STOP

        Frame::Ptr frame;
        { //Block for QMutexLocker
            QMutexLocker locker {&_frameLock}; (void) locker;
            if (_frame.empty())
            {
                _frameCond.wait(&_frameLock, 10);
                if (_frame.empty())
                    continue;
            }
            frame = std::move(_frame);
        }
        if (frame.empty())
        {
            // Отладить, разобраться что делать в этой ситуации
            break_point
            continue;
        }

        auto sendVideoStreamInfo = [this, &frame](Terminal* terminal)
        {
            data::VideoStreamInfo videoStreamInfo;
            videoStreamInfo.deviceIndex = _deviceIndex;
            videoStreamInfo.frameNumber = frame->number;
            videoStreamInfo.realWidth   = frame->width;
            videoStreamInfo.realHeight  = frame->height;
            videoStreamInfo.scale       = _frameScale;
            videoStreamInfo.scaleWidth  = frame->width;  // будет переопределено ниже
            videoStreamInfo.scaleHeight = frame->height; //
            videoStreamInfo.fps         = _fps;
            videoStreamInfo.realFps     = frame->fps;

            if (_widthScale)
            {
                float scale = float(_widthScale) / frame->width;
                videoStreamInfo.scale = quint8(scale * 100);
                videoStreamInfo.scaleWidth  = frame->width  * scale;
                videoStreamInfo.scaleHeight = frame->height * scale;

                if (videoStreamInfo.scaleWidth > frame->width)
                {
                    videoStreamInfo.scaleWidth  = frame->width;
                    videoStreamInfo.scaleHeight = frame->height;
                }
            }
            else if (_frameScale != 100)
            {
                float scale = float(_frameScale) / 100;
                videoStreamInfo.scaleWidth  = frame->width  * scale;
                videoStreamInfo.scaleHeight = frame->height * scale;
            }

            if (!frame->data.empty())
                videoStreamInfo.cvType = frame->data.type();

            Message::Ptr m = createMessage(videoStreamInfo,
                                           {terminal->messageContentFormat});
            terminal->socket->send(m);
        };

        if (_terminalsChanged)
        {
            QMutexLocker locker {&_terminalsLock}; (void) locker;

            terminals.clear();
            for (Terminal* t : _terminals)
            {
                t->add_ref();
                terminals.add(t);
            }
            _terminalsChanged = false;
        }

        // Пустая data - признак начала нового видеопотока
        if (frame->data.empty())
        {
            for (Terminal* t : terminals)
            {
                t->isNew = false;
                sendVideoStreamInfo(t);
            }
            continue;
        }

        if (terminals.empty())
        {
            sleep(1);
            continue;
        }

        for (Terminal* t : terminals)
            if (t->isNew)
            {
                t->isNew = false;
                sendVideoStreamInfo(t);
            }

        // Начало отсчета времени отправки кадра
        sendTimer.reset();

        data::FrameHeader frameHeader;
        frameHeader.deviceIndex = _deviceIndex;
        frameHeader.frameNumber = frame->number;
        frameHeader.cvType      = frame->data.type();
        frameHeader.realWidth   = frame->width;
        frameHeader.realHeight  = frame->height;
        frameHeader.realStep    = frame->data.step;
        frameHeader.scale       = _frameScale;
        frameHeader.scaleWidth  = frame->width;     // будет переопределено ниже
        frameHeader.scaleHeight = frame->height;    //
        frameHeader.scaleStep   = frame->data.step; //
        frameHeader.fps         = _fps;
        frameHeader.realFps     = frame->fps;

        cv::Mat resizeFrame;

        if (_widthScale)
        {
            float scale = float(_widthScale) / frame->width;
            frameHeader.scale = quint8(scale * 100);
            frameHeader.scaleWidth  = frame->width  * scale;
            frameHeader.scaleHeight = frame->height * scale;

            if (frameHeader.scaleWidth > frame->width)
            {
                frameHeader.scaleWidth  = frame->width;
                frameHeader.scaleHeight = frame->height;
                resizeFrame = frame->data;
            }
            else
            {
                cv::Size size {frameHeader.scaleWidth, frameHeader.scaleHeight};
                cv::resize(frame->data, resizeFrame, size);
                frameHeader.scaleStep = resizeFrame.step;
            }
        }
        else if (_frameScale != 100)
        {
            float scale = float(_frameScale) / 100;
            frameHeader.scaleWidth  = frame->width  * scale;
            frameHeader.scaleHeight = frame->height * scale;

            cv::Size size {frameHeader.scaleWidth, frameHeader.scaleHeight};
            cv::resize(frame->data, resizeFrame, size);
            frameHeader.scaleStep = resizeFrame.step;
        }
        else
            resizeFrame = frame->data;

        QByteArray frameBuff;
        std::vector<uchar> jpegBuff;
        int dataSize = resizeFrame.rows * resizeFrame.step;
        frameHeader.compress = static_cast<quint8>(_compression);

        if (_compression == VideoFrameCompression::None)
        {
            frameBuff = QByteArray::fromRawData((char*)resizeFrame.data, dataSize);
        }
        else if (_compression == VideoFrameCompression::Zip)
        {
            frameBuff = qCompress(resizeFrame.data, dataSize);
            dataSize = frameBuff.size();
        }
        else // Jpeg-compression
        {
            std::vector<int> encodeParams;
            encodeParams.push_back(cv::IMWRITE_JPEG_QUALITY);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"

            switch (_compression)
            {
                case VideoFrameCompression::Jpeg50:
                {
                    encodeParams.push_back(50);
                    break;
                }
                case VideoFrameCompression::Jpeg60:
                {
                    encodeParams.push_back(60);
                    break;
                }
                case VideoFrameCompression::Jpeg70:
                {
                    encodeParams.push_back(70);
                    break;
                }
                case VideoFrameCompression::Jpeg80:
                {
                    encodeParams.push_back(80);
                    break;
                }
                case VideoFrameCompression::Jpeg90:
                {
                    encodeParams.push_back(90);
                    break;
                }
                case VideoFrameCompression::Jpeg100:
                {
                    encodeParams.push_back(100);
                    break;
                }
                default:
                {
                    log_error_m << log_format(
                        "Unsupported image compression format. Compress code: %?",
                        static_cast<int>(_compression));

                    frameHeader.compress = 0;
                    frameBuff.clear();
                    dataSize = 0;
                }
            }

#pragma GCC diagnostic pop

            if (frameHeader.compress != 0)
            {
                cv::imencode(".JPEG", resizeFrame, jpegBuff, encodeParams);
                frameBuff = QByteArray::fromRawData((char*)jpegBuff.data(),
                                                    (int)jpegBuff.size());
                dataSize = frameBuff.size();
            }
        }

        frameHeader.chunkCount = 0;
        frameHeader.chunkStep = 0;
        frameHeader.dataSize = (quint32)dataSize;

        for (Terminal* t : terminals)
        {
            QMutexLocker locker {&t->answerIdLock}; (void) locker;
            if (t->answerId.isNull() && t->socket->isConnected())
            {
                data::VideoSenderFrame videoSenderFrame;
                videoSenderFrame.frameHeader = frameHeader;

                // Отправляем кадр
                if (t->messageContentFormat == SerializeFormat::QBinary)
                {
                    videoSenderFrame.data = frameBuff;
                }
                else if (t->messageContentFormat == SerializeFormat::Json)
                {
                    videoSenderFrame.data = frameBuff.toBase64();
                }
                Message::Ptr m = createMessage(videoSenderFrame,
                                               {t->messageContentFormat});

                if (t->messageContentFormat == SerializeFormat::QBinary)
                    m->disableCompress();

                t->answerId = m->id();
                t->socket->send(m);
            }
        }

        int64_t elapsedTime = sendTimer.elapsed<std::chrono::microseconds>();
        int64_t sleepInterval = sendInterval - elapsedTime;
        if (sleepInterval > 0)
            usleep(sleepInterval);
            //this_thread::sleep_for(chrono::microseconds(sleepInterval));

    } // while (true)

    // Удаляем все терминалы
    removeTerminal();

    { //Block for alog::Line
        alog::Line logLine = log_info_m
                    << "Sender stopped"
                    << ". Device index: " << int(_deviceIndex)
                    << ". Fps: "          << int(_fps);

        if (_widthScale)
            logLine << ". Frame width: "  << int(_widthScale);
        else
            logLine << ". Frame scale: "  << int(_frameScale) << "%";

        logLine     << ". Compression: "  << toString(_compression);
    }
}

//-------------------------------- SenderPool --------------------------------

bool SendPool::init(pproto::transport::tcp::Listener* listener)
{
    if (listener)
        _listener = listener;
    else
        _listener = &pproto::transport::tcp::listener();

    return true;
}

void SendPool::deinit()
{
    for (Sender* sender : _senders)
        if (!sender->stop(5*1000 /*5 сек*/))
        {
            log_info_m << "Stop thread sender. Timeout expired"
                       << ", thread will be terminated";
            sender->terminate();
        }
}

void SendPool::message(const pproto::Message::Ptr& message)
{
    if (message->processed())
        return;

    if (message->command() == command::VideoSenderStart)
    {
        data::VideoSenderStart videoSenderStart;
        readFromMessage(message, videoSenderStart);

        bool found = false;
        for (Sender* sender : _senders)
        {
            if (sender->isFit(videoSenderStart.deviceIndex,
                              videoSenderStart.fps,
                              videoSenderStart.frameScale,
                              videoSenderStart.widthScale,
                              videoSenderStart.compression))
            {
                found = true;
                pproto::transport::base::Socket::Ptr socket =
                    _listener->socketByDescriptor(message->socketDescriptor());

                sender->addTerminal(socket, message->contentFormat());

                if (!sender->isRunning())
                    sender->start();
                break;
            }
        }
        if (!found)
        {
            Sender* sender {new Sender(this)};
            sender->init(videoSenderStart.deviceIndex,
                         videoSenderStart.fps,
                         videoSenderStart.frameScale,
                         videoSenderStart.widthScale,
                         videoSenderStart.compression);

            { //Block for alog::Line
                alog::Line logLine = log_verbose_m
                            << "Sender created"
                            << ". Device index: " << int(sender->_deviceIndex)
                            << ". Fps: "          << int(sender->_fps);

                if (sender->_widthScale)
                    logLine << ". Frame width: "  << int(sender->_widthScale);
                else
                    logLine << ". Frame scale: "  << int(sender->_frameScale) << "%";

                logLine     << ". Compression: "  << toString(sender->_compression);
            }

            pproto::transport::base::Socket::Ptr socket =
                _listener->socketByDescriptor(message->socketDescriptor());

            // TODO Разобраться почему socket иногда бывает NULL
            if (socket.empty())
            {
                SocketDescriptor socketDescr = message->socketDescriptor();
                (void) socketDescr;

                QVector<SocketDescriptor> descriptors;
                for (pproto::transport::base::Socket* s : _listener->sockets())
                    descriptors.append(s->socketDescriptor());

                (void) descriptors;
                break_point
            }

            sender->addTerminal(socket, message->contentFormat());
            sender->start();
            _senders.add(sender);
        }

        Message::Ptr answer = message->cloneForAnswer();
        _listener->send(answer);
    }
    else if (message->command() == command::VideoSenderStop)
    {
        data::VideoSenderStop videoSenderStop;
        readFromMessage(message, videoSenderStop);

        for (Sender* sender : _senders)
            if (sender->isFit(videoSenderStop.deviceIndex))
            {
                sender->removeTerminal(message->socketDescriptor());
                if (sender->terminalsIsEmpty())
                    if (!sender->stop(5*1000 /*5 сек*/))
                    {
                        log_info_m << "Stop thread sender. Timeout expired"
                                   << ", thread will be terminated";
                        sender->terminate();
                    }
            }

        Message::Ptr answer = message->cloneForAnswer();
        writeToMessage(videoSenderStop, answer, message->contentFormat());
        _listener->send(answer);
    }
    else if (message->command() == command::VideoSenderFrame
             && message->type() == Message::Type::Answer)
    {
        for (Sender* sender : _senders)
            if (sender->frameAnswerReceived(message->id()))
                break;
    }
}

void SendPool::socketConnected(pproto::SocketDescriptor)
{}

void SendPool::socketDisconnected(pproto::SocketDescriptor socketDescriptor)
{
    for (Sender* sender : _senders)
    {
        sender->removeTerminal(socketDescriptor);
        if (sender->terminalsIsEmpty())
            if (!sender->stop(5*1000 /*5 сек*/))
            {
                log_info_m << "Stop thread sender. Timeout expired"
                           << ", thread will be terminated";
                sender->terminate();
            }
    }
}

void SendPool::setFrame(const video::Frame::Ptr& frame)
{
    for (Sender* sender : _senders)
        sender->setFrame(frame);
}

bool SendPool::terminalsIsEmpty() const
{
    for (Sender* sender : _senders)
        if (!sender->terminalsIsEmpty())
            return false;

    return true;
}

SendPool& sendPool()
{
    return safe::singleton<SendPool, 0>();
}

SendPool& sendPool2()
{
    return safe::singleton<SendPool, 2>();
}

} // namespace video::transport::tcp
