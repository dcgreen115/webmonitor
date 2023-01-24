#include <iostream>
#include <curl/curl.h>
#include <cstring>
#include <getopt.h>
#include <sstream>
#include "monitor.hpp"
#include "terminal.hpp"

// This function is used as a callback for libcurl so that it doesn't print HTTP responses to stdout
std::size_t write_data([[maybe_unused]] void* buffer, std::size_t size, std::size_t nmemb, [[maybe_unused]] void* userp) {
    return size * nmemb;
}

void parse_args(Monitor& monitor, int argc, char* argv[]) {
    struct option long_options[] = {
            {"help", no_argument, nullptr, 'h'},
            {"address", required_argument, nullptr, 'a'},
            {"interval", required_argument, nullptr, 'i'},
            {nullptr, 0, nullptr, 0}
            };

    while(true) {
        int short_option = getopt_long(argc, argv, "ha:i:", long_options, nullptr);

        if (short_option == -1) return; // Reached end of arguments

        switch(short_option) {
            case 'h': // Help
                std::cout << "Help text" << std::endl; //TODO: Add help text
                break;
            case 'a': // Address
            {
                monitor.getAddresses()->push_back(optarg);
                CURL* handle = curl_easy_init();
                curl_easy_setopt(handle, CURLOPT_URL, optarg); // Set the URL to the current address argument
                curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data); // Custom callback function prevents CURL output from being written to the console
                curl_easy_setopt(handle, CURLOPT_TIMEOUT, 10); // Times out at 10 seconds
                monitor.getHandles()->push_back(handle);
            }
                break;
            case 'i': // Interval
            {
                std::stringstream ss(optarg);
                int i;
                ss >> i;
                monitor.setInterval(i);
                break;
            }
            case '?': // Something other than the specified options
                throw std::invalid_argument("");
            default:
                break;
        }
    }
}

int main(int argc, char* argv[]) {

    // If no arguments were entered, show how to use the "help" option
    if (argc == 1) {
        std::cout << "Use 'pingmonitor --help' for more information" << std::endl;
        return 0;
    }

    // Set up the monitor using the arguments provided
    Monitor monitor;
    try {
        parse_args(monitor, argc, argv);
    } catch (std::invalid_argument&) {
        return 1;
    }

    // Take control of the terminal this program is run in and start displaying
    // data from the monitor
    Terminal terminal(monitor);
    terminal.init();
    terminal.run();
}