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

#include "sender.h"

#include "shared/break_point.h"
#include "shared/prog_abort.h"
#include "shared/spin_locker.h"
#include "shared/logger/logger.h"
#include "shared/logger/format.h"
#include "shared/network/logger_operators.h"
#include "shared/qt/logger_operators.h"
#include "pproto/transport/tcp.h"

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

namespace video::transport {

using namespace std;
using namespace pproto::transport;

bool Sender::init(quint8 deviceIndex, quint8 fps, quint8 frameScale,
                  quint16 chunkStep, VideoFrameCompression compression)
{
    _deviceIndex = deviceIndex;
    _fps = fps;
    _frameScale = frameScale;
    _chunkStep = chunkStep;
    _compression = compression;

    return true;
}

bool Sender::isFit(quint8 deviceIndex) const
{
    return _deviceIndex == deviceIndex;
}

bool Sender::isFit(quint8 deviceIndex, quint8 fps, quint8 frameScale,
                   quint16 chunkStep, VideoFrameCompression compression) const
{
    return _deviceIndex == deviceIndex
            && _fps == fps
            && _frameScale == frameScale
            && _chunkStep == chunkStep
            && _compression == compression;
}

void Sender::addTerminal(SocketDescriptor socketDescriptor, quint64 terminalId,
                         const HostPoint& remotePoint)
{
    QMutexLocker locker {&_terminalsLock}; (void) locker;

    for (Terminal* t : _terminals)
        if (t->id == terminalId)
            return;

    _newTerminalDescriptors.insert(socketDescriptor);

    Terminal* t {new Terminal};
    t->socketDescriptor = socketDescriptor;
    t->id = terminalId;

    t->remoteAddr.sin_family = AF_INET;
    t->remoteAddr.sin_port = htons(remotePoint.port());
    t->remoteAddr.sin_addr.s_addr = htonl(remotePoint.address().toIPv4Address());

    t->add_ref();
    _terminals.add(t);
    _terminalsChanged = true;

    log_verbose_m << "Terminal added. Udp host: " << t->remoteAddr;
}

void Sender::removeTerminal(quint64 terminalId)
{
    QMutexLocker locker {&_terminalsLock}; (void) locker;

    _terminals.removeCond([terminalId, this](Terminal* t) {
        if (t->id == terminalId)
        {
            log_verbose_m << "Terminal removed. Udp host: " << t->remoteAddr;
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

void Sender::removeTerminalBySocket(SocketDescriptor socketDescriptor)
{
    QMutexLocker locker {&_terminalsLock}; (void) locker;

    _terminals.removeCond([socketDescriptor, this](Terminal* t) {
        if (t->socketDescriptor == socketDescriptor)
        {
            log_verbose_m << "Terminal removed. Udp host: " << t->remoteAddr;
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

void Sender::addFeedback(const FrameFeedback::Ptr& feedback)
{
    QMutexLocker locker {&_feedbacksLock}; (void) locker;

    feedback->add_ref();
    _feedbacks.add(feedback);
    _feedbacksCond.wakeAll();
}

void Sender::run()
{
    log_info_m << "Sender started"
               << ". Device index: " << int(_deviceIndex)
               << ". Fps: " << int(_fps)
               << ". Frame scale: " << int(_frameScale) << "%";

    if (_socket != -1)
        close(_socket);

    _socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (_socket == -1)
    {
        log_error_m << "Can't create socket";
        return;
    }

    // Временной интервал отправки кадров (в микросекундах)
    const double sendInterval = 1000000. / _fps * 0.98;

    { //Block for QMutexLocker
        QMutexLocker locker {&_terminalsLock}; (void) locker;
        _terminalsChanged = true;
    }

    steady_timer sendTimer;
    Terminal::List terminals;

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

        auto sendVideoStreamInfo = [this, &frame]
                                   (const SocketDescriptorSet& descriptors)
        {
            if (descriptors.isEmpty())
                return;

            data::VideoStreamInfo videoStreamInfo;
            videoStreamInfo.deviceIndex = _deviceIndex;
            videoStreamInfo.frameNumber = frame->number;
            videoStreamInfo.realWidth   = frame->width;
            videoStreamInfo.realHeight  = frame->height;
            videoStreamInfo.scale       = _frameScale;
            videoStreamInfo.scaleWidth  = frame->width;
            videoStreamInfo.scaleHeight = frame->height;
            videoStreamInfo.fps         = _fps;
            videoStreamInfo.realFps     = frame->fps;

            if (_frameScale != 100)
            {
                float scale = float(_frameScale) / 100;
                videoStreamInfo.scaleWidth  = frame->width  * scale;
                videoStreamInfo.scaleHeight = frame->height * scale;
            }

            if (!frame->data.empty())
                videoStreamInfo.cvType = frame->data.type();

            Message::Ptr m = createMessage(videoStreamInfo);
            m->destinationSockets() = descriptors;
            tcp::listener().send(m);
        };

        // Пустая data - признак начала нового видеопотока
        if (frame->data.empty())
        {
            SocketDescriptorSet descriptors;
            for (Terminal* t : _terminals)
                descriptors.insert(t->socketDescriptor);

            sendVideoStreamInfo(descriptors);
            continue;
        }

        if (_terminalsChanged)
        {
            QMutexLocker locker {&_terminalsLock}; (void) locker;

            terminals.clear();
            for (Terminal* t : _terminals)
            {
                t->add_ref();
                terminals.add(t);
            }
            if (!_newTerminalDescriptors.isEmpty())
            {
                sendVideoStreamInfo(_newTerminalDescriptors);
                _newTerminalDescriptors.clear();
            }
            _terminalsChanged = false;
        }

        if (terminals.empty())
        {
            sleep(1);
            continue;
        }

        { //Block for QMutexLocker
            QMutexLocker locker {&_feedbacksLock}; (void) locker;
            _feedbacks.clear();
        }

        // Начало отсчета времени отправки кадра
        sendTimer.reset();

        char sockBuff[65536];
        int  writeBuffSize;

        FrameHeader frameHeader;
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
        if (_frameScale != 100)
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

        if (_compression != VideoFrameCompression::None)
        {
            frameHeader.compress = static_cast<quint8>(_compression);
            if (_compression == VideoFrameCompression::Zip)
            {
                frameBuff = qCompress(resizeFrame.data, dataSize);
            }
            else
            {
                std::vector<int> encodeParams;
                encodeParams.push_back(cv::IMWRITE_JPEG_QUALITY);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"

                switch (_compression)
                {
                    case VideoFrameCompression::Jpeg90:
                    {
                        encodeParams.push_back(90);
                        break;
                    }
                    case VideoFrameCompression::Jpeg80:
                    {
                        encodeParams.push_back(80);
                        break;
                    }
                    case VideoFrameCompression::Jpeg70:
                    {
                        encodeParams.push_back(70);
                        break;
                    }
                    case VideoFrameCompression::Jpeg60:
                    {
                        encodeParams.push_back(60);
                        break;
                    }
                    case VideoFrameCompression::Jpeg50:
                    {
                        encodeParams.push_back(50);
                        break;
                    }
                    default:
                        log_error_m << "Unsupported compression algorithm";
                        frameHeader.compress = 0;
                }

#pragma GCC diagnostic pop

                cv::imencode(".JPEG", resizeFrame, jpegBuff, encodeParams);
                frameBuff = QByteArray::fromRawData((char*)jpegBuff.data(),
                                                    (int)jpegBuff.size());
            }

#ifdef VIDEO_TRANSPORT_DEBUG
            log_debug2_m << "Compress data: " << dataSize << " -> " << frameBuff.size()
                         << ". Compression: " << toString(_compression);
#endif
            dataSize = frameBuff.size();
        }
        if (frameHeader.compress == 0)
            frameBuff = QByteArray::fromRawData((char*)resizeFrame.data, dataSize);

        char* frameBuffBegin = (char*)frameBuff.constData();
        char* frameBuffEnd = frameBuffBegin + dataSize;

        frameHeader.chunkCount = dataSize / _chunkStep;
        if (dataSize % _chunkStep != 0)
            frameHeader.chunkCount += 1;

        frameHeader.chunkStep = _chunkStep;
        frameHeader.dataSize  = (quint32)dataSize;

        auto sendToSocket = [this](char* buff, int writeSize, const struct sockaddr_in& addr)
        {
//#ifdef VIDEO_TRANSPORT_DEBUG
//            log_debug2_m << "Send datagramm to host: " << addr;
//#endif
            if (-1 == sendto(_socket, buff, writeSize, 0,
                             (struct sockaddr*) &addr, sizeof(addr)))
            {
                log_error_m << "Can't send datagramm to socket (errno " << SND_ERRNO << ")"
                            << ". Host: " << addr;
            }
        };

        // Рассылка заголовка
        writeBuffSize = frameHeader.writeToBuffer(sockBuff);
        for (Terminal* t : terminals)
            sendToSocket(sockBuff, writeBuffSize, t->remoteAddr);

        // Рассылка кусков
        for (int i = 0; i < int(frameHeader.chunkCount); ++i)
        {
            FrameItem frameItem;
            frameItem.deviceIndex = _deviceIndex;
            frameItem.frameNumber = frame->number;
            frameItem.dataSize    = dataSize;
            frameItem.chunkNumber = i;
            frameItem.chunkStep   = frameHeader.chunkStep;
            frameItem.chunkSize   = frameHeader.chunkStep;

            char* chunkBuffBegin = frameBuffBegin + (i * frameHeader.chunkStep);
            char* chunkBuffEnd   = chunkBuffBegin + frameHeader.chunkStep;

            if (chunkBuffEnd > frameBuffEnd)
                frameItem.chunkSize = quint16(frameBuffEnd - chunkBuffBegin);

            writeBuffSize = frameItem.writeToBuffer(sockBuff, chunkBuffBegin);
            for (Terminal* t : terminals)
                sendToSocket(sockBuff, writeBuffSize, t->remoteAddr);

            if (i % 5 == 0)
                usleep(10);

            // Для подстраховки повторяем рассылку заголовков
            if ((i == 5) || (i == 10))
            {
                writeBuffSize = frameHeader.writeToBuffer(sockBuff);
                for (Terminal* t : terminals)
                    sendToSocket(sockBuff, writeBuffSize, t->remoteAddr);
            }
        }

        // Финиширующий фрейм
        FrameFinish frameFinish;
        frameFinish.senderId    = _id;
        frameFinish.deviceIndex = _deviceIndex;
        frameFinish.frameNumber = frame->number;
        frameFinish.status      = FrameFinish::Status::Success;

        writeBuffSize = frameFinish.writeToBuffer(sockBuff);
        for (Terminal* t : terminals)
            sendToSocket(sockBuff, writeBuffSize, t->remoteAddr);

#ifdef VIDEO_TRANSPORT_DEBUG
        log_debug2_m << "Sent frame: " << frame->number
                     << ". Time: " << sendTimer.elapsed<chrono::microseconds>();
#endif
        for (Terminal* t : terminals)
        {
            t->frameReceived = false;
            t->resendFinishTimer.reset();
            t->lostChunks = 0;
        }

        // Досылаем потерянные куски данных пока не истекло время на обработку кадра
        while (sendTimer.elapsed<chrono::microseconds>() < sendInterval)
        {
            if (threadStop())
                break;

            // Признак, что кадр получен всеми терминалами
            bool frameReceivedAll = true;
            for (Terminal* t : terminals)
                if (!t->frameReceived)
                {
                    frameReceivedAll = false;
                    break;
                }

            if (frameReceivedAll)
            {
#ifdef VIDEO_TRANSPORT_DEBUG
                log_debug2_m << "Sleep (frameReceivedAll). frame: " << frame->number
                             << ". Time: " << sendTimer.elapsed<chrono::microseconds>();
#endif
                // Если все разослали, то просто спим до наступления следующего
                // временного интервала
                usleep(sendInterval - sendTimer.elapsed<chrono::microseconds>());

#ifdef VIDEO_TRANSPORT_DEBUG
                log_debug2_m << "Awakening (frameReceivedAll). frame: " << frame->number
                             << ". Time: " << sendTimer.elapsed<chrono::microseconds>();
#endif
                break;
            }

            if (sendTimer.elapsed<chrono::microseconds>() > (sendInterval - 500))
            {
                // Финиширующий фрейм со статусом Timeout
                frameFinish.status = FrameFinish::Status::Timeout;
                writeBuffSize = frameFinish.writeToBuffer(sockBuff);

                // Отправляем 3 раза
                for (int i = 0; i < 3; ++i)
                {
                    for (Terminal* t : terminals)
                        if (!t->frameReceived)
                            sendToSocket(sockBuff, writeBuffSize, t->remoteAddr);
                    usleep(20);
                }

                // Отправка статистики
                for (Terminal* t : terminals)
                    if (!t->frameReceived)
                    {
                        LostStat::Ptr ls {new LostStat};
                        ls->terminalId = t->id;
                        ls->terminalAddr = t->remoteAddr;
                        ls->deviceIndex = _deviceIndex;
                        ls->fps = _fps;
                        ls->frameScale = _frameScale;
                        ls->lostChunks = t->lostChunks;
                        ls->totalChunks = frameHeader.chunkCount;
                        _sendPool->addLostStat(ls);
                    }

#ifdef VIDEO_TRANSPORT_DEBUG
                log_debug2_m << "Send FrameFinish::Status::Timeout. frame: " << frame->number
                             << " sendTimer: " << sendTimer.elapsed<chrono::microseconds>()
                             << " sendInterval-500: " << (sendInterval - 500);
#endif
                break;
            }

            FrameFeedback::List feedbacks;
            { //Block for QMutexLocker
                QMutexLocker locker {&_feedbacksLock}; (void) locker;
                if (_feedbacks.empty())
                    _frameCond.wait(&_feedbacksLock, 2);
                feedbacks.swap(_feedbacks);
            }

            feedbacks.removeCond([this, frame](FrameFeedback* f) {
                return (_id != f->senderId) || (frame->number != f->frameNumber);
            });

            if (feedbacks.empty())
            {
                for (Terminal* t : terminals)
                    if (!t->frameReceived
                        && t->resendFinishTimer.elapsed<chrono::microseconds>() > 2000)
                    {
                        // Повторно отправляем финиширующий фрейм терминалам,
                        // которые еще не подтвердили получение всего кадра
                        writeBuffSize = frameFinish.writeToBuffer(sockBuff);
                        sendToSocket(sockBuff, writeBuffSize, t->remoteAddr);

#ifdef VIDEO_TRANSPORT_DEBUG
                        log_debug2_m << "Resend FrameFinish. frameNumber: " << frame->number
                                     << ". Time: " << sendTimer.elapsed<chrono::microseconds>();
#endif
                        t->resendFinishTimer.reset();
                    }
                continue;
            }

            for (FrameFeedback* f : feedbacks)
                for (Terminal* t : terminals)
                    if (f->terminalId == t->id)
                    {
                        if (t->frameReceived)
                            break;

                        if (f->status == FrameFeedback::Status::Success)
                        {
                            t->frameReceived = true;

                            // Отправка статистики
                            LostStat::Ptr ls {new LostStat};
                            ls->terminalId = t->id;
                            ls->terminalAddr = t->remoteAddr;
                            ls->deviceIndex = _deviceIndex;
                            ls->fps = _fps;
                            ls->frameScale = _frameScale;
                            ls->lostChunks = t->lostChunks;
                            ls->totalChunks = frameHeader.chunkCount;
                            _sendPool->addLostStat(ls);

#ifdef VIDEO_TRANSPORT_DEBUG
                            log_debug2_m << "FrameReceived. frameNumber: " << f->frameNumber
                                         << ". Time: " << sendTimer.elapsed<chrono::microseconds>();
#endif
                            break;
                        }

                        if (f->status == FrameFeedback::Status::RepeateHead)
                        {
                            // Отправляем заголовок
                            writeBuffSize = frameHeader.writeToBuffer(sockBuff);
                            sendToSocket(sockBuff, writeBuffSize, t->remoteAddr);

                            // Отправляем финиширующий фрейм
                            writeBuffSize = frameFinish.writeToBuffer(sockBuff);
                            sendToSocket(sockBuff, writeBuffSize, t->remoteAddr);

#ifdef VIDEO_TRANSPORT_DEBUG
                            log_debug2_m << "Send (RepeateHead). frameNumber: " << frame->number
                                         << ". Time: " << sendTimer.elapsed<chrono::microseconds>();
#endif
                            t->resendFinishTimer.reset();
                            break;
                        }

                        if (f->status == FrameFeedback::Status::RepeatItem)
                        {
                            // Подсчет потерянных кусков
                            t->lostChunks += f->chunkNumbers.count();

                            // Повторная отправка потерянных кусков
                            for (int i = 0; i < f->chunkNumbers.count(); ++i)
                            {
                                quint16 chunkNum = f->chunkNumbers[i];

                                FrameItem frameItem;
                                frameItem.deviceIndex = _deviceIndex;
                                frameItem.frameNumber = frame->number;
                                frameItem.chunkNumber = chunkNum;
                                frameItem.chunkStep   = frameHeader.chunkStep;
                                frameItem.chunkSize   = frameHeader.chunkStep;

                                char* chunkBuffBegin = frameBuffBegin + (chunkNum * frameHeader.chunkStep);
                                char* chunkBuffEnd   = chunkBuffBegin + frameHeader.chunkStep;

                                if (chunkBuffEnd > frameBuffEnd)
                                    frameItem.chunkSize = quint16(frameBuffEnd - chunkBuffBegin);

                                writeBuffSize = frameItem.writeToBuffer(sockBuff, chunkBuffBegin);
                                sendToSocket(sockBuff, writeBuffSize, t->remoteAddr);

                                if (i % 5 == 0)
                                    usleep(10);
                            }

                            // Отправляем финиширующий фрейм
                            writeBuffSize = frameFinish.writeToBuffer(sockBuff);
                            sendToSocket(sockBuff, writeBuffSize, t->remoteAddr);

#ifdef VIDEO_TRANSPORT_DEBUG
                            log_debug2_m << "Send (RepeatItem). frameNumber: " << frame->number
                                         << ". Time: " << sendTimer.elapsed<chrono::microseconds>();
#endif
                            t->resendFinishTimer.reset();
                            break;
                        }
                    } // if (f->terminalId == t->id)

        } // while (sendTimer.elapsed<chrono::microseconds>() < sendInterval)

#ifdef VIDEO_TRANSPORT_DEBUG
        log_debug2_m << "End sendTimer interval"
                     << " sendTimer: " << sendTimer.elapsed<chrono::microseconds>()
                     << " sendInterval: " << sendInterval
                     << " frameNumber: " << frame->number;
#endif
    } // while (true)

    close(_socket);
    _socket = -1;

    log_info_m << "Sender stopped"
               << ". Device index: " << int(_deviceIndex)
               << ". Fps: " << int(_fps)
               << ". Frame scale: " << int(_frameScale) << "%";
}

//-------------------------------- SenderPool --------------------------------

bool SendPool::init(int feedbackPort)
{
    if (isRunning())
    {
        log_error_m << "Impossible execute a initialization "
                       "because SendPool thread is running";
        return false;
    }

    if ((feedbackPort < 1) || (feedbackPort > 65535))
    {
        log_error_m << "A feedback port must be in interval 1 - 65535";
        return false;
    }
    _feedbackPort = quint16(feedbackPort);

    _feedbackSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (_feedbackSocket == -1)
    {
        log_error_m << "Can't create feedback socket";
        return false;
    }

    int on = 1;
    if (-1 == setsockopt(_feedbackSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
    {
        log_error_m << "Can't set option SO_REUSEADDR to feedback socket";
        close(_feedbackSocket);
        return false;
    }

    sockaddr_in bindAddr;
    memset(&bindAddr, 0, sizeof(bindAddr));

    bindAddr.sin_port = htons(_feedbackPort);
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int bindAttempts = 3;
    while (-1 == ::bind(_feedbackSocket, (sockaddr*) &bindAddr, sizeof(bindAddr)))
    {
        log_error_m << "Can't bind feedback socket with address " << bindAddr
                    << " (errno " << SND_ERRNO << ")";

        if (--bindAttempts < 0)
        {
            close(_feedbackSocket);
            return false;
        }
        sleep(1);
    }

    log_info_m << "Feedback socket successfully bound with address " << bindAddr;
    return true;
}

void SendPool::message(const pproto::Message::Ptr& message)
{
    if (message->processed())
        return;

    if (threadStop())
        return;

    if (message->command() == command::VideoSenderStart)
    {
        data::VideoSenderStart videoSenderStart;
        readFromMessage(message, videoSenderStart);

        HostPoint recvPoint {message->sourcePoint().address(),
                             videoSenderStart.sendPort};
#ifdef VIDEO_TRANSPORT_DEBUG
        //recvPoint.setAddress(QHostAddress("127.0.0.1"));
        log_debug2_m << "Terminal receive point: " << recvPoint;
#endif
        QMutexLocker locker {&_threadLock}; (void) locker;

        bool found = false;
        for (Sender* sender : _senders)
        {
            if (sender->isFit(videoSenderStart.deviceIndex,
                              videoSenderStart.fps,
                              videoSenderStart.frameScale,
                              videoSenderStart.chunkStep,
                              videoSenderStart.compression))
            {
                found = true;
                sender->addTerminal(message->socketDescriptor(),
                                    videoSenderStart.terminalId, recvPoint);
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
                         videoSenderStart.chunkStep,
                         videoSenderStart.compression);

            log_verbose_m << "Sender created"
                          << ". Device index: " << int(sender->_deviceIndex)
                          << ". Fps: "          << int(sender->_fps)
                          << ". Frame scale: "  << int(sender->_frameScale) << "%"
                          << ". Chunk step: "   << int(sender->_chunkStep)
                          << ". Compression: "  << toString(sender->_compression);

            sender->addTerminal(message->socketDescriptor(),
                                videoSenderStart.terminalId, recvPoint);
            sender->start();
            _senders.add(sender);
        }

        Message::Ptr answer = message->cloneForAnswer();
        tcp::listener().send(answer);
    }
    else if (message->command() == command::VideoSenderStop)
    {
        data::VideoSenderStop videoSenderStop;
        readFromMessage(message, videoSenderStop);

        QMutexLocker locker {&_threadLock}; (void) locker;

        for (Sender* sender : _senders)
            if (sender->isFit(videoSenderStop.deviceIndex))
            {
                sender->removeTerminal(videoSenderStop.terminalId);
                if (sender->terminalsIsEmpty())
                    if (!sender->stop(5*1000 /*5 сек*/))
                    {
                        log_info_m << "Stop thread sender. Timeout expired"
                                   << ", thread will be terminated";
                        sender->terminate();
                    }
            }

        Message::Ptr answer = message->cloneForAnswer();
        writeToMessage(videoSenderStop, answer);
        tcp::listener().send(answer);
    }
    else if (message->command() == command::VideoFeedbackPort)
    {
        Message::Ptr answer = message->cloneForAnswer();
        answer->setTag(_feedbackPort);
        tcp::listener().send(answer);
    }
}

void SendPool::socketConnected(pproto::SocketDescriptor)
{}

void SendPool::socketDisconnected(pproto::SocketDescriptor socketDescriptor)
{
    QMutexLocker locker {&_threadLock}; (void) locker;

    for (Sender* sender : _senders)
    {
        sender->removeTerminalBySocket(socketDescriptor);
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
    if (threadStop())
        return;

    QMutexLocker locker {&_threadLock}; (void) locker;

    for (Sender* sender : _senders)
        sender->setFrame(frame);
}

void SendPool::run()
{
    log_info_m << "SendPool started";

    char buff[8192];
    sockaddr_in remoteAddr;
    socklen_t remoteAddrLen = sizeof(remoteAddr);
    memset(&remoteAddr, 0, remoteAddrLen);

    int ret;
    pollfd fds[1];
    fds[0].fd = _feedbackSocket;
    fds[0].events = POLLIN;

    steady_timer lostStatTimer;
    LostStat::List lostStatList;

    while (true)
    {
        CHECK_QTHREADEX_STOP

        if (lostStatTimer.elapsed<chrono::seconds>() > 10)
        {
            { //Block for SpinLocker
                SpinLocker locker {_lostStatLock}; (void) locker;
                lostStatList.swap(_lostStatList);
            }
            for (LostStat* ls : lostStatList)
            {
                int losses = (float(ls->lostChunks) / ls->totalChunks) * 100;
                if (losses > 10)
                    log_warn_m << "Data loss " << losses << "% during frames sending"
                               << ". Terminal addr: " << ls->terminalAddr
                               << ". Device index: " << int(ls->deviceIndex)
                               << ". Fps: " << int(ls->fps)
                               << ". Frame scale: " << int(ls->frameScale) << "%";
                              //<< ". Possibly increasing system UDP buffer can reduce loss";
            }
            lostStatList.clear();
            lostStatTimer.reset();
        }

        // Ждем данных в сокете (таймаут 10 миллисекунд)
        ret = poll(fds, 1, 10);

        if (0 == ret)
        {
            // В эту секцию попадаем если интервал ожидания функции poll()
            // истек и не было получено новых пакетов.
            continue;
        }

        if (-1 == ret)
        {
            log_error_m << "Error waiting of socket (errno " << SND_ERRNO << ")";
            sleep(1);
            continue;
        }

        ret = recvfrom(_feedbackSocket, buff, sizeof(buff), 0,
                       (struct sockaddr*) &remoteAddr, &remoteAddrLen);
        if (-1 == ret)
        {
            log_error_m << "Can't read from socket (errno " << SND_ERRNO << ")"
                        << ". Remote address " << remoteAddr;
            sleep(1);
            continue;
        }
        if (ret >= ssize_t(sizeof(buff)))
        {
            log_error_m << "Can't handle packet. Datagram too big"
                        << ". Remote address " << remoteAddr;
            continue;
        }

        char* buffCur = buff;
        if (!checkUdpSignature(buffCur))
        {
            log_error_m << "Can't handle packet. Incompatible signature"
                        << ". Remote address " << remoteAddr;
            continue;
        }
        FrameFeedback::Ptr feedback = FrameFeedback::readFromBuffer(buffCur);

        { //Block for QMutexLocker
            QMutexLocker locker {&_threadLock}; (void) locker;
            for (Sender* sender : _senders)
                sender->addFeedback(feedback);
        }
    } // while (true)

    close(_feedbackSocket);
    _feedbackSocket = -1;

    { //Block for QMutexLocker
        QMutexLocker locker {&_threadLock}; (void) locker;
        for (Sender* sender : _senders)
            if (!sender->stop(5*1000 /*5 сек*/))
            {
                log_info_m << "Stop thread sender. Timeout expired"
                           << ", thread will be terminated";
                sender->terminate();
            }
    }
    log_info_m << "SendPool stopped";
}

void SendPool::addLostStat(const LostStat::Ptr& ls)
{
    SpinLocker locker {_lostStatLock}; (void) locker;

    bool found = false;
    for (LostStat* l : _lostStatList)
        if (l->terminalId == ls->terminalId
            && l->deviceIndex == ls->deviceIndex
            && l->fps == ls->fps
            && l->frameScale == ls->frameScale)
        {
            l->lostChunks  += ls->lostChunks;
            l->totalChunks += ls->totalChunks;
            found = true;
            break;
        }

    if (!found)
    {
        ls->add_ref();
        _lostStatList.add(ls.get());
    }
}

SendPool& sendPool()
{
    return safe::singleton<SendPool>();
}

} // namespace video::transport
