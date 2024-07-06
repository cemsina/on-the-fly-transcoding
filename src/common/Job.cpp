#include "Job.h"

namespace otft {
    void Job::setUrl(const std::string& url) {
        this->url = url;
    }

    void Job::setBitrate(int bitrate) {
        this->bitrate = bitrate;
    }

    void Job::setWidth(int width) {
        this->width = width;
    }

    void Job::setHeight(int height) {
        this->height = height;
    }

    void Job::setFps(float fps) {
        this->fps = fps;
    }

    void Job::setStart(float start) {
        this->start = start;
    }

    void Job::setDuration(float duration) {
        this->duration = duration;
    }

    void Job::setMedia(const Media& media) {
        this->media = media;
    }

    void Job::setFormat(const Format& format) {
        this->format = format;
    }

    void Job::setVideoCodec(const std::string& videoCodec) {
        this->videoCodec = videoCodec;
    }

    void Job::setAudioCodec(const std::string& audioCodec) {
        this->audioCodec = audioCodec;
    }

    const std::string& Job::getUrl() const {
        return url;
    }

    int Job::getBitrate() const {
        return bitrate;
    }

    int Job::getWidth() const {
        return width;
    }

    int Job::getHeight() const {
        return height;
    }

    float Job::getFps() const {
        return fps;
    }

    float Job::getStart() const {
        return start;
    }

    float Job::getDuration() const {
        return duration;
    }

    const Media& Job::getMedia() const {
        return media;
    }

    const Format& Job::getFormat() const {
        return format;
    }

    const std::string& Job::getVideoCodec() const {
        return videoCodec;
    }

    const std::string& Job::getAudioCodec() const {
        return audioCodec;
    }
}  // namespace otft
