#include <algorithm>
#include <cmath>

#include "util_frame_governor.h"

namespace dxvk {

  void FrameGovernor::setOptions(
    const FrameGovernorOptions& options) {
    std::lock_guard<dxvk::mutex> lock(m_mutex);

    m_options = options;
    m_options.targetPercent = std::max(m_options.targetPercent, 100u);
    m_stats.drawBudget = m_options.drawBudget;

    if (!m_options.enabled) {
      m_pendingEarlyDelay = Duration::zero();
      m_recoveryFramesLeft = 0u;
    }

    m_stats.mode = getModeLocked();
  }


  void FrameGovernor::setTargetFrameRate(
          double                frameRate) {
    std::lock_guard<dxvk::mutex> lock(m_mutex);

    double target = std::abs(frameRate);

    m_targetInterval = target != 0.0
      ? Duration(int64_t(double(Duration::period::den) / target))
      : Duration::zero();

    if (m_targetInterval == Duration::zero())
      m_pendingEarlyDelay = Duration::zero();

    m_stats.mode = getModeLocked();
  }


  void FrameGovernor::beginFrame(
          uint64_t              frameId) {
    std::lock_guard<dxvk::mutex> lock(m_mutex);

    m_stats.frameId = frameId;
    m_stats.directDraws = 0u;
    m_stats.skippedDraws = 0u;
    m_stats.earlySleep = std::chrono::microseconds::zero();
    m_stats.mode = getModeLocked();
  }


  void FrameGovernor::endFrame(
          uint64_t              frameId,
          uint32_t              directDraws,
          uint32_t              skippedDraws) {
    std::lock_guard<dxvk::mutex> lock(m_mutex);

    m_stats.frameId = frameId;
    m_stats.directDraws = directDraws;
    m_stats.skippedDraws = skippedDraws;

    if (m_options.drawBudget)
      m_stats.drawBudget = m_options.drawBudget;
    else if (directDraws)
      m_stats.drawBudget = (directDraws * m_options.targetPercent + 99u) / 100u;

    if (m_recoveryFramesLeft)
      m_recoveryFramesLeft -= 1u;

    m_stats.mode = getModeLocked();
  }


  void FrameGovernor::notifyLimiterSleep(
          uint64_t              frameId,
          Duration              sleep) {
    std::lock_guard<dxvk::mutex> lock(m_mutex);

    if (!m_options.enabled || !m_options.pacing || m_targetInterval == Duration::zero())
      return;

    m_stats.frameId = frameId;
    m_stats.limiterSleep = std::chrono::duration_cast<std::chrono::microseconds>(sleep);

    if (sleep <= Duration::zero()) {
      m_pendingEarlyDelay = Duration::zero();
      m_stats.mode = getModeLocked();
      return;
    }

    // Keep a portion of limiter back-pressure at the next frame boundary.
    // Large sleeps commonly come from loading, alt-tab, or mode changes.
    m_pendingEarlyDelay = std::min(sleep, m_targetInterval / 2u);
    m_stats.mode = getModeLocked();
  }


  FrameGovernor::Duration FrameGovernor::getEarlyPaceDelay(
          uint64_t              frameId) {
    std::lock_guard<dxvk::mutex> lock(m_mutex);

    if (!m_options.enabled || !m_options.pacing || m_recoveryFramesLeft)
      return Duration::zero();

    Duration result = m_pendingEarlyDelay;
    m_pendingEarlyDelay = Duration::zero();

    if (result > Duration::zero()) {
      m_stats.frameId = frameId;
      m_stats.earlySleep = std::chrono::duration_cast<std::chrono::microseconds>(result);
    }

    m_stats.mode = getModeLocked();
    return result;
  }


  bool FrameGovernor::shouldShedDraw(
          uint32_t              directDrawIndex,
          uint32_t              vertexCount) const {
    std::lock_guard<dxvk::mutex> lock(m_mutex);

    if (!m_options.enabled || !m_options.drawShedding || m_recoveryFramesLeft)
      return false;

    if (!m_options.skipMaxVertices || vertexCount > m_options.skipMaxVertices)
      return false;

    return m_stats.drawBudget && directDrawIndex > m_stats.drawBudget;
  }


  FrameGovernorStats FrameGovernor::getStats() const {
    std::lock_guard<dxvk::mutex> lock(m_mutex);
    return m_stats;
  }


  FrameGovernorMode FrameGovernor::getModeLocked() const {
    if (!m_options.enabled || m_targetInterval == Duration::zero())
      return FrameGovernorMode::Disabled;

    if (m_recoveryFramesLeft)
      return FrameGovernorMode::Recover;

    if (m_options.drawShedding)
      return FrameGovernorMode::Shed;

    if (m_options.pacing && m_pendingEarlyDelay > Duration::zero())
      return FrameGovernorMode::Pace;

    return FrameGovernorMode::Observe;
  }

}
