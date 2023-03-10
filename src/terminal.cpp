#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <utility>
#include <csignal>
#include <iostream>
#include <thread>
#include <cpp-terminal/base.hpp>
#include "terminal.hpp"

using std::string;
using std::size_t;
using std::pair;
using std::vector;
using std::cout;

// If set to true, the terminal refresh loop will continue running
extern sig_atomic_t running;

/**
 * Creates a new terminal controller, using the specified monitor to obtain
 * new data
 * @param new_monitor The monitor to take data from
 */
Terminal::Terminal(Monitor* new_monitor) {
    monitor = new_monitor;
    init();
}

// Resets the terminal the program runs in
Terminal::~Terminal() {
    std::cout << Term::clear_screen() << Term::cursor_move(1, 1);
}

/**
 * Sets up the empty view-box within the terminal and calculates write
 * positions such that text is centered within their boxes.
 */
void Terminal::init() {
    // Form the line that shows each of the addresses entered
    const auto address_line = form_address_line();

    // Calculate the positions within the terminal that data should be written at for each address
    data_write_positions = calculate_data_positions(address_line);

    // Write the empty view-box to the terminal
    cout << Term::clear_screen() << Term::cursor_move(1, 1);
    cout << TERMINAL_GRAY << string(address_line.length(), '#') << TERMINAL_WHITE << '\n';
    cout << address_line << '\n';

    // Separating '#'s for the address and data rows
    cout << TERMINAL_GRAY << '#';
    cout << cursor_move_relative(-1, -1) << '#' << Term::cursor_down(1);
    for (std::string_view address : *monitor->getAddresses()) {
        if (address.length() < DATA_MAX_LENGTH) {
            cout << std::string(25, ' ') << '#';
            cout << cursor_move_relative(-1, -1) << '#' << Term::cursor_down(1);
        } else {
            cout << std::string(address.length() + 8, ' ') << '#';
            cout << cursor_move_relative(-1, -1) << '#' << Term::cursor_down(1);
        }
    }
    cout << '\n';
    cout << string(address_line.length(), '#') << TERMINAL_WHITE << std::endl;
}

/**
 * Continuously updates the HTTP status and time-to-last-byte for each address
 * held by the monitor and prints new data to the terminal.
 */
void Terminal::run() {
    const size_t numThreads = monitor->getAddresses()->size();
    vector<std::thread> statusThreads;
    vector<int64_t> statuses(numThreads);
    vector<int64_t> pings(numThreads);

    while (running) {
        // Fill the status and TTLB vectors with default values
        // These will be updated inside the status threads
        fill_n(statuses.begin(), numThreads, -1);
        fill_n(pings.begin(), numThreads, -1);

        // Spawn an HTTP status thread for each address, then join them all
        for (size_t i = 0; i < numThreads; i++) {
            statusThreads.emplace_back(&Terminal::statusThread, monitor, i, &statuses.at(i), &pings.at(i));
        }
        std::for_each_n(statusThreads.begin(), numThreads, [&](std::thread& t){t.join();});
        statusThreads.clear();

        update_terminal(statuses, pings);

        // Sleep for the specified interval
        sleep(monitor->getInterval());
    }
}

/**
 * Updates the data line within the terminal, writing new HTTP statuses and
 * time-to-last-byte times for each address
 * @param statuses An array containing the HTTP statuses of each address
 * @param times An array containing the time-to-last-byte for each address
 */
void Terminal::update_terminal(const vector<int64_t>& statuses, const vector<int64_t>& times) const {
    for (size_t i = 0; i < data_write_positions.size(); i++) {
        auto column = data_write_positions.at(i);

        // Clear the old data and write the new data to the terminal
        cout << Term::cursor_move(DATA_ROW_INDEX, column);
        cout << string(DATA_MAX_LENGTH, ' ');
        cout << Term::cursor_move(DATA_ROW_INDEX, column);

        if (statuses[i] != -1) {
            cout << get_status_color(statuses[i]) << "HTTP " << statuses[i]
            << TERMINAL_WHITE << " | "
            << get_time_color(times[i]) << times[i] << "ms";
        } else {
            cout << TERMINAL_RED << "      ERROR" << TERMINAL_WHITE;
        }
    }

    cout << Term::cursor_move(5, 1) << std::flush;
}

/**
 * Creates a string containing each of the monitor's addresses to be displayed
 * in the terminal. Each address will be centered within their respective
 * boxes, and the string returned can be printed to the terminal unmodified.
 * @return The string containing each of the monitor's addresses
 */
string Terminal::form_address_line() {
    string address_line;
    address_line.append(" ");  // The left edge of the view-box will be handled in init(), so just print a space here

    for (std::string_view address : *monitor->getAddresses()) {
        // All addresses will have at least 4 leading spaces
        address_line.append("    ");

        size_t address_length = address.size();

        // If the address is shorter than the length of its data, then it must be padded with spaces
        if (address_length < DATA_MAX_LENGTH) {
            string padded_address;

            // Add the left padding
            size_t left_padding = (DATA_MAX_LENGTH - address_length) / 2;
            padded_address.append(string(left_padding, ' '));

            // Add the address in the middle
            padded_address.append(address);

            // Add the right padding
            size_t right_padding;
            if ((DATA_MAX_LENGTH - address_length) % 2 == 1) {
                right_padding = (DATA_MAX_LENGTH - address_length) / 2 + 1;
            } else {
                right_padding = (DATA_MAX_LENGTH - address_length) / 2;
            }
            padded_address.append(string(right_padding, ' '));

            // Finally, add the newly padded address to the address line
            address_line.append(padded_address);

        } else {  // Otherwise, we can add the address as is
            address_line.append(address);
        }

        // Addresses will have at least 4 trailing spaces and a '#' separator
        // The '#' separator is printed in init(), so we will append 5 spaces here
        address_line.append("     ");
    }

    return address_line;
}

/**
 * Finds the indices within the terminal window that data should be written at
 * for each of the monitor's addresses
 * @param address_line The line to be printed on the terminal containing
 * each address
 * @return A vector containing std::pair<std:size_t, std:size_t> where each
 * pair holds <row, column> indices where the data for its corresponding
 * address should be printed at
 */
vector<size_t> Terminal::calculate_data_positions(const std::string_view address_line) const {
    vector<size_t> returnVector;

    // The list of addresses being used by the monitor and its size
    auto addresses = monitor->getAddresses();
    auto numAddresses = addresses->size();

    // To help string::find() run properly with multiple addresses
    size_t last_address_index = 0;

    // Loop through each of the monitor's addresses
    for (size_t i = 0; i < numAddresses; i++) {
        string address = addresses->at(i);

        // Find where the current address starts within the address line, used to calculate the column
        // index its data starts at
        size_t address_start_index = address_line.find(address, last_address_index + 1);
        last_address_index = address_start_index + address.length();

        /* If the address is shorter than its data, then the data will be shifted to the left relative
         to its address using the formula: offset = (DATA_MAX_LENGTH - address length) / 2 - 1
         Otherwise, the data will be shifted right relative to its address using the
         formula: offset = (address length - DATA_MAX_LENGTH) / 2 + 2 */
        if (address.size() < DATA_MAX_LENGTH) {
            size_t offset = (DATA_MAX_LENGTH - address.size()) / 2 - 1;
            size_t columnIndex = address_start_index - offset;
            returnVector.push_back(columnIndex);
        } else {
            size_t offset = (address.size() - DATA_MAX_LENGTH) / 2 + 2;
            size_t columnIndex = address_start_index + offset;
            returnVector.push_back(columnIndex);
        }
    }

    return returnVector;
}

/**
 * Moves the terminal cursor relative to its current location
 * @param rows How many rows to move. Negative values move up, positive values move down.
 * @param columns How many columns to move. Negative values move left, positive values move right.
 * @return A TTY code that specifies where the cursor should move, as an std::string
 */
std::string Terminal::cursor_move_relative(int64_t rows, int64_t columns) {
    std::string return_string;
    if (rows < 0) {
        return_string.append(Term::cursor_up(rows * -1));
    } else if (rows > 0) {
        return_string.append(Term::cursor_down(rows));
    }

    if (columns < 0) {
        return_string.append(Term::cursor_left(columns * -1));
    } else if (columns > 0) {
        return_string.append(Term::cursor_right(columns));
    }

    return return_string;
}

/**
 * Returns a TTY color code based on the HTTP status passed in the parameter.
 * 4XX and 5XX codes will be shown in red, while all other codes will be shown in green.
 * @param status An HTTP status code
 * @return The TTY color code corresponding to the HTTP status as an std::string
 */
string Terminal::get_status_color(int64_t status) const {
    if (status >= 400) {
        return TERMINAL_RED;
    } else {
        return TERMINAL_GREEN;
    }
}

/**
 * Returns a TTY color code based on the time-to-last-byte passed in the parameter.
 * Times 200ms and below will be shown in green, between 201ms and 1000ms inclusive in yellow,
 * and 1001 or above in red.
 * @param time A time in milliseconds
 * @return The TTY color code corresponding to the time parameter as an std::string
 */
string Terminal::get_time_color(int64_t time) const {
    if (time <= 200) {
        return TERMINAL_GREEN;
    } else if (time <= 1000) {
        return TERMINAL_YELLOW;
    } else {
        return TERMINAL_RED;
    }
}
