#pragma once

#include <iostream>
#include "../common/Job.h"
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
}

namespace otft {
    class Transcoder {
      public:
        Transcoder();
        ~Transcoder();
        std::vector<char> transcodeSegment(const Job &job) const;
        std::vector<char> transcodeInit(const Job &job) const;
    };
};  // namespace otft