#ifndef SRC_TERMINAL_HPP_
#define SRC_TERMINAL_HPP_

#include <chrono>
#include <string>
#include <utility>
#include <vector>
#include <cpp-terminal/base.hpp>
#include "./monitor.hpp"

class Terminal {
 public:
    // A constructor that uses a monitor to take data from
    explicit Terminal(Monitor* monitor);

    // Continuously gets new data from the monitor and updates data
    // in the terminal window
    void run();

 private:
    // The row that data will be printed on within the terminal
    static constexpr std::size_t DATA_ROW_INDEX = 3;
    // The maximum length that data can have - "HTTP 200 | 1000ms"
    static constexpr std::size_t DATA_MAX_LENGTH = 17;

    // The monitor from which HTTP status and time-to-last-byte data is
    // obtained from
    Monitor* monitor;

    // A list holding the indices that data will be written to on the terminal
    std::vector<std::pair<std::size_t, std::size_t>> data_write_positions;

    // Sets up the terminal view-box
    void init();

    // Gets new data from the monitor and updates the terminal
    void update_terminal(const std::vector<int64_t>& statuses, const std::vector<int64_t>& times) const;

    // Creates the address line that will be printed on the terminal,
    // with centered text
    std::string form_address_line();

    // Finds the positions in the terminal that data should be written at
    [[nodiscard]] std::vector<std::pair<std::size_t, std::size_t>>
        calculate_data_positions(std::string_view address_line) const;

    // A thread that will get the current HTTP status and time-to-last-byte of
    // a website contained in the monitor
    static void statusThread(Monitor* monitor, const std::size_t address_index, int64_t* status, int64_t* duration) {
        auto start = std::chrono::high_resolution_clock::now();
        *status = monitor->get_http_status(address_index);
        auto stop = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        *duration = elapsed.count();
    }
};

#endif  // SRC_TERMINAL_HPP_
