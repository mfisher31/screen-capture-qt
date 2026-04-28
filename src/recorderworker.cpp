#include "recorderworker.hpp"

#include <QMutexLocker>

namespace sc {

RecorderWorker::RecorderWorker(const CaptureRegion& region,
                               const RecordingSettings& settings,
                               QObject* parent)
    : QObject(parent)
    , m_settings(settings)
    , m_region(region)
{}

RecorderWorker::~RecorderWorker() = default;

void RecorderWorker::setCaptureRegion(const CaptureRegion& region)
{
    QMutexLocker lock(&m_regionMutex);
    m_region = region;
}

CaptureRegion RecorderWorker::captureRegion() const
{
    QMutexLocker lock(&m_regionMutex);
    return m_region;
}

} // namespace sc
