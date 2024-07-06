#pragma once

#include "../net/http.h"
#include "../common/Job.h"
#include "../utils/base64url.h"
#include "../processor/Transcoder.h"

#include "../predefined.h"

namespace otft {
    class TranscodeController {
      private:
        Transcoder *transcoder;

      public:
        TranscodeController();
        ~TranscodeController();
        // add request headers here
        void handleCombined(const httplib::Request &req, httplib::Response &res) const;
        void handleVideoInit(const httplib::Request &req, httplib::Response &res) const;
        void handleAudioInit(const httplib::Request &req, httplib::Response &res) const;
        void handleVideoSegment(const httplib::Request &req, httplib::Response &res) const;
        void handleAudioSegment(const httplib::Request &req, httplib::Response &res) const;
    };
};  // namespace otft