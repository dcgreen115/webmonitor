#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <iostream>
#include <thread>
#include "terminal.hpp"
#include "monitor.hpp"
#include <cpp-terminal/base.hpp>
#include <utility>
#include <csignal>

using namespace std;

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

/**
 * Sets up the empty view-box within the terminal and calculates write
 * positions such that text is centered within their boxes.
 */
void Terminal::init() {

    // Form the line that shows each of the addresses entered
    const string address_line = form_address_line();

    // Calculate the positions within the terminal that data should be written at for each address
    data_write_positions = calculate_data_positions(address_line);

    // Write the empty view-box to the terminal
    cout << Term::clear_screen() << Term::cursor_move(1, 1);
    cout << string(address_line.length(), '#') << '\n';
    cout << address_line << '\n';

    cout << '#';
    for (string_view address : *monitor->getAddresses()) {
        if (address.length() < DATA_MAX_LENGTH) {
            cout << std::string(25, ' ') << '#';
        } else {
            cout << std::string(address.length() + 8, ' ') << '#';
        }

    }
    cout << '\n';

    //cout << '#' << string(address_line.length() - 2, ' ') << "#\n";
    cout << string(address_line.length(), '#') << endl;
}

/**
 * Continuously updates the HTTP status and time-to-last-byte for each address
 * held by the monitor and prints new data to the terminal.
 */
void Terminal::run() {

    while (running) {
        // Set up arrays to hold threads and their return values
        size_t numThreads = monitor->getAddresses()->size();
        thread statusThreads[numThreads];
        long statuses[numThreads];
        long pings[numThreads];

        // Spawn an HTTP status and ping thread for each address
        for (size_t i = 0; i < numThreads; i++) {

            // Initialize the current elements in the arrays - these values will be updated in the threads upon success
            statuses[i] = -1;
            pings[i] = -1;

            // Start the threads
            statusThreads[i] = thread(&Terminal::statusThread, ref(*monitor), i, ref(statuses[i]), ref(pings[i]));
        }

        for (size_t i = 0; i < numThreads; i++) {
            statusThreads[i].join();
        }

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
void Terminal::update_terminal(const long* statuses, const long* times) const {
    for (size_t i = 0; i < data_write_positions.size(); i++) {
        auto position_pair = data_write_positions.at(i);

        // Clear the old data and write the new data to the terminal
        cout << Term::cursor_move(position_pair.first, position_pair.second);
        cout << string(DATA_MAX_LENGTH, ' ');
        cout << Term::cursor_move(position_pair.first, position_pair.second);

        if (statuses[i] != -1) {
            cout << "HTTP " << statuses[i] << " | " << times[i] << "ms";
        } else {
            cout << "      ERROR";
        }

    }

    cout << Term::cursor_move(5, 1) << flush;
}

/**
 * Creates a string containing each of the monitor's addresses to be displayed
 * in the terminal. Each address will be centered within their respective
 * boxes, and the string returned can be printed to the terminal unmodified.
 * @return The string containing each of the monitor's addresses
 */
string Terminal::form_address_line() {
    string address_line;
    address_line.append("#"); // The left edge of the view-box

    for (const string_view address : *monitor->getAddresses()) {

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

        } else { // Otherwise, we can add the address as is
            address_line.append(address);
        }

        // Addresses will have at least 4 trailing spaces and a '#' separator
        address_line.append("    #");
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
vector<pair<size_t, size_t>> Terminal::calculate_data_positions(const string_view address_line) {
    vector<pair<size_t, size_t>> returnVector;

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

        // If the address is shorter than its data, then the data will be shifted to the left relative
        // to its address using the formula: offset = (DATA_MAX_LENGTH - address length) / 2 - 1
        if (address.size() < DATA_MAX_LENGTH) {
            size_t offset = (DATA_MAX_LENGTH - address.size()) / 2 - 1;
            size_t columnIndex = address_start_index - offset;
            returnVector.emplace_back(DATA_ROW_INDEX, columnIndex);
        }
        // Otherwise, the data will be shifted right relative to its address using the
        // formula: offset = (address length - DATA_MAX_LENGTH) / 2 + 2
        else {
            size_t offset = (address.size() - DATA_MAX_LENGTH) / 2 + 2;
            size_t columnIndex = address_start_index + offset;
            returnVector.emplace_back(DATA_ROW_INDEX, columnIndex);
        }
    }

    return returnVector;
}
