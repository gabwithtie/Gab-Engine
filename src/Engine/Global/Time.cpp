#include "Time.h"

gbe::Time::Time()
{
    using clock = std::chrono::high_resolution_clock;
    curr_time = clock::now();
    prev_time = curr_time;
    deltaTime = 0;
}

void gbe::Time::Reset() {
    prev_time = curr_time;
}

void gbe::Time::UpdateTime() {
    using clock = std::chrono::high_resolution_clock;

    curr_time = clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(curr_time - prev_time);
    prev_time = curr_time;

    auto final_scale = scale;

    if (this->paused)
        final_scale = 0;

    curr_ns += (float)dur.count() * scale;
}

void gbe::Time::TickFixed(std::function<void(double)> fixed_update_callback)
{
    auto final_scale = scale;

    if (this->paused)
        final_scale = 0;

    while (curr_ns >= fixed_timestep.count()) {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(fixed_timestep);
        auto seconds = (double)ms.count() / 1000;
        fixed_update_callback(seconds * final_scale);

        curr_ns -= fixed_timestep.count();
    }
}
