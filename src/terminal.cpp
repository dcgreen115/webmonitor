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


    std::cout << Term::clear_screen() << Term::cursor_move(0, 0);
    std::cout << std::string(address_line.length(), '#') << '\n';
    std::cout << address_line << '\n';
    std::cout << std::string(address_line.length(), '#') << '\n';

}

[[noreturn]] void Terminal::run() {

    while (true) {

        // Set up arrays to hold threads and their return values
        std::size_t numThreads = monitor->getAddresses()->size();
        std::thread statusThreads[numThreads];
        long statuses[numThreads];
        long pings[numThreads];

        std::cout << "Starting threads" << std::endl;
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
        for (std::size_t i = 0; i < numThreads; i++) {
            std::cout << i << ": " << monitor->getAddresses()->at(i) << ": " << statuses[i] << ", " << pings[i] << "ms" << std::endl;
        }

        // Sleep for the specified interval
        sleep(monitor->getInterval());
    }
}

void Terminal::update_terminal(long* statuses, long* pings, std::size_t num_threads) {
    // TODO
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

void Terminal::calculate_data_write_positions() {
    std::size_t numAddresses = monitor->getAddresses()->size();
    dataWritePositions.reserve(numAddresses);
}
