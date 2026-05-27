#pragma once

#include "thread.h"
#include "util_time.h"

namespace dxvk {

  enum class FrameGovernorMode : uint32_t {
    Disabled,
    Observe,
    Pace,
    Shed,
    Recover,
  };

  struct FrameGovernorOptions {
    bool enabled = false;
    bool pacing = true;
    bool drawShedding = false;
    uint32_t drawBudget = 0;
    uint32_t targetPercent = 110;
    uint32_t skipMaxVertices = 64;
    uint32_t recoveryFrames = 30;
  };

  struct FrameGovernorStats {
    uint64_t frameId = 0;
    uint32_t directDraws = 0;
    uint32_t skippedDraws = 0;
    uint32_t drawBudget = 0;
    std::chrono::microseconds limiterSleep = { };
    std::chrono::microseconds earlySleep = { };
    FrameGovernorMode mode = FrameGovernorMode::Disabled;
  };

  /**
   * \brief Frame pacing governor
   *
   * Tracks limiter sleeps and exposes a bounded early-frame delay so a
   * front-end can pace before submitting a full frame worth of work.
   */
  class FrameGovernor {

  public:

    using Duration = std::chrono::nanoseconds;

    void setOptions(
      const FrameGovernorOptions& options);

    void setTargetFrameRate(
            double                frameRate);

    void beginFrame(
            uint64_t              frameId);

    void endFrame(
            uint64_t              frameId,
            uint32_t              directDraws,
            uint32_t              skippedDraws);

    void notifyLimiterSleep(
            uint64_t              frameId,
            Duration              sleep);

    Duration getEarlyPaceDelay(
            uint64_t              frameId);

    bool shouldShedDraw(
            uint32_t              directDrawIndex,
            uint32_t              vertexCount) const;

    FrameGovernorStats getStats() const;

  private:

    mutable dxvk::mutex     m_mutex;

    FrameGovernorOptions    m_options = { };
    FrameGovernorStats      m_stats = { };

    Duration                m_targetInterval = Duration::zero();
    Duration                m_pendingEarlyDelay = Duration::zero();

    uint32_t                m_recoveryFramesLeft = 0u;

    FrameGovernorMode getModeLocked() const;

  };

}
