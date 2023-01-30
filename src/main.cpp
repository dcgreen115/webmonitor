#include <curl/curl.h>
#include <getopt.h>
#include <iostream>
#include <cstring>
#include <sstream>
#include <csignal>
#include "monitor.hpp"
#include "terminal.hpp"

sig_atomic_t volatile running = true;

// This function is used as a callback for libcurl so that it doesn't print
// HTTP responses to stdout
std::size_t write_data([[maybe_unused]] void* buffer, std::size_t size,
                       std::size_t nmemb, [[maybe_unused]] void* userp) {
    return size * nmemb;
}

/**
 * Uses getopt to parse program arguments
 * @param monitor The Monitor object that will hold the data parsed from
 * the program arguments
 * @param argc The argc passed in to the main function
 * @param argv The argv passed in to the main function
 */
void parse_args(Monitor* monitor, const int argc, char* const argv[]) {
    struct option long_options[] = {
            {"help", no_argument, nullptr, 'h'},
            {"address", required_argument, nullptr, 'a'},
            {"interval", required_argument, nullptr, 'i'},
            {nullptr, 0, nullptr, 0}
            };

    while (true) {
        int short_option = getopt_long(argc, argv, "ha:i:", long_options, nullptr);

        if (short_option == -1) return;  // Reached end of arguments

        switch (short_option) {
            case 'h':  // Help
                std::cout << "Usage: webmonitor [options]\n"
                             "Options:\n"
                             "  -h, --help            Display the help text.\n"
                             "  -a, --address <URL>   Set a URL to monitor.\n"
                             "  -i, --interval <time> Set the refresh interval in seconds."
                << std::endl;
                exit(0);
                break;
            case 'a':  // Address
            {
                monitor->getAddresses()->push_back(optarg);
                CURL* handle = curl_easy_init();
                curl_easy_setopt(handle, CURLOPT_URL, optarg);  // Set the URL to the current address argument
                curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);  // Prevents CURL from writing to stdout
                curl_easy_setopt(handle, CURLOPT_TIMEOUT, 5);  // Times out at 5 seconds
                monitor->getHandles()->push_back(handle);
            }
                break;
            case 'i':  // Interval
            {
                std::stringstream ss(optarg);  // Create a stringstream to extract the number passed in
                int i;
                ss >> i;

                if (ss.fail()) {
                    throw std::invalid_argument("The entered interval is not a number");
                } else if (i < 1) {
                    throw std::invalid_argument("Interval must be at least 1 second");
                } else {
                    monitor->setInterval(i);
                }

                break;
            }
            case '?':  // Something other than the specified options
                throw std::invalid_argument("");
            default:
                break;
        }
    }
}

// Signals the main Terminal loop to stop running so that the program exits cleanly
void sig_handler([[maybe_unused]] int signum) {
    running = false;
}

// Set sig_handler to run when the user enters Ctrl+C or the kill command
void register_signal_handlers() {
    signal(SIGINT, &sig_handler);
    signal(SIGTERM, &sig_handler);
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
        parse_args(&monitor, argc, argv);
    } catch (std::invalid_argument& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }

    if (monitor.getAddresses()->empty()) {
        std::cout << "Program requires one or more addresses to monitor." << std::endl;
        return 1;
    }

    // Register signal handlers so that the program exits cleanly
    register_signal_handlers();

    // Take control of the terminal this program is run in and start displaying data from the monitor
    Terminal terminal(&monitor);
    terminal.run();
}
