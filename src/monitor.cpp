#include <iostream>
#include "monitor.hpp"

long Monitor::get_http_status(std::size_t addressIndex) {
    CURLcode code = curl_easy_perform(handles.at(addressIndex));

    // If the connection failed, immediately return
    if (code != CURLE_OK) {
        return -1;
    }

    // Otherwise, return the HTTP response code
    long http_code = 0;
    curl_easy_getinfo(handles.at(addressIndex), CURLINFO_RESPONSE_CODE, &http_code);
    return http_code;
}

// Returns a reference to this monitor's address list
std::vector<std::string>* Monitor::getAddresses() {
    return &addresses;
}

// Returns a reference to this monitor's CURL handle list
std::vector<CURL*>* Monitor::getHandles() {
    return &handles;
}

uint32_t Monitor::getInterval() const {
    return interval;
}

void Monitor::setInterval(uint32_t newInterval) {
    interval = newInterval;
}



