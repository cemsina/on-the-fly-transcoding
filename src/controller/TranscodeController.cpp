#include "TranscodeController.h"

using namespace otft;

TranscodeController::TranscodeController() { this->transcoder = new Transcoder(); }

TranscodeController::~TranscodeController() { delete this->transcoder; }

void TranscodeController::handleCombined(const httplib::Request &req,
                                         httplib::Response &res) const {
    auto base64EncodedInputUrl = req.matches[1].str();
    int bitrate = std::stoi(req.matches[2].str());
    int width = std::stoi(req.matches[3].str());
    int height = std::stoi(req.matches[4].str());
    int startAt = std::stoi(req.matches[5].str());
    int duration = std::stoi(req.matches[6].str());
    auto extension = req.matches[7].str();

    std::string inputUrl = base64url::decode(base64EncodedInputUrl);

    Job job;
    job.setUrl(inputUrl);
    job.setBitrate(bitrate);
    job.setWidth(width);
    job.setHeight(height);
    job.setStart(startAt);
    job.setDuration(duration);
    job.setAudioCodec(DEFAULT_AUDIO_CODEC);
    job.setVideoCodec(DEFAULT_VIDEO_CODEC);
    job.setFps(DEFAULT_FPS);
    job.setMedia(Media::COMBINED);

    std::string contentType = "video/";

    if (extension == "ts") {
        job.setFormat(Format::MPEGTS);
        contentType += "mp2t";
    } else if (extension == "mp4") {
        job.setFormat(Format::MPEG4);
        contentType += "mp4";
    }

    // contentType = "text/plain";  // testing

    auto result = this->transcoder->transcodeSegment(job);
    res.set_content(result.data(), result.size(), contentType);
}

void TranscodeController::handleVideoInit(const httplib::Request &req,
                                          httplib::Response &res) const {
    auto base64EncodedInputUrl = req.matches[1].str();
    int bitrate = std::stoi(req.matches[2].str());
    int width = std::stoi(req.matches[3].str());
    int height = std::stoi(req.matches[4].str());
    std::string inputUrl = base64url::decode(base64EncodedInputUrl);

    Job job;
    job.setUrl(inputUrl);
    job.setBitrate(bitrate);
    job.setWidth(width);
    job.setHeight(height);
    job.setStart(-1);
    job.setDuration(-1);
    job.setAudioCodec(DEFAULT_AUDIO_CODEC);
    job.setVideoCodec(DEFAULT_VIDEO_CODEC);
    job.setFps(DEFAULT_FPS);
    job.setFormat(Format::MPEG4);
    job.setMedia(Media::VIDEO);

    std::string contentType = "video/mp4";

    // contentType = "text/plain";  // testing

    auto result = this->transcoder->transcodeInit(job);
    res.set_content(result.data(), result.size(), contentType);
}

void TranscodeController::handleAudioInit(const httplib::Request &req,
                                          httplib::Response &res) const {
    auto base64EncodedInputUrl = req.matches[1].str();
    int bitrate = std::stoi(req.matches[2].str());

    std::string inputUrl = base64url::decode(base64EncodedInputUrl);

    Job job;
    job.setUrl(inputUrl);
    job.setBitrate(bitrate);
    job.setStart(-1);
    job.setDuration(-1);
    job.setAudioCodec(DEFAULT_AUDIO_CODEC);
    job.setVideoCodec(DEFAULT_VIDEO_CODEC);
    job.setFps(DEFAULT_FPS);
    job.setFormat(Format::MPEG4);
    job.setMedia(Media::AUDIO);

    std::string contentType = "audio/mp4";

    // contentType = "text/plain";  // testing

    auto result = this->transcoder->transcodeInit(job);
    res.set_content(result.data(), result.size(), contentType);
}

void TranscodeController::handleVideoSegment(const httplib::Request &req,
                                             httplib::Response &res) const {
    auto base64EncodedInputUrl = req.matches[1].str();
    int bitrate = std::stoi(req.matches[2].str());
    int width = std::stoi(req.matches[3].str());
    int height = std::stoi(req.matches[4].str());
    int startAt = std::stoi(req.matches[5].str());
    int duration = std::stoi(req.matches[6].str());

    std::string inputUrl = base64url::decode(base64EncodedInputUrl);

    Job job;
    job.setUrl(inputUrl);
    job.setBitrate(bitrate);
    job.setWidth(width);
    job.setHeight(height);
    job.setStart(startAt);
    job.setDuration(duration);
    job.setAudioCodec(DEFAULT_AUDIO_CODEC);
    job.setVideoCodec(DEFAULT_VIDEO_CODEC);
    job.setFps(DEFAULT_FPS);
    job.setFormat(Format::MPEG4);
    job.setMedia(Media::VIDEO);

    std::string contentType = "video/mp4";

    // contentType = "text/plain";  // testing

    auto result = this->transcoder->transcodeSegment(job);
    res.set_content(result.data(), result.size(), contentType);
}

void TranscodeController::handleAudioSegment(const httplib::Request &req,
                                             httplib::Response &res) const {
    auto base64EncodedInputUrl = req.matches[1].str();
    int bitrate = std::stoi(req.matches[2].str());
    int startAt = std::stoi(req.matches[3].str());
    int duration = std::stoi(req.matches[4].str());

    std::string inputUrl = base64url::decode(base64EncodedInputUrl);

    Job job;
    job.setUrl(inputUrl);
    job.setBitrate(bitrate);
    job.setStart(startAt);
    job.setDuration(duration);
    job.setAudioCodec(DEFAULT_AUDIO_CODEC);
    job.setVideoCodec(DEFAULT_VIDEO_CODEC);
    job.setFps(DEFAULT_FPS);
    job.setFormat(Format::MPEG4);
    job.setMedia(Media::VIDEO);

    std::string contentType = "video/mp4";

    // contentType = "text/plain";  // testing

    auto result = this->transcoder->transcodeSegment(job);
    res.set_content(result.data(), result.size(), contentType);
}