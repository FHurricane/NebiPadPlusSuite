#include "speechbackend.h"

#import <AVFoundation/AVFoundation.h>
#import <Speech/Speech.h>

#include <QLocale>
#include <QMetaObject>
#include <QPointer>

#include <atomic>
#include <cstring>
#include <memory>

namespace {
struct Session {
    QPointer<QObject> context;
    SpeechCallbacks callbacks;
    __strong SFSpeechRecognizer *recognizer = nil;
    __strong SFSpeechAudioBufferRecognitionRequest *request = nil;
    __strong SFSpeechRecognitionTask *task = nil;
    __strong AVAudioFormat *format = nil;
    std::atomic<bool> cancelled{false};
    std::atomic<bool> delivered{false};
};

void onMain(const std::shared_ptr<Session> &session, std::function<void()> callback)
{
    const auto retainedSession = session;
    QObject *context = retainedSession->context.data();
    if (!context) return;
    QMetaObject::invokeMethod(context,
        [retainedSession, callback = std::move(callback)] {
            if (!retainedSession->cancelled.load() && retainedSession->context)
                callback();
        }, Qt::QueuedConnection);
}

QString errorText(NSError *error)
{
    return error ? QString::fromNSString(error.localizedDescription) : QString();
}

class MacSpeechBackend final : public SpeechBackend
{
public:
    explicit MacSpeechBackend(QObject *context) : context_(context) {}
    ~MacSpeechBackend() override { cancel(); }

    void start(SpeechCallbacks callbacks) override
    {
        cancel();
        auto session = std::make_shared<Session>();
        session->context = context_;
        session->callbacks = std::move(callbacks);
        session_ = session;
        const QString localeName = QLocale::system().name().replace('_', '-');
        NSString *identifier = QString(localeName).toNSString();
        session->recognizer = [[SFSpeechRecognizer alloc]
            initWithLocale:[[NSLocale alloc] initWithLocaleIdentifier:identifier]];
        if (!session->recognizer) {
            if (session->callbacks.error)
                session->callbacks.error(QStringLiteral("La lingua del sistema non è supportata dalla dettatura."));
            return;
        }
        [SFSpeechRecognizer requestAuthorization:^(SFSpeechRecognizerAuthorizationStatus status) {
            if (status != SFSpeechRecognizerAuthorizationStatusAuthorized) {
                onMain(session, [session] {
                    if (session->callbacks.error)
                        session->callbacks.error(QStringLiteral("Autorizza NebiPad+ al riconoscimento vocale nelle impostazioni di macOS."));
                });
                return;
            }
            [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted) {
                onMain(session, [session, granted] {
                    if (!granted) {
                        if (session->callbacks.error)
                            session->callbacks.error(QStringLiteral("Autorizza NebiPad+ a usare il microfono nelle impostazioni di macOS."));
                        return;
                    }
                    if (!session->recognizer.available) {
                        if (session->callbacks.error)
                            session->callbacks.error(QStringLiteral("Il riconoscimento vocale non è disponibile per la lingua corrente."));
                        return;
                    }
                    session->format = [[AVAudioFormat alloc]
                        initWithCommonFormat:AVAudioPCMFormatInt16 sampleRate:16000
                        channels:1 interleaved:YES];
                    session->request = [[SFSpeechAudioBufferRecognitionRequest alloc] init];
                    session->request.shouldReportPartialResults = YES;
                    session->request.taskHint = SFSpeechRecognitionTaskHintDictation;
                    session->task = [session->recognizer recognitionTaskWithRequest:session->request
                        resultHandler:^(SFSpeechRecognitionResult *result, NSError *error) {
                        if (session->cancelled.load()) return;
                        if (result) {
                            const QString text = QString::fromNSString(
                                result.bestTranscription.formattedString);
                            if (result.isFinal) {
                                if (!session->delivered.exchange(true))
                                    onMain(session, [session, text] {
                                        if (session->callbacks.finalText) session->callbacks.finalText(text);
                                    });
                            } else {
                                onMain(session, [session, text] {
                                    if (session->callbacks.partial) session->callbacks.partial(text);
                                });
                            }
                        }
                        if (error && !session->delivered.exchange(true)) {
                            const QString message = errorText(error);
                            onMain(session, [session, message] {
                                if (session->callbacks.error)
                                    session->callbacks.error(QStringLiteral("Dettatura non riuscita: %1").arg(message));
                            });
                        }
                    }];
                    if (session->callbacks.ready) session->callbacks.ready();
                });
            }];
        }];
    }

    void appendAudio(const QByteArray &pcm16) override
    {
        const auto session = session_;
        if (!session || session->cancelled.load() || !session->request || pcm16.size() < 2) return;
        @autoreleasepool {
            const AVAudioFrameCount frames = AVAudioFrameCount(pcm16.size() / 2);
            AVAudioPCMBuffer *buffer = [[AVAudioPCMBuffer alloc]
                initWithPCMFormat:session->format frameCapacity:frames];
            if (!buffer) return;
            buffer.frameLength = frames;
            std::memcpy(buffer.audioBufferList->mBuffers[0].mData, pcm16.constData(), frames * 2);
            [session->request appendAudioPCMBuffer:buffer];
        }
    }

    void finish() override
    {
        if (session_ && session_->request && !session_->cancelled.load())
            [session_->request endAudio];
    }

    void cancel() override
    {
        if (!session_) return;
        session_->cancelled = true;
        if (session_->task) [session_->task cancel];
        session_.reset();
    }

private:
    QObject *context_;
    std::shared_ptr<Session> session_;
};
}

std::unique_ptr<SpeechBackend> createSpeechBackend(QObject *context)
{
    return std::make_unique<MacSpeechBackend>(context);
}
