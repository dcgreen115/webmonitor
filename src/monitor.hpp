// monitor.hpp - Declares methods used to get the HTTP status and ping of websites
// Author: Dylan Green
// Version: 2022.12.15

#ifndef PINGMONITOR_MONITOR_HPP
#define PINGMONITOR_MONITOR_HPP

#include <vector>
#include <cstdint>
#include <curl/curl.h>
#include <string>

class Monitor {
public:

    Monitor() = default;

    long get_http_status(std::size_t addressIndex);

    std::vector<std::string>* getAddresses();

    std::vector<CURL*>* getHandles();

    [[nodiscard]] uint32_t getInterval() const;

    void setInterval(uint32_t newInterval);
private:
    std::vector<std::string> addresses;
    std::vector<CURL*> handles;
    uint32_t interval = 5;

};

#endif //PINGMONITOR_MONITOR_HPP
