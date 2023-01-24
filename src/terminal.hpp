#ifndef PINGMONITOR_TERMINAL_HPP
#define PINGMONITOR_TERMINAL_HPP

#include <chrono>
#include <cpp-terminal/base.hpp>
#include "monitor.hpp"

class Terminal {
public:

    explicit Terminal(Monitor& monitor);

    void init();

    [[noreturn]] void run();

private:
    Monitor* monitor;
    std::vector<std::pair<std::size_t, std::size_t>> dataWritePositions;

    void update_terminal(long* statuses, long* pings, std::size_t num_threads);

    std::string form_address_line();

    std::vector<std::pair<std::size_t, std::size_t>> calculate_data_write_positions(const std::string& address_line);

    static void statusThread(Monitor& monitor, std::size_t address_index, long& status, long& duration) {
        auto start = std::chrono::high_resolution_clock::now();
        status = monitor.get_http_status(address_index);
        auto stop = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        duration = elapsed.count();
    }
};

#endif //PINGMONITOR_TERMINAL_HPP
