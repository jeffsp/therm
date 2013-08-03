/// @file ui.h
/// @brief user interface
/// @author Jeff Perry <jeffsp@gmail.com>
/// @date 2013-07-04

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

#ifndef UI_H
#define UI_H

#include "options.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <ncurses.h>
#include <sstream>

namespace proctemp
{

/// @brief draw ncurses text with attributes
///
/// @param attrs vector of attrs
/// @param r row
/// @param c col
/// @param s string
/// @param args print args
template<typename... T>
void text (const std::vector<int> &attrs, int r, int c, const char *s, T... args)
{
    for (auto a : attrs)
        attron (a);
    mvprintw (r, c, s, args...);
    for (auto a : attrs)
        attroff (a);
}

/// @brief ncurses user interface
class ncurses_ui
{
    private:
    /// @brief screen dimensions
    int rows, cols;
    /// @brief configuration options
    options &opts;
    /// @brief event loop support
    bool done;
    /// @brief flag for debugging
    bool debug;
    static const int WHITE = COLOR_PAIR(1);
    static const int GREEN = COLOR_PAIR(2);
    static const int YELLOW = COLOR_PAIR(3);
    static const int RED = COLOR_PAIR(4);
    static const int BLUE = COLOR_PAIR(5);
    public:
    /// @brief constructor
    ncurses_ui (options &opts)
        : opts (opts)
        , done (false)
        , debug (false)
    {
        init ();
        labels ();
    }
    /// @brief destructor
    ~ncurses_ui ()
    {
        release ();
    }
    /// @brief initialize ncurses stuff
    void init ()
    {
        initscr ();
        start_color ();
        use_default_colors ();
        raw ();
        keypad (stdscr, 1);
        noecho ();
        curs_set (0); // make cursor invisible
        erase ();
        getmaxyx (stdscr, rows, cols);
        init_pair (1, COLOR_WHITE, -1);
        init_pair (2, COLOR_GREEN, -1);
        init_pair (3, COLOR_YELLOW, -1);
        init_pair (4, COLOR_RED, -1);
        init_pair (5, COLOR_BLUE, -1);
        timeout (1000); // timeout in ms
    }
    /// @brief ncurses cleanup
    void release () const
    {
        endwin ();
    }
    /// @brief event loop support
    ///
    /// @return true if done
    bool is_done () const
    {
        return done;
    }
    /// @brief event loop support
    void process (int ch, const std::string &config_fn)
    {
        switch (ch)
        {
            default:
            break;
            case 'q':
            case 'Q':
            done = true;
            break;
            case 's':
            case 'S':
            release ();
            write (opts, config_fn);
            init ();
            labels ();
            break;
            case 't':
            case 'T':
            opts.set_fahrenheit (!opts.get_fahrenheit ());
            break;
            case '!':
            debug = !debug;
            release ();
            init ();
            labels ();
            break;
        }
        if (is_term_resized (rows, cols))
        {
            release ();
            init ();
            labels ();
        }
        refresh ();
    }
    /// @brief display temps
    ///
    /// @param busses vector of busses
    void show_temps (const busses &bs) const
    {
        // get the width of the cpu number column
        size_t max_cpus = 0;
        for (auto bus : bs)
            for (auto chip : bus.chips)
                if (chip.temps.size () > max_cpus)
                    max_cpus = chip.temps.size ();
        std::stringstream ss;
        ss << max_cpus;
        // length of largest number plus a space
        const int indent1 = ss.str ().size () + 1;
        // assumes temps are 3 digits at most, plus the C or F, plus a space
        const int indent2 = indent1 + 5;
        // print the temperatures
        auto row = 0;
        for (auto bus : bs)
        {
            text ({}, row++, 0, "%s", bus.name.c_str ());
            size_t chipno = 0;
            for (auto chip : bus.chips)
            {
                // don't print on last line
                if (row + 1 == rows)
                    continue;
                if (bus.chips.size () > 1)
                    text ({}, row++, 0, "%s %d", chip.name.c_str (), chipno++);
                else
                    text ({}, row++, 0, "%s", chip.name.c_str ());
                size_t n = 0;
                for (auto t : chip.temps)
                {
                    // don't print on last line
                    if (row + 1 == rows)
                        continue;
                    if (debug && !(rand () % chip.temps.size ()))
                        t.current = (rand () % int (t.critical + 10 - t.high)) + t.high;
                    // print the cpu number
                    std::stringstream ss;
                    ss << n++;
                    text ({}, row, 0, ss.str ().c_str ());
                    // print the numerical value
                    ss.str ("");
                    ss << round (opts.get_fahrenheit () ? ctof (t.current) : t.current) << (opts.get_fahrenheit () ? 'F' : 'C');
                    if (t.high == -1)
                    {
                        int color = GREEN;
                        text ({A_BOLD, color}, row, indent1, "%4s", ss.str ().c_str ());
                    }
                    else
                    {
                        int color = GREEN;
                        if (t.current >= t.high)
                            color = YELLOW;
                        if (t.current >= t.critical)
                            color = RED;
                        text ({A_BOLD, color}, row, indent1, "%4s", ss.str ().c_str ());
                        // print the bar
                        const int size = 2 * cols / 3 - indent2 - 5;
                        temp_bar (n, row++, indent2, size, t);
                    }
                }
            }
        }
    }
    private:
    /// @brief draw a temperature bar
    ///
    /// @tparam T temperature type
    /// @param n cpu number
    /// @param i row
    /// @param j col
    /// @param size bar length
    /// @param t temperature
    template<typename T>
    void temp_bar (size_t n, int i, int j, int size, T t) const
    {
        text ({A_BOLD}, i, j, "[");
        text ({A_BOLD}, i, j + size - 1, "]");
        const int MIN = 40;
        const int MAX = t.critical + 5;
        int current = t.current < MIN ? MIN : (t.current > MAX ? MAX : t.current);
        int len = size * (current - MIN) / (MAX - MIN);
        for (int k = 1; k + 1 < size; ++k)
        {
            int color;
            if (k < size * (t.high - MIN) / (MAX - MIN))
                color = GREEN;
            else if (k < size * (t.critical - MIN) / (MAX - MIN))
                color = YELLOW;
            else
                color = RED;
            if (k < len)
                text ({A_BOLD, A_REVERSE, color}, i, j + k, " ");
            else
                text ({A_BOLD, color}, i, j + k, "-");
        }
    }
    /// @brief draw labels
    void labels () const
    {
        int row = 0;
        const int COL = 2 * cols / 3;
        std::stringstream ss;
        ss.str ("");
        ss << "proctempview version " << proctemp::MAJOR_REVISION << '.' << proctemp::MINOR_REVISION;
        text ({A_BOLD, BLUE}, rows - 1, 0, ss.str ().c_str ());
        ss.str ("");
        ss << "T = change Temperature scale";
        text ({}, row++, COL, ss.str ().c_str ());
        ss.str ("");
        ss << "S = Save configuration options";
        text ({}, row++, COL, ss.str ().c_str ());
        ss.str ("");
        ss << "Q = Quit";
        text ({}, row++, COL, ss.str ().c_str ());
        if (debug)
        {
            ++row;
            ss.str ("");
            ss << "ncurses version " << NCURSES_VERSION_MAJOR << '.' << NCURSES_VERSION_MINOR;
            text ({}, row++, COL, ss.str ().c_str ());
            ss.str ("");
            ss << "terminal dimensions " << rows << " X " << cols;
            text ({}, row++, COL, ss.str ().c_str ());
            ++row;
            ss.str ("");
            ss << "YOU ARE IN DEBUG MODE.";
            text ({}, row++, COL, ss.str ().c_str ());
            ss.str ("");
            ss << "PRESS '!' TO TURN OFF DEBUG MODE.";
            text ({}, row++, COL, ss.str ().c_str ());
        }
    }
};

/// @brief ncurses user interface
class debug_ui
{
    private:
    options &opts;
    int done;
    public:
    /// @brief constructor
    debug_ui (options &opts)
        : opts (opts)
        , done (0)
    {
        std::clog << "options" << std::endl;
        std::clog << "fahrenheit:\t" << opts.get_fahrenheit () << std::endl;
    }
    /// @brief initialize
    void init ()
    {
    }
    /// @brief cleanup
    void release () const
    {
    }
    /// @brief event loop support
    ///
    /// @return true if done
    bool is_done ()
    {
        return done++;
    }
    /// @brief event loop support
    void process (int , const std::string &)
    {
    }
    /// @brief display temps
    ///
    /// @param busses vector of busses
    void show_temps (const busses &bs) const
    {
        for (auto bus : bs)
        {
            std::clog << bus.name << std::endl;
            for (auto chip : bus.chips)
            {
                std::clog << "adapter " << chip.name << std::endl;
                for (auto t : chip.temps)
                {
                    std::clog
                        << round (opts.get_fahrenheit () ? ctof (t.current) : t.current)
                        << (opts.get_fahrenheit () ? 'F' : 'C')
                        << std::endl;
                }
            }
        }
    }
};

} // namespace proctemp

#endif
