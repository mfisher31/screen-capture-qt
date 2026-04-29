#pragma once

#include "recordingstrategy.hpp"
#include "capture/framestore.hpp"

#include <QThread>

namespace sc {

class GifEncoder;

// Buffered recording strategy for GIF output.
//
// Accumulates all captured frames as QImage in FrameStore during recording,
// then runs GifEncoder on a dedicated thread once finish() is called.
//
// RAM budget: ~800×450×4 bytes × 30fps × 30s ≈ 1.2 GB.
// This strategy is appropriate for short recordings (< ~30s). For longer
// recordings, StreamingStrategy (Phase 2) writes incrementally to disk.
class BufferedStrategy : public RecordingStrategy {
    Q_OBJECT

public:
    explicit BufferedStrategy(const RecordingSettings& settings,
                              QObject* parent = nullptr);
    ~BufferedStrategy() override;

    void onFrame(const QImage& image, const CaptureRegion& region) override;
    void finish() override;

private:
    FrameStore* m_frameStore    = nullptr;
    QThread*    m_encoderThread = nullptr;
};

} // namespace sc
