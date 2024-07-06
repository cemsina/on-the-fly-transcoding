
#include <iostream>

#include "net/http.h"
#include "controller/TranscodeController.h"

int main() {
    httplib::Server server;

    const char *portEnv = std::getenv("PORT");
    std::string port;
    if (portEnv == nullptr) {
        std::cout << "Environment variable PORT not set. Using default port 8080." << std::endl;
        port = "8080";
    } else {
        port = portEnv;
    }

    otft::TranscodeController *transcodeController = new otft::TranscodeController();

    server.Get(R"(/([^/]+)/combined/(\d+)/(\d+)x(\d+)/(\d+):(\d+)/segment\.(ts|mp4))",
               std::bind(&otft::TranscodeController::handleCombined, transcodeController,
                         std::placeholders::_1, std::placeholders::_2));
    server.Get(R"(/([^/]+)/combined/(\d+)/(\d+)x(\d+)/(\d+):(\d+)/segment\.mp4)",
               std::bind(&otft::TranscodeController::handleCombined, transcodeController,
                         std::placeholders::_1, std::placeholders::_2));
    server.Get(R"(/([^/]+)/video/(\d+)/(\d+)x(\d+)/init\.mp4)",
               std::bind(&otft::TranscodeController::handleVideoInit, transcodeController,
                         std::placeholders::_1, std::placeholders::_2));
    server.Get(R"(/([^/]+)/audio/(\d+)/init\.mp4)",
               std::bind(&otft::TranscodeController::handleAudioInit, transcodeController,
                         std::placeholders::_1, std::placeholders::_2));
    server.Get(R"(/([^/]+)/video/(\d+)/(\d+)x(\d+)/(\d+):(\d+)/segment\.mp4)",
               std::bind(&otft::TranscodeController::handleVideoSegment, transcodeController,
                         std::placeholders::_1, std::placeholders::_2));
    server.Get(R"(/([^/]+)/audio/(\d+)/(\d+):(\d+)/segment\.mp4)",
               std::bind(&otft::TranscodeController::handleAudioSegment, transcodeController,
                         std::placeholders::_1, std::placeholders::_2));

    server.listen("0.0.0.0", std::stoi(port));
    return 0;
}