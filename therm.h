/// @file therm.h
/// @brief therm
/// @author Jeff Perry <jeffsp@gmail.com>
/// @date 2013-04-30

// Copyright (C) 2013 Jeffrey S. Perry
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef THERM_H
#define THERM_H

#include "sensors.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace therm
{

/// @brief version info
const int MAJOR_REVISION = 0;
const int MINOR_REVISION = 3;

/// @brief convert from fahrenheit to celsius
///
/// @param c temperature in celsius
///
/// @return temperature in fahrenheit
double ctof (const double c)
{
    return c * 9.0 / 5.0 + 32.0;
}

/// @brief temperature reading
struct temperature
{
    double current;
    double high;
    double critical;
};

/// @brief fan speed reading
struct fan_speed
{
    double current;
};

/// @brief a chip on a bus with sensor data
struct chip
{
    std::string name;
    std::vector<temperature> temps;
    std::vector<fan_speed> fan_speeds;
};

/// @brief a bus that may have chips
struct bus
{
    std::string name;
    unsigned id;
    std::vector<chip> chips;
};

/// @brief collection of busses
typedef std::vector<bus> busses;

/// @brief scan the busses for sensor data
///
/// @param s sensors
///
/// @return vector of bus sensor data
busses scan (const sensors &s)
{
    busses bs;
    for (short i = 0; i < MAX_BUSSES; ++i)
    {
        // get chips on this bus
        auto chips = s.get_chips (i);
        if (chips.empty ())
            continue;
        // get bus name
        bus b;
        sensors_bus_id id { i, 0 };
        const char *name = sensors_get_adapter_name (&id);
        if (name == nullptr)
            b.name = "Unknown";
        else
            b.name = name;
        b.id = i;
        for (auto c : chips)
        {
            chip ch;
            ch.name = c->prefix;
            auto temps = s.get_temperatures (c);
            for (auto t : temps)
            {
                temperature temp { t.current, t.high, t.critical };
                ch.temps.push_back (temp);
            }
            auto fss = s.get_fan_speeds (c);
            for (auto f : fss)
            {
                fan_speed fs { f.current };
                ch.fan_speeds.push_back (fs);
            }
            b.chips.push_back (ch);
        }
        bs.push_back (b);
    }
    return bs;
}

} // namespace therm

#endif
