//
//
//

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


Terminal::Terminal(Monitor& new_monitor) {
    monitor = &new_monitor;
}

void Terminal::init() {



    // Form the line that shows each of the addresses entered
    std::string address_line = form_address_line();

    dataWritePositions = calculate_data_write_positions(address_line);

    std::cout << Term::clear_screen() << Term::cursor_move(1, 1);
    std::cout << std::string(address_line.length(), '#') << '\n';
    std::cout << address_line << '\n';
    std::cout << '#' << std::string(address_line.length() - 2, ' ') << "#\n";
    std::cout << std::string(address_line.length(), '#') << '\n';
}

[[noreturn]] void Terminal::run() {

    while (true) {

        // Set up arrays to hold threads and their return values
        std::size_t numThreads = monitor->getAddresses()->size();
        std::thread statusThreads[numThreads];
        long statuses[numThreads];
        long pings[numThreads];

        //std::cout << "Starting threads" << std::endl;
        // Spawn an HTTP status and ping thread for each address
        for (std::size_t i = 0; i < numThreads; i++) {

            // Initialize the current elements in the arrays - these values will be updated in the threads upon success
            statuses[i] = -1;
            pings[i] = -1;

            // Start the threads
            statusThreads[i] = std::thread(&Terminal::statusThread, std::ref(*monitor), i, std::ref(statuses[i]), std::ref(pings[i]));
        }

        for (std::size_t i = 0; i < numThreads; i++) {
            statusThreads[i].join();
        }


        update_terminal(statuses, pings, numThreads);
        // TODO: remove after update_terminal is implemented
        //for (std::size_t i = 0; i < numThreads; i++) {
            //std::cout << i << ": " << monitor->getAddresses()->at(i) << ": " << statuses[i] << ", " << pings[i] << "ms" << std::endl;
            //std::cout << dataWritePositions.at(i).first << ' ' << dataWritePositions.at(i).second << std::endl;
        //}

        // Sleep for the specified interval
        sleep(monitor->getInterval());
    }
}

void Terminal::update_terminal(long* statuses, long* pings, std::size_t num_threads) {
    for (std::size_t i = 0; i < dataWritePositions.size(); i++) {
        auto position_pair = dataWritePositions.at(i);

        std::cout << Term::cursor_move(position_pair.first, position_pair.second);
        std::cout << std::string(17, ' '); // Clear the old data from the screen
        std::cout << Term::cursor_move(position_pair.first, position_pair.second);
        std::cout << "HTTP " << statuses[i] << " | " << pings[i] << "ms";
    }

    std::cout << std::flush;
}

std::string Terminal::form_address_line() {
    std::string address_line;
    address_line.append("#");

    for (const std::string& address : *monitor->getAddresses()) {
        address_line.append("    ");

        std::size_t address_length = address.size();

        if (address_length < 17) { // The data to be printed out for each address can be at most 17 chars long
            std::string padded_address;
            padded_address.reserve(17);

            std::size_t beginning_length = (17 - address_length) / 2;
            padded_address.append(std::string(beginning_length, ' '));
            padded_address.append(address);

            std::size_t end_length = (17 - address_length) % 2 ? (17 - address_length) / 2 : (17 - address_length) / 2 + 1;
            padded_address.append(std::string(end_length, ' '));
            address_line.append(padded_address);
        } else {
            address_line.append(address);
        }

        address_line.append("    #");
    }

    return address_line;
}

std::vector<std::pair<std::size_t, std::size_t>> Terminal::calculate_data_write_positions(const std::string& address_line) {
    std::vector<std::pair<std::size_t, std::size_t>> returnVector;
    std::vector<std::string>* addresses = monitor->getAddresses();
    std::size_t numAddresses = addresses->size();
    std::size_t last_address_index = 0;

    for (std::size_t i = 0; i < numAddresses; i++) {
        std::size_t address_start_index = address_line.find(addresses->at(i), last_address_index + 1);
        last_address_index = address_start_index;

        // If the address is less than 17 chars long, the data will be shifted to the left relative
        // to its address using the formula: offset = (DATA_MAX_LENGTH - address length) / 2
        if (addresses->at(i).size() <= 17) {                                  // Example:
            std::size_t offset = (17 - addresses->at(i).size()) / 2;          //   www.google.com    (Length = 14)
            std::size_t rowIndex = 3;                                            //  HTTP 200 | 1000ms  (Max length = 17)
            std::size_t columnIndex = address_start_index - offset + 1;          // In this case: (17 - 14) / 2 = 1, so the data is shifted to the left by 1 char relative to its address
            returnVector.emplace_back(rowIndex, columnIndex);
        }
        else { // Otherwise, the data will be centered relative to its address
            std::size_t offset = (addresses->at(i).size() - 17) / 2;
            std::size_t rowIndex = 3;
            std::size_t columnIndex = address_start_index + offset + 1;
            returnVector.emplace_back(rowIndex, columnIndex);
        }
    }

    return returnVector;
}
