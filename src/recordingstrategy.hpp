#pragma once

#include "appcontroller.hpp"

#include <QImage>
#include <QObject>
#include <QString>

namespace sc {

// Abstract base for recording strategies.
//
// A strategy owns the data-flow between raw captured frames and the final
// encoded output file. AppController selects the concrete strategy at
// record-start time based on OutputFormat; nothing else in the codebase
// needs to know which strategy is active.
//
// Threading contract:
//   onFrame() is called on the MAIN thread (Qt::QueuedConnection from worker).
//   finish()  is called on the MAIN thread when recordingFinished() arrives.
//   Signals are emitted on the thread that owns the strategy (main thread).
//   Subclasses may spawn their own encoder threads internally.
class RecordingStrategy : public QObject {
    Q_OBJECT

public:
    explicit RecordingStrategy(const RecordingSettings& settings,
                               QObject* parent = nullptr)
        : QObject(parent), m_settings(settings) {}

    // Called for every frame that passed the FPS throttle.
    // image is already cropped to the capture region.
    // region carries the per-frame screen + rect snapshot.
    virtual void onFrame(const QImage& image, const CaptureRegion& region) = 0;

    // Called once after the capture worker has stopped.
    // The strategy should begin or finalize encoding here.
    virtual void finish() = 0;

signals:
    void encodingProgress(float fraction);        // 0.0 – 1.0
    void encodingFinished(QString outputPath);
    void encodingFailed(QString reason);

protected:
    RecordingSettings m_settings;
};

} // namespace sc
