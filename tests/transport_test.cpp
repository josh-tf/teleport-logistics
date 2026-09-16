#include "Core/TeleportLogisticsScheduler.h"
#include <cassert>
#include <deque>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <limits>

using teleport_logistics::Budget;
using teleport_logistics::Cursor;
using teleport_logistics::distribute;

void merger_splitter()
{
    std::vector<Budget> in(2), out(3);
    Cursor cursor;
    std::vector<int> source{90, 90}, dest(3);
    for (int tick = 0; tick < 180; ++tick)
    {
        for (auto &b : in)
            b.accrue(.05, 20);
        for (auto &b : out)
            b.accrue(.05, 20);
        distribute(in, out, cursor, 1, [&](auto i, auto o, auto) {
            if (source[i] == 0)
                return 0;
            --source[i];
            ++dest[o];
            return 1;
        });
    }
    assert(source[0] == 0 && source[1] == 0);
    assert(dest[0] == 60 && dest[1] == 60 && dest[2] == 60);
}
void blocked_receiver_and_recovery()
{
    std::vector<Budget> in(1), out(3);
    Cursor cursor;
    int received[3]{};
    for (int t = 0; t < 300; ++t)
    {
        in[0].accrue(.05, 20);
        for (auto &b : out)
            b.accrue(.05, 20);
        distribute(in, out, cursor, 1, [&](auto, auto o, auto) {
            if (o == 1 && t < 150)
                return 0;
            ++received[o];
            return 1;
        });
    }
    assert(received[1] == 50);
    assert(received[0] == 125 && received[2] == 125);
}
void head_of_line_and_item_state()
{
    struct Item
    {
        int type, state;
    };
    std::deque<Item> source{{1, 77}, {2, 88}, {1, 99}};
    std::vector<Item> destination;
    std::vector<Budget> in(1), out(1);
    Cursor cursor;
    int accepts = 2;
    auto move = [&](auto, auto, auto) {
        if (source.empty() || source.front().type != accepts)
            return 0;
        destination.push_back(source.front());
        source.pop_front();
        return 1;
    };
    in[0].credit = 3;
    out[0].credit = 3;
    assert(distribute(in, out, cursor, 1, move) == 0); // Does not skip the blocked first item.
    accepts = 1;
    assert(distribute(in, out, cursor, 1, move) == 1);
    accepts = 2;
    assert(distribute(in, out, cursor, 1, move) == 1);
    accepts = 1;
    assert(distribute(in, out, cursor, 1, move) == 1);
}
void fluids_fair_by_volume()
{
    std::vector<Budget> in(2), out(2);
    Cursor cursor;
    int received[2]{};
    for (int t = 0; t < 1000; ++t)
    {
        in[0].accrue(.05, 2000);
        in[1].accrue(.05, 20000);
        for (auto &b : out)
            b.accrue(.05, 20000);
        distribute(in, out, cursor, 1000, [&](auto, auto o, auto amount) {
            received[o] += amount;
            return amount;
        });
    }
    assert(std::abs(received[0] - received[1]) <= 1000);
    assert(received[0] + received[1] == 1100000);
}
void budget_bounds()
{
    Budget b;
    b.accrue(3600, 20);
    assert(b.available() == 5);
    b.spend(2);
    assert(b.available() == 3);
    b.accrue(-1, 20);
    b.accrue(std::numeric_limits<double>::quiet_NaN(), 20);
    assert(b.available() == 3);
    b.spend(100);
    assert(b.available() == 0);
    std::vector<Budget> in, out;
    Cursor cursor;
    assert(distribute(in, out, cursor, 1, [](auto, auto, auto) { return 1; }) == 0);
    in.resize(1);
    out.resize(1);
    in[0].credit = 100;
    out[0].credit = 100;
    int calls = 0;
    distribute(
        in, out, cursor, 1,
        [&](auto, auto, auto) {
            ++calls;
            return 1;
        },
        7);
    assert(calls == 7);
    assert(in[0].available() == 93);
}
void topology_changes()
{
    std::vector<Budget> in(4), out(4);
    Cursor cursor{999, 999, 0};
    for (auto &b : in)
        b.credit = 1;
    for (auto &b : out)
        b.credit = 1;
    assert(distribute(in, out, cursor, 1, [](auto, auto, auto) { return 1; }) == 4);
    out.resize(1);
    in.resize(1);
    in[0].credit = 1;
    out[0].credit = 1;
    assert(distribute(in, out, cursor, 1, [](auto, auto, auto) { return 1; }) == 1);
}
void randomized_conservation()
{
    std::mt19937 rng(43197);
    for (int run = 0; run < 200; ++run)
    {
        const int ni = 1 + rng() % 8, no = 1 + rng() % 8;
        std::vector<int> source(ni), dest(no), capacity(no);
        std::vector<Budget> in(ni), out(no);
        Cursor cursor;
        for (auto &n : source)
            n = 10 + rng() % 1000;
        for (auto &n : capacity)
            n = 1 + rng() % 200;
        const int initial = std::accumulate(source.begin(), source.end(), 0);
        int consumed = 0;
        for (int tick = 0; tick < 300; ++tick)
        {
            for (auto &b : in)
                b.accrue(.05, 20);
            for (auto &b : out)
                b.accrue(.05, 20);
            distribute(in, out, cursor, 1, [&](auto i, auto o, auto request) {
                const int moved = std::min({source[i], capacity[o] - dest[o], static_cast<int>(request)});
                source[i] -= moved;
                dest[o] += moved;
                return moved;
            });
            for (int o = 0; o < no; ++o)
            {
                if (rng() % 3 == 0 && dest[o] > 0)
                {
                    --dest[o];
                    ++consumed;
                }
                assert(dest[o] >= 0 && dest[o] <= capacity[o]);
            }
            assert(std::accumulate(source.begin(), source.end(), 0) +
                       std::accumulate(dest.begin(), dest.end(), 0) + consumed ==
                   initial);
        }
    }
}
int main()
{
    merger_splitter();
    blocked_receiver_and_recovery();
    head_of_line_and_item_state();
    fluids_fair_by_volume();
    budget_bounds();
    topology_changes();
    randomized_conservation();
    std::cout << "PASS\n";
}
