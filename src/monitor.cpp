#include <iostream>
#include "monitor.hpp"

Monitor::~Monitor() {
    for (auto handle : handles) {
        curl_easy_cleanup(handle);
    }
}

/**
 * Gets the current HTTP status of the CURL handle at the specified index
 * @param addressIndex The index of the CURL handle to use
 * @return The current HTTP code of the specified CURL handle
 */
int32_t Monitor::get_http_status(std::size_t addressIndex) const {
    CURLcode code = curl_easy_perform(handles.at(addressIndex));

    // If the connection failed, immediately return
    if (code != CURLE_OK) {
        return -1;
    }

    // Otherwise, return the HTTP response code
    int32_t http_code = 0;
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

// Returns this monitor's refresh interval
uint32_t Monitor::getInterval() const {
    return interval;
}

// Sets this monitor's refresh interval
void Monitor::setInterval(const uint32_t newInterval) {
    interval = newInterval;
}
