#include <iostream>
#include <thread>
//#include <ncurses.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <regex>
#include <string>
#include <fstream>
#include <fcntl.h>
#include <chrono>
#include <thread>

#include "Simple-Web-Server-master/client_http.hpp"
#include "Simple-Web-Server-master/server_http.hpp"

#include "popen2.cpp"


void printNchars(int n, char c) {
    for (int i = 0; i < n; i++) {
        std::cout << c;
    }
}

/**
 * Overloaded variant to accept multibyte UTF-8 characters
 */
void printNchars(int n, std::string c) {
    for (int i = 0; i < n; i++) {
        std::cout << c;
    }
}


void getStackAndHeap(int pid, unsigned long long int& _stack, unsigned long long int& _heap) {
    FILE *f;
    unsigned long long int low;
    unsigned long long int high;
    char filepath[32];
    char lineBuf[1000];
    sprintf(filepath, "/proc/%d/maps", pid);
    f = fopen(filepath, "r");
    if (f) {
        while (NULL != fgets(lineBuf, 1000, f)) {
            if (strstr(lineBuf, "heap")) {
                sscanf(lineBuf, "%llx-%llx", &low, &high);
                _heap = high - low;
            } else if (strstr(lineBuf, "stack")) {
                sscanf(lineBuf, "%llx-%llx", &low, &high);
                _stack = high - low;
            }
        }
    }
}

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
unsigned long long int stack = 0;
unsigned long long int heap = 0;

int main(int argc,  char **argv) {
    // Parse Arguments
    if (argc < 2) {
        std::cout << "proc_monitor Usage: ..." << std::endl;
        return 1;
    }
    int opt;
    bool gFlag = false; // Host web server to show graphs if set
    bool pFlag = false; // Monitor process instead of command
    // Shut GetOpt error messages down (return '?'):
    opterr = 0;
    while ((opt = getopt(argc, argv, "gp")) != -1) {  // for each option...
        switch (opt) {
            case 'g':
                gFlag = true;
                break;
            case 'p':
                pFlag = true;
                break;
            case '?':  // unknown option...
                std::cout << "Unknown option: '" << char(optopt) << "'!" << std::endl;
                return 1;
        }
    }

    std::string cmd_args;
    for (int i = 1; i < argc; i++) {
        cmd_args.append(std::string(argv[i])).append(" ");
    }
    if (gFlag)
        cmd_args = std::regex_replace(cmd_args, std::regex("\\-g "), "");
    if (pFlag)
        cmd_args = std::regex_replace(cmd_args, std::regex("\\-p "), "");


    // Configure the local web server if -g flag is present
    HttpServer server;
    std::promise<unsigned short> server_port;

    if (gFlag) {
        server.config.port = 8080;

        server.resource["^/proc_monitor$"]["GET"] = [cmd_args](std::shared_ptr <HttpServer::Response> response,
                                                               std::shared_ptr <HttpServer::Request> request) {
            std::stringstream stream;

            // Copy-pasted html from proc_monitor.html in lieu of compile-time link
            stream << "<!DOCTYPE html>\n"
                      "<html lang=\"en\">\n"
                      "<head>\n"
                      "    <meta charset=\"UTF-8\">\n"
                      "    <title>Proc Monitor</title>\n"
                      "    <script src=\"https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.9.4/Chart.js\"></script>\n"
                      "</head>\n"
                      "<body>\n"
                      "    <h1>Proc Monitor</h1>\n"
                      "    <div>\n"
                      "        Monitoring: <code>" << cmd_args << "</code>\n"
                      "    </div>\n"
                      "    <div class=\"stack_graph\">\n"
                      "            <canvas id=\"StackGraph\" width=360 height=180></canvas>\n"
                      "        </div>\n"
                      "    <div class=\"heap_graph\">\n"
                      "        <canvas id=\"HeapGraph\" width=360 height=180></canvas>\n"
                      "    </div>\n"
                      "    <div>\n"
                      "        Stack Usage: <code id=\"stack_indicator\">0</code>\n"
                      "    </div>\n"
                      "    <div>\n"
                      "        Heap Usage: <code id=\"heap_indicator\">0</code>\n"
                      "    </div>\n"
                      "    <script>\n"
                      "\n"
                      "        let stack_graph, heap_graph;\n"
                      "        let time = [];\n"
                      "        let stack_y = [];\n"
                      "        let heap_y = [];\n"
                      "        let t0 = 0; // First timestamp\n"
                      "        function getData() {\n"
                      "            const url = 'http://localhost:8080/proc_monitor/data';\n"
                      "            fetch(url)\n"
                      "                .then(response => response.json())\n"
                      "                .then(json => {\n"
                      "                    console.log(json);\n"
                      "                    if (t0 === 0) {\n"
                      "                        t0 = json[\"time\"];\n"
                      "                    }\n"
                      "                    time.push(json[\"time\"] - t0);\n"
                      "                    if (stack_graph) {\n"
                      "                        stack_graph.destroy()\n"
                      "                    }\n"
                      "                    stack_graph = generate_stack_graph(json)\n"
                      "                    if (heap_graph) {\n"
                      "                        heap_graph.destroy()\n"
                      "                        // freq_chart.width = \"400px\"\n"
                      "                    }\n"
                      "                    heap_graph = generate_heap_graph(json)\n"
                      "                })\n"
                      "        }\n"
                      "        function generate_stack_graph(json) {\n"
                      "            const decimals = 2;\n"
                      "            // Chart.defaults.global.defaultFontColor = \"#FFF\";\n"
                      "            stack_y.push(json[\"stack_y\"])\n"
                      "            document.getElementById(\"stack_indicator\").innerText = json[\"stack_y\"];\n"
                      "            return new Chart(\"StackGraph\", {\n"
                      "                type: \"line\",\n"
                      "                data: {\n"
                      "                    labels: time,\n"
                      "                    datasets: [{\n"
                      "                        backgroundColor: 'rgba(0, 169, 0, 0.25)',\n"
                      "                        borderColor: 'rgba(0, 255, 0, 0.5)',\n"
                      "                        data: stack_y,\n"
                      "                        borderWidth: 2,\n"
                      "                        borderRadius: 2,\n"
                      "                    }]\n"
                      "                },\n"
                      "                options: {\n"
                      "                    animation: {\n"
                      "                        duration: 0\n"
                      "                    },\n"
                      "                    legend: {display: false},\n"
                      "                    title: {\n"
                      "                        display: true,\n"
                      "                        text: \"Stack Usage\"\n"
                      "                    },\n"
                      "                    responsive: false,\n"
                      "                    scales: {\n"
                      "                        xAxes: [{\n"
                      "                            ticks: {\n"
                      "                                fontSize: 9\n"
                      "                            },\n"
                      "                            gridLines: {\n"
                      "                                display: false,\n"
                      "                                color: \"#949494\"\n"
                      "                            },\n"
                      "                            scaleLabel: {\n"
                      "                                display: true,\n"
                      "                                labelString: \"Time (s)\"\n"
                      "                            }\n"
                      "                        }],\n"
                      "                        yAxes: [{\n"
                      "                            ticks: {\n"
                      "                                beginAtZero: true,\n"
                      "                                callback: function(value, index, values) {\n"
                      "                                    return value.toFixed(decimals) + \" bytes\";\n"
                      "                                }\n"
                      "                            },\n"
                      "                            gridLines: {\n"
                      "                                display: true,\n"
                      "                                color: \"#949494\"\n"
                      "                            },\n"
                      "                        }]\n"
                      "                    }\n"
                      "                }\n"
                      "            });\n"
                      "        }\n"
                      "\n"
                      "        function generate_heap_graph(json) {\n"
                      "            const decimals = 2;\n"
                      "            // Chart.defaults.global.defaultFontColor = \"#FFF\";\n"
                      "            heap_y.push(json[\"heap_y\"])\n"
                      "            document.getElementById(\"heap_indicator\").innerText = json[\"heap_y\"];\n"
                      "            return new Chart(\"HeapGraph\", {\n"
                      "                type: \"line\",\n"
                      "                data: {\n"
                      "                    labels: time,\n"
                      "                    datasets: [{\n"
                      "                        backgroundColor: 'rgba(169, 0, 0, 0.25)',\n"
                      "                        borderColor: 'rgba(255, 0, 0, 0.5)',\n"
                      "                        data: heap_y,\n"
                      "                        borderWidth: 2,\n"
                      "                        borderRadius: 2,\n"
                      "                    }]\n"
                      "                },\n"
                      "                options: {\n"
                      "                    animation: {\n"
                      "                        duration: 0\n"
                      "                    },\n"
                      "                    legend: {display: false},\n"
                      "                    title: {\n"
                      "                        display: true,\n"
                      "                        text: \"Heap Usage\"\n"
                      "                    },\n"
                      "                    responsive: false,\n"
                      "                    scales: {\n"
                      "                        xAxes: [{\n"
                      "                            ticks: {\n"
                      "                                fontSize: 9\n"
                      "                            },\n"
                      "                            gridLines: {\n"
                      "                                display: false,\n"
                      "                                color: \"#949494\"\n"
                      "                            },\n"
                      "                            scaleLabel: {\n"
                      "                                display: true,\n"
                      "                                labelString: \"Time (s)\"\n"
                      "                            }\n"
                      "                        }],\n"
                      "                        yAxes: [{\n"
                      "                            ticks: {\n"
                      "                                beginAtZero: true,\n"
                      "                                callback: function(value, index, values) {\n"
                      "                                    return value.toFixed(decimals) + \" bytes\";\n"
                      "                                }\n"
                      "                            },\n"
                      "                            gridLines: {\n"
                      "                                display: true,\n"
                      "                                color: \"#949494\"\n"
                      "                            },\n"
                      "                        }]\n"
                      "                    }\n"
                      "                }\n"
                      "            });\n"
                      "        }\n"
                      "        const interval = setInterval(getData, 1000);\n"
                      "    </script>\n"
                      "</body>\n"
                      "</html>";

            response->write(stream);
        };

        // Feed the graphs json data via a get request from the front-end
        server.resource["^/proc_monitor/data$"]["GET"] = [](std::shared_ptr <HttpServer::Response> response,
                                                            std::shared_ptr <HttpServer::Request> request) {
            const auto time = std::chrono::system_clock::now();
            int secs = std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch()).count();
            std::string json = "{\"time\": " + std::to_string(secs) + ", "
                              "\"stack_y\": " + std::to_string(stack) + ", "
                              "\"heap_y\": " + std::to_string(heap) + "}";
            response->write(json);
        };


        server.on_error = [](std::shared_ptr <HttpServer::Request> /*request*/, const SimpleWeb::error_code & /*ec*/) {
            // Handle errors here
            // Note that connection timeouts will also call this handle with ec set to SimpleWeb::errc::operation_canceled
        };

        // Start server and receive assigned port when server is listening for requests
        std::thread server_thread([&server, &server_port]() {
            server.start([&server_port](unsigned short port) {
                server_port.set_value(port);
            });
        });
        server_thread.detach();
        std::cout << "Server listening on port " << server_port.get_future().get() << "\n\n";

        // Open the web page in the user's preferred browser
        system("xdg-open \"http://localhost:8080/proc_monitor\"");
    }

    if (pFlag) {
        int pid;
        std::cout << cmd_args << "\n";
        try {
            pid = stoi(cmd_args);
        } catch (std::invalid_argument) {
            std::cout << "Argument is not an integer and therefore not a valid PID\n";
            return 1;
        }
        while(1) {
            std::cout << "\x1b[1A\x1b[2K";       // Delete the stats

            getStackAndHeap(pid, stack, heap);

            std::cout << "      Stack Size: " << stack << "   Heap Size: " << heap << " \n";

            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    } else {
        // Run the command and display std out in formatted box
        std::cout << "\nStarting: " << cmd_args << std::endl;
        std::cout << "\nstdout:\n";

        const int max_buffer = 1024;
        char buffer[max_buffer];

        // Run the command
        int pid = 0;
        FILE *stream = popen2(cmd_args, "r", pid);

        // Mark the file descriptor as nonblocking preventing fgets() from blocking
        int fd = fileno(stream);
        int flags = fcntl(fd, F_GETFL, 0);
        flags |= O_NONBLOCK;
        fcntl(fd, F_SETFL, flags);

        struct winsize w;
        if (stream) {
            ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

            // Draw box top
            std::cout << "╔";
            printNchars(w.ws_col - 2, "═");
            std::cout << "╗\n║";
            printNchars(w.ws_col - 2, ' ');
            std::cout << "║\n║";
            printNchars(w.ws_col - 2, ' ');
            std::cout << "║\n";

            // Draw box bottom and Statistics
            std::cout << "╚";
            printNchars(w.ws_col - 2, "═");
            std::cout << "╝\n      Stack Size: ...   Heap Size: ... ";

            int lineLength = 5;
            while (!feof(stream)) {
                getStackAndHeap(pid, stack, heap);


                ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
                // Erase Statistics and bottom box
                std::cout << "\x1b[1A\x1b[2K";       // Delete the stats
                std::cout << "\x1b[1A\x1b[2K";       // Delete the bottom box line

                // Check the stdout at 5 Hz
                if (fgets(buffer, max_buffer, stream) != NULL) {
                    std::cout << "\x1b[1A\x1b[2K";   // Delete the bottom margin line if stdout is present
                    // Draw "Terminal" Contents
                    std::cout << "║    ";
                    for (int i = 0; i < max_buffer && buffer[i] != 0; i++) {
                        if (buffer[i] == '\n') {
                            printNchars(w.ws_col - lineLength - 1, ' ');
                            std::cout << "║\n║";
                            printNchars(w.ws_col - 2, ' ');
                            std::cout << "║\n";
                            lineLength = 5;
                        } else {
                            std::cout << buffer[i];
                            lineLength++;
                        }
                    }
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                }

                // Draw box bottom and Statistics
                std::cout << "╚";
                printNchars(w.ws_col - 2, "═");
                std::cout << "╝\n      Stack Size: " << stack << "   Heap Size: " << heap << " \n";
            }
            pclose2(stream, pid);
        }
        std::cout << "Done\n";
    }
    return 0;
}
