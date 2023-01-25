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

using namespace std;

Terminal::Terminal(Monitor& new_monitor) {
    monitor = &new_monitor;
    init();
}

void Terminal::init() {

    // Form the line that shows each of the addresses entered
    string address_line = form_address_line();

    // Calculate the positions within the terminal that data should be written at for each address
    data_write_positions = calculate_data_write_positions(address_line);

    // Write the empty view-box to the terminal
    cout << Term::clear_screen() << Term::cursor_move(1, 1);
    cout << string(address_line.length(), '#') << '\n';
    cout << address_line << '\n';
    cout << '#' << string(address_line.length() - 2, ' ') << "#\n";
    cout << string(address_line.length(), '#') << '\n';
}

[[noreturn]] void Terminal::run() {

    while (true) {
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

        update_terminal(statuses, pings, numThreads);

        // Sleep for the specified interval
        sleep(monitor->getInterval());
    }
}

void Terminal::update_terminal(long* statuses, long* pings, size_t num_threads) {
    for (size_t i = 0; i < data_write_positions.size(); i++) {
        auto position_pair = data_write_positions.at(i);

        // Clear the old data and write the new data to the terminal
        cout << Term::cursor_move(position_pair.first, position_pair.second);
        cout << string(DATA_MAX_LENGTH, ' ');
        cout << Term::cursor_move(position_pair.first, position_pair.second);
        cout << "HTTP " << statuses[i] << " | " << pings[i] << "ms";
    }

    cout << Term::cursor_move(5, 1) << flush;
}

string Terminal::form_address_line() {
    string address_line;
    address_line.append("#");

    for (const string& address : *monitor->getAddresses()) {
        address_line.append("    ");

        size_t address_length = address.size();

        // If the address is shorter than the length of its data, then it must be padded with spaces
        if (address_length < DATA_MAX_LENGTH) {
            string padded_address;

            size_t beginning_length = (DATA_MAX_LENGTH - address_length) / 2;
            padded_address.append(string(beginning_length, ' '));
            padded_address.append(address);

            size_t end_length = (DATA_MAX_LENGTH - address_length) % 2 ? (DATA_MAX_LENGTH - address_length) / 2 : (DATA_MAX_LENGTH - address_length) / 2 + 1;
            padded_address.append(string(end_length, ' '));
            address_line.append(padded_address);
        } else {
            address_line.append(address);
        }

        address_line.append("    #");
    }

    return address_line;
}

pair_vector Terminal::calculate_data_write_positions(string_view address_line) {
    vector<pair<size_t, size_t>> returnVector;

    // The list of addresses being used by the monitor and its size
    vector<string>* addresses = monitor->getAddresses();
    size_t numAddresses = addresses->size();

    // To help string::find() run properly with multiple addresses
    size_t last_address_index = 0;

    // Loop through each of the monitor's addresses
    for (size_t i = 0; i < numAddresses; i++) {
        string address = addresses->at(i);

        // Find where the current address starts within the address line, used to calculate the column
        // index its data starts at
        size_t address_start_index = address_line.find(address, last_address_index + 1);
        last_address_index = address_start_index;

        // If the address is shorter in length than its data, then the data will be shifted to the left relative
        // to its address using the formula: offset = (DATA_MAX_LENGTH - address length) / 2 - 1
        if (address.size() <= DATA_MAX_LENGTH) {
            size_t offset = (DATA_MAX_LENGTH - address.size()) / 2 - 1;
            size_t columnIndex = address_start_index - offset;
            returnVector.emplace_back(DATA_ROW_INDEX, columnIndex);
        }
        // Otherwise, the data will be shifted right relative to its address using the
        // formula: offset = (address length - DATA_MAX_LENGTH) / 2 + 1
        else {
            size_t offset = (address.size() - DATA_MAX_LENGTH) / 2 + 2;
            size_t columnIndex = address_start_index + offset;
            returnVector.emplace_back(DATA_ROW_INDEX, columnIndex);
        }
    }

    return returnVector;
}
