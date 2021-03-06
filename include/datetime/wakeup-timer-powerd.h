/*
 * Copyright 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#ifndef INDICATOR_DATETIME_WAKEUP_TIMER_POWERD_H
#define INDICATOR_DATETIME_WAKEUP_TIMER_POWERD_H

#include <datetime/clock.h>
#include <datetime/wakeup-timer.h>

#include <memory> // std::unique_ptr, std::shared_ptr

namespace unity {
namespace indicator {
namespace datetime {

/***
****
***/

/**
 * \brief a WakeupTimer implemented with g_timeout_add() 
 */
class PowerdWakeupTimer: public WakeupTimer
{
public:
    explicit PowerdWakeupTimer(const std::shared_ptr<Clock>&);
    ~PowerdWakeupTimer();
    void set_wakeup_time(const DateTime&) override;
    core::Signal<>& timeout() override;

private:
    PowerdWakeupTimer(const PowerdWakeupTimer&) =delete;
    PowerdWakeupTimer& operator=(const PowerdWakeupTimer&) =delete;
    class Impl;
    std::unique_ptr<Impl> p;
};

/***
****
***/

} // namespace datetime
} // namespace indicator
} // namespace unity

#endif // INDICATOR_DATETIME_WAKEUP_TIMER_MAINLOOP_H
