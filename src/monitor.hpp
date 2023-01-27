#ifndef PINGMONITOR_MONITOR_HPP
#define PINGMONITOR_MONITOR_HPP

#include <vector>
#include <cstdint>
#include <curl/curl.h>
#include <string>

class Monitor {
public:
    Monitor() = default;

    // Returns the current HTTP status of the CURL handle at addressIndex
    [[nodiscard]] int32_t get_http_status(std::size_t addressIndex) const;

    // Getters and setters
    std::vector<std::string>* getAddresses();
    std::vector<CURL*>* getHandles();
    [[nodiscard]] uint32_t getInterval() const;
    void setInterval(uint32_t newInterval);
private:
    // A list of website addresses used for printing on the terminal
    std::vector<std::string> addresses;

    // A list of CURL handles to use to get HTTP statuses from
    std::vector<CURL*> handles;

    // How long to wait before refreshing data, measured in seconds
    uint32_t interval = 5;
};

#endif //PINGMONITOR_MONITOR_HPP
