#pragma once

#include "../appcontroller.hpp"

#include <QAudioDevice>
#include <QAudioInput>
#include <QElapsedTimer>
#include <QMediaCaptureSession>
#include <QMediaRecorder>
#include <QObject>
#include <QSize>
#include <QVideoFrameInput>

namespace sc {

// Encodes a stream of QImage frames to MP4 or WebM using Qt Multimedia.
// When RecordingSettings::captureAudio is true, microphone audio is recorded
// via QAudioInput and muxed automatically by QMediaRecorder.
//
// Audio sync note: video frames are stamped by QElapsedTimer; audio uses the
// system audio clock — both clocks start at roughly record-start, so sync is
// good for short clips. Proper PTS-aligned sync (plan Milestone 8) can be
// layered in later without changing this interface.
//
// Threading: lives on and must be used from the main thread. QMediaRecorder
// manages its own internal threads.
//
// Usage:
//   encoder.start(outputPath, frameSize);
//   encoder.sendFrame(image);   // called per captured frame
//   encoder.stop();             // asynchronous — encodingFinished fires when done
class VideoEncoder : public QObject {
    Q_OBJECT

public:
    explicit VideoEncoder(const RecordingSettings& settings,
                          QObject* parent = nullptr);

    // Begin recording. Returns false if the recorder cannot start.
    bool start(const QString& outputPath, QSize frameSize);

    // Feed one captured frame. Must call start() first.
    void sendFrame(const QImage& image);

    // Finalize the file. encodingFinished (or encodingFailed) is emitted
    // asynchronously once the recorder reaches StoppedState.
    void stop();

signals:
    void encodingFinished(QString outputPath);
    void encodingFailed(QString reason);

private:
    RecordingSettings    m_settings;
    QMediaCaptureSession m_session;
    QVideoFrameInput     m_input;
    QAudioInput          m_audioInput;   // null device = no audio
    QMediaRecorder       m_recorder;
    QElapsedTimer        m_elapsed;
    QString              m_outputPath;
};

} // namespace sc
