// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef autonomylib_common_StateReporterWrapper_hpp
#define autonomylib_common_StateReporterWrapper_hpp

#include "Common.hpp"
#include "FrequencyLimiter.hpp"
#include "StateReporter.hpp"
#include "UpdatableObject.hpp"
#include "utils/OnlineStats.hpp"

#include <iomanip>
#include <sstream>
#include <string>

namespace nervosys {
namespace autonomylib {

class StateReporterWrapper : public UpdatableObject {

  private:
    typedef common_utils::OnlineStats OnlineStats;

    StateReporter report_;

    OnlineStats dt_stats_;

    FrequencyLimiter report_freq_;
    bool enabled_;
    bool is_wait_complete = false;
    TTimePoint last_time_;

  public:
    static constexpr real_T DefaultReportFreq = 3.0f;

    StateReporterWrapper(bool enabled = false, int float_precision = 3, bool is_scientific_notation = false) {
        initialize(enabled, float_precision, is_scientific_notation);
    }
    void initialize(bool enabled = false, int float_precision = 3, bool is_scientific_notation = false) {
        enabled_ = enabled;
        report_.initialize(float_precision, is_scientific_notation);
        report_freq_.initialize(DefaultReportFreq);
    }

    void clearReport() {
        report_.clear();
        is_wait_complete = false;
    }

    //*** Start: UpdatableState implementation ***//
    virtual void resetImplementation() override {
        last_time_ = clock()->nowNanos();
        clearReport();
        dt_stats_.clear();
        report_freq_.reset();
    }

    virtual void failResetUpdateOrdering(std::string err) override {
        // Do nothing.
        // Disable checks for reset/update sequence because
        // this object may get created but not used.
    }

    virtual void update() override {
        UpdatableObject::update();

        TTimeDelta dt = clock()->updateSince(last_time_);

        if (enabled_) {
            dt_stats_.insert(dt);
            report_freq_.update();
            is_wait_complete = is_wait_complete || report_freq_.isWaitComplete();
        }
    }
    virtual void reportState(StateReporter &reporter) override {
        // TODO: perhaps we should be using supplied reporter?
        unused(reporter);

        // write dt stats
        report_.writeNameOnly("dt");
        report_.writeValueOnly(dt_stats_.mean());
        report_.writeValueOnly(dt_stats_.variance());
        report_.writeValueOnly(dt_stats_.size(), true);
    }
    //*** End: UpdatableState implementation ***//

    bool canReport() { return enabled_ && is_wait_complete; }

    StateReporter *getReporter() { return &report_; }

    string getOutput() { return report_.getOutput(); }

    void setReportFreq(real_T freq) { report_freq_.initialize(freq); }

    void setEnable(bool enable) {
        if (enable == enabled_)
            return;

        enabled_ = enable;
        if (enable)
            report_freq_.initialize(DefaultReportFreq);
        else
            report_freq_.initialize(0);
    }
    bool getEnable() { return enabled_; }
};

} // namespace autonomylib
} // namespace nervosys

#endif
