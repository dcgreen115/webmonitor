# Webmonitor
`Webmonitor` is a small and simple command line application for monitoring websites 
entered by the user. It shows the current HTTP status and time-to-last-byte of each
website being monitored, and supports an unlimited number of URLs as input. Webmonitor
is currently confirmed working on Linux operating systems and is expected to be fully
working on Windows and MacOS.

## Program Arguments
`Webmonitor` expects the following command line arguments:\
`-h, --help` Displays this help text.\
`-a, --address` Sets a URL to monitor. The program requires at least one URL and will work even if the entered URLs are malformed.\
`-i, --interval` Sets how often the program should refresh its data, measured in seconds. The entered interval must be greater than or equal to 1 second. Default is 5 seconds.

## Examples
`webmonitor -a www.google.com`: Sets a single monitored website with the default refresh interval\
<img src="images/singleURL.png" alt="Terminal Output For Single URL"/>\
`webmonitor -a www.google.com -a https://github.com -a greentechnologies.info`: Sets multiple monitored websites with the default refresh interval\
<img src="images/multipleURLs.png" alt="Terminal Output For Multiple URLs"/>\
`webmonitor -a www.google.com -i 30`: Sets the refresh interval to 30 seconds

## License
Webmonitor is licensed under the terms of [the MIT License](https://github.com/dcgreen115/webmonitor/blob/master/LICENSE) by Dylan Green