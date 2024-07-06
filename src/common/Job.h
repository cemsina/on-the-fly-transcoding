#pragma once

#include <iostream>
#include <string>
#include "Media.h"
#include "Format.h"

namespace otft {
    class Job {
      private:
        std::string url;
        int bitrate = 0;
        int width = 0;
        int height = 0;
        float fps = 0;
        float start = 0;
        float duration = 0;
        Media media;
        Format format;
        std::string videoCodec;
        std::string audioCodec;

      public:
        void setUrl(const std::string &url);
        void setBitrate(int bitrate);
        void setWidth(int width);
        void setHeight(int height);
        void setFps(float fps);
        void setStart(float start);
        void setDuration(float duration);
        void setMedia(const Media &media);
        void setFormat(const Format &format);
        void setVideoCodec(const std::string &videoCodec);
        void setAudioCodec(const std::string &audioCodec);

        // Getters for accessing the values if needed
        const std::string &getUrl() const;
        int getBitrate() const;
        int getWidth() const;
        int getHeight() const;
        float getFps() const;
        float getStart() const;
        float getDuration() const;
        const Media &getMedia() const;
        const Format &getFormat() const;
        const std::string &getVideoCodec() const;
        const std::string &getAudioCodec() const;
    };
}  // namespace otft
