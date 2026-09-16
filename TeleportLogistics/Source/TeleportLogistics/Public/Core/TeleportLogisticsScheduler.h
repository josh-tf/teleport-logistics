#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

// Engine-independent policy, shared by the runtime and executable tests.
namespace teleport_logistics
{
struct Budget
{
    // Accrued credit is capped at a quarter second of throughput, so a hitch cannot
    // release a large backlog in one tick.
    static constexpr double BurstSeconds = 0.25;
    double credit = 0;
    void accrue(double seconds, double unitsPerSecond)
    {
        if (!std::isfinite(seconds) || !std::isfinite(unitsPerSecond) || seconds <= 0 || unitsPerSecond <= 0)
            return;
        credit = std::min(credit + seconds * unitsPerSecond, std::max(1.0, unitsPerSecond * BurstSeconds));
    }
    std::int64_t available() const
    {
        return static_cast<std::int64_t>(credit);
    }
    void spend(std::int64_t amount)
    {
        credit = std::max(0.0, credit - static_cast<double>(amount));
    }
};

struct Cursor
{
    std::size_t input = 0, output = 0;
    std::int64_t remaining = 0;
};

// Move must atomically transfer <= requested units and report the amount committed.
// Inputs and outputs have independent budgets; stalled ports never consume credit.
// A persistent output cursor prevents one source or a short tick favouring output 0.
template <class Move>
std::int64_t distribute(std::vector<Budget> &inputs, std::vector<Budget> &outputs, Cursor &cursor,
                        std::int64_t quantum, Move move, std::size_t maxAttempts = 65536)
{
    if (inputs.empty() || outputs.empty() || quantum <= 0)
        return 0;
    const auto ready = [](const Budget &budget) { return budget.available() > 0; };
    if (!std::any_of(inputs.begin(), inputs.end(), ready) ||
        !std::any_of(outputs.begin(), outputs.end(), ready))
        return 0;
    cursor.input %= inputs.size();
    cursor.output %= outputs.size();
    if (cursor.remaining <= 0 || cursor.remaining > quantum)
        cursor.remaining = quantum;
    std::int64_t total = 0;
    std::size_t attempts = 0, idleInputs = 0;
    while (attempts < maxAttempts && idleInputs < inputs.size())
    {
        const auto i = cursor.input;
        cursor.input = (cursor.input + 1) % inputs.size();
        bool progressed = false;
        if (inputs[i].available() > 0)
        {
            for (std::size_t checked = 0; checked < outputs.size() && attempts < maxAttempts; ++checked)
            {
                const auto o = cursor.output;
                ++attempts;
                const auto request =
                    std::min({cursor.remaining, inputs[i].available(), outputs[o].available()});
                const std::int64_t moved = request > 0 ? move(i, o, request) : 0;
                if (moved <= 0)
                {
                    cursor.output = (cursor.output + 1) % outputs.size();
                    cursor.remaining = quantum;
                    continue;
                }
                // The adapter owns conservation. Never account for more than requested.
                const auto committed = std::min(moved, request);
                inputs[i].spend(committed);
                outputs[o].spend(committed);
                total += committed;
                cursor.remaining -= committed;
                if (cursor.remaining == 0)
                {
                    cursor.output = (cursor.output + 1) % outputs.size();
                    cursor.remaining = quantum;
                }
                progressed = true;
                break;
            }
        }
        idleInputs = progressed ? 0 : idleInputs + 1;
    }
    return total;
}
} // namespace teleport_logistics
