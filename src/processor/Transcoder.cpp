#include "Transcoder.h"

using namespace otft;

Transcoder::Transcoder() {
    // Register all codecs and formats if needed by latest FFmpeg
}

Transcoder::~Transcoder() {}

std::vector<char> Transcoder::transcodeSegment(const Job &job) const {
    std::vector<char> outputBuffer;

    // Initialize FFmpeg components
    AVFormatContext *inputFormatContext = nullptr;
    AVFormatContext *outputFormatContext = nullptr;
    AVCodecContext *videoDecoderContext = nullptr;
    AVCodecContext *audioDecoderContext = nullptr;
    AVCodecContext *videoEncoderContext = nullptr;
    AVCodecContext *audioEncoderContext = nullptr;
    AVStream *videoStream = nullptr;
    AVStream *audioStream = nullptr;
    int videoStreamIndex = -1;
    int audioStreamIndex = -1;

    // Open input file
    if (avformat_open_input(&inputFormatContext, job.getUrl().c_str(), nullptr, nullptr) < 0) {
        std::cerr << "Could not open input file" << std::endl;
        return outputBuffer;
    }

    // Find stream info
    if (avformat_find_stream_info(inputFormatContext, nullptr) < 0) {
        std::cerr << "Could not find stream info" << std::endl;
        avformat_close_input(&inputFormatContext);
        return outputBuffer;
    }

    // Find video and audio stream indices
    for (unsigned int i = 0; i < inputFormatContext->nb_streams; ++i) {
        if (inputFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
        } else if (inputFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
        }
    }

    // Seek to the start position
    int64_t startTime = job.getStart() * AV_TIME_BASE;
    if (av_seek_frame(inputFormatContext, -1, startTime, AVSEEK_FLAG_BACKWARD) < 0) {
        std::cerr << "Error seeking to start position" << std::endl;
        avformat_close_input(&inputFormatContext);
        return outputBuffer;
    }

    // Create output format context
    const char *outputFormat = job.getFormat() == Format::MPEGTS ? "mpegts" : "mp4";
    avformat_alloc_output_context2(&outputFormatContext, nullptr, outputFormat, nullptr);
    if (!outputFormatContext) {
        std::cerr << "Could not create output context" << std::endl;
        avformat_close_input(&inputFormatContext);
        return outputBuffer;
    }

    // Add streams to output format context
    if (videoStreamIndex >= 0 &&
        (job.getMedia() == Media::VIDEO || job.getMedia() == Media::COMBINED)) {
        videoStream = avformat_new_stream(outputFormatContext, nullptr);
        if (!videoStream) {
            std::cerr << "Failed to allocate video stream" << std::endl;
            avformat_close_input(&inputFormatContext);
            avformat_free_context(outputFormatContext);
            return outputBuffer;
        }
        AVRational time_base = av_d2q(job.getFps(), 100000);
        videoStream->time_base = time_base;
    }
    if (audioStreamIndex >= 0 &&
        (job.getMedia() == Media::AUDIO || job.getMedia() == Media::COMBINED)) {
        audioStream = avformat_new_stream(outputFormatContext, nullptr);
        if (!audioStream) {
            std::cerr << "Failed to allocate audio stream" << std::endl;
            avformat_close_input(&inputFormatContext);
            avformat_free_context(outputFormatContext);
            return outputBuffer;
        }
        audioStream->time_base = (AVRational){1, 48000};  // Example sample rate
    }

    // Open input codecs
    if (videoStreamIndex >= 0) {
        const AVCodec *videoDecoder =
            avcodec_find_decoder(inputFormatContext->streams[videoStreamIndex]->codecpar->codec_id);
        videoDecoderContext = avcodec_alloc_context3(videoDecoder);
        avcodec_parameters_to_context(videoDecoderContext,
                                      inputFormatContext->streams[videoStreamIndex]->codecpar);
        if (avcodec_open2(videoDecoderContext, videoDecoder, nullptr) < 0) {
            std::cerr << "Could not open video decoder" << std::endl;
            avformat_close_input(&inputFormatContext);
            avformat_free_context(outputFormatContext);
            avcodec_free_context(&videoDecoderContext);
            return outputBuffer;
        }
    }
    if (audioStreamIndex >= 0) {
        const AVCodec *audioDecoder =
            avcodec_find_decoder(inputFormatContext->streams[audioStreamIndex]->codecpar->codec_id);
        audioDecoderContext = avcodec_alloc_context3(audioDecoder);
        avcodec_parameters_to_context(audioDecoderContext,
                                      inputFormatContext->streams[audioStreamIndex]->codecpar);
        if (avcodec_open2(audioDecoderContext, audioDecoder, nullptr) < 0) {
            std::cerr << "Could not open audio decoder" << std::endl;
            avformat_close_input(&inputFormatContext);
            avformat_free_context(outputFormatContext);
            avcodec_free_context(&audioDecoderContext);
            return outputBuffer;
        }
    }

    // Open output codecs
    if (videoStreamIndex >= 0) {
        const AVCodec *videoEncoder = avcodec_find_encoder_by_name(job.getVideoCodec().c_str());
        if (!videoEncoder) {
            std::cerr << "Video encoder not found" << std::endl;
            avformat_close_input(&inputFormatContext);
            avformat_free_context(outputFormatContext);
            return outputBuffer;
        }
        videoEncoderContext = avcodec_alloc_context3(videoEncoder);
        videoEncoderContext->width = job.getWidth();
        videoEncoderContext->height = job.getHeight();
        AVRational time_base = av_d2q(job.getFps(), 100000);
        videoEncoderContext->time_base = time_base;
        videoEncoderContext->pix_fmt = AV_PIX_FMT_YUV420P;
        if (avcodec_open2(videoEncoderContext, videoEncoder, nullptr) < 0) {
            std::cerr << "Could not open video encoder" << std::endl;
            avformat_close_input(&inputFormatContext);
            avformat_free_context(outputFormatContext);
            avcodec_free_context(&videoEncoderContext);
            return outputBuffer;
        }
        avcodec_parameters_from_context(videoStream->codecpar, videoEncoderContext);
    }
    if (audioStreamIndex >= 0) {
        const AVCodec *audioEncoder = avcodec_find_encoder_by_name(job.getAudioCodec().c_str());
        if (!audioEncoder) {
            std::cerr << "Audio encoder not found" << std::endl;
            avformat_close_input(&inputFormatContext);
            avformat_free_context(outputFormatContext);
            return outputBuffer;
        }
        audioEncoderContext = avcodec_alloc_context3(audioEncoder);
        audioEncoderContext->sample_rate = 48000;  // Example sample rate
        audioEncoderContext->sample_fmt = audioEncoder->sample_fmts[0];
        audioEncoderContext->time_base = (AVRational){1, audioEncoderContext->sample_rate};
        audioEncoderContext->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO;
        if (avcodec_open2(audioEncoderContext, audioEncoder, nullptr) < 0) {
            std::cerr << "Could not open audio encoder" << std::endl;
            avformat_close_input(&inputFormatContext);
            avformat_free_context(outputFormatContext);
            avcodec_free_context(&audioEncoderContext);
            return outputBuffer;
        }
        avcodec_parameters_from_context(audioStream->codecpar, audioEncoderContext);
    }

    // Allocate AVPacket and AVFrame
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    // Calculate the end time based on the start time and duration
    int64_t endTime = startTime + static_cast<int64_t>(job.getDuration() * AV_TIME_BASE);

    // Transcode the video and audio frames within the specified segment
    while (av_read_frame(inputFormatContext, packet) >= 0) {
        int64_t packetPts = packet->pts *
                            av_q2d(inputFormatContext->streams[packet->stream_index]->time_base) *
                            AV_TIME_BASE;
        if (packetPts >= endTime) {
            break;
        }

        if (packet->stream_index == videoStreamIndex) {
            if (avcodec_send_packet(videoDecoderContext, packet) == 0) {
                while (avcodec_receive_frame(videoDecoderContext, frame) == 0) {
                    avcodec_send_frame(videoEncoderContext, frame);
                    while (avcodec_receive_packet(videoEncoderContext, packet) == 0) {
                        outputBuffer.insert(outputBuffer.end(), packet->data,
                                            packet->data + packet->size);
                        av_packet_unref(packet);
                    }
                    av_frame_unref(frame);
                }
            }
        } else if (packet->stream_index == audioStreamIndex) {
            if (avcodec_send_packet(audioDecoderContext, packet) == 0) {
                while (avcodec_receive_frame(audioDecoderContext, frame) == 0) {
                    avcodec_send_frame(audioEncoderContext, frame);
                    while (avcodec_receive_packet(audioEncoderContext, packet) == 0) {
                        outputBuffer.insert(outputBuffer.end(), packet->data,
                                            packet->data + packet->size);
                        av_packet_unref(packet);
                    }
                    av_frame_unref(frame);
                }
            }
        }
        av_packet_unref(packet);
    }

    // Clean up
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&videoDecoderContext);
    avcodec_free_context(&audioDecoderContext);
    avcodec_free_context(&videoEncoderContext);
    avcodec_free_context(&audioEncoderContext);
    avformat_close_input(&inputFormatContext);
    avformat_free_context(outputFormatContext);
    return outputBuffer;
}
std::vector<char> Transcoder::transcodeInit(const Job &job) const {
    std::vector<char> outputBuffer;

    // Initialize FFmpeg components
    AVFormatContext *inputFormatContext = nullptr;
    AVFormatContext *outputFormatContext = nullptr;
    AVCodecContext *videoDecoderContext = nullptr;
    AVCodecContext *audioDecoderContext = nullptr;
    AVCodecContext *videoEncoderContext = nullptr;
    AVCodecContext *audioEncoderContext = nullptr;
    AVStream *videoStream = nullptr;
    AVStream *audioStream = nullptr;
    int videoStreamIndex = -1;
    int audioStreamIndex = -1;

    // Open input file
    if (avformat_open_input(&inputFormatContext, job.getUrl().c_str(), nullptr, nullptr) < 0) {
        std::cerr << "Could not open input file" << std::endl;
        return outputBuffer;
    }

    // Find stream info
    if (avformat_find_stream_info(inputFormatContext, nullptr) < 0) {
        std::cerr << "Could not find stream info" << std::endl;
        avformat_close_input(&inputFormatContext);
        return outputBuffer;
    }

    // Find video and audio stream indices
    for (unsigned int i = 0; i < inputFormatContext->nb_streams; ++i) {
        if (inputFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
        } else if (inputFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
        }
    }

    // Create output format context
    const char *outputFormat = job.getFormat() == Format::MPEGTS ? "mpegts" : "mp4";
    avformat_alloc_output_context2(&outputFormatContext, nullptr, outputFormat, nullptr);
    if (!outputFormatContext) {
        std::cerr << "Could not create output context" << std::endl;
        avformat_close_input(&inputFormatContext);
        return outputBuffer;
    }

    // Add streams to output format context
    if (videoStreamIndex >= 0 &&
        (job.getMedia() == Media::VIDEO || job.getMedia() == Media::COMBINED)) {
        videoStream = avformat_new_stream(outputFormatContext, nullptr);
        if (!videoStream) {
            std::cerr << "Failed to allocate video stream" << std::endl;
            avformat_close_input(&inputFormatContext);
            avformat_free_context(outputFormatContext);
            return outputBuffer;
        }
        AVRational time_base = av_d2q(job.getFps(), 100000);
        videoStream->time_base = time_base;
    }
    if (audioStreamIndex >= 0 &&
        (job.getMedia() == Media::AUDIO || job.getMedia() == Media::COMBINED)) {
        audioStream = avformat_new_stream(outputFormatContext, nullptr);
        if (!audioStream) {
            std::cerr << "Failed to allocate audio stream" << std::endl;
            avformat_close_input(&inputFormatContext);
            avformat_free_context(outputFormatContext);
            return outputBuffer;
        }
        audioStream->time_base = (AVRational){1, 48000};  // Example sample rate
    }

    // Open input codecs
    if (videoStreamIndex >= 0) {
        const AVCodec *videoDecoder =
            avcodec_find_decoder(inputFormatContext->streams[videoStreamIndex]->codecpar->codec_id);
        videoDecoderContext = avcodec_alloc_context3(videoDecoder);
        avcodec_parameters_to_context(videoDecoderContext,
                                      inputFormatContext->streams[videoStreamIndex]->codecpar);
        if (avcodec_open2(videoDecoderContext, videoDecoder, nullptr) < 0) {
            std::cerr << "Could not open video decoder" << std::endl;
            avformat_close_input(&inputFormatContext);
            avformat_free_context(outputFormatContext);
            avcodec_free_context(&videoDecoderContext);
            return outputBuffer;
        }
    }
    if (audioStreamIndex >= 0) {
        const AVCodec *audioDecoder =
            avcodec_find_decoder(inputFormatContext->streams[audioStreamIndex]->codecpar->codec_id);
        audioDecoderContext = avcodec_alloc_context3(audioDecoder);
        avcodec_parameters_to_context(audioDecoderContext,
                                      inputFormatContext->streams[audioStreamIndex]->codecpar);
        if (avcodec_open2(audioDecoderContext, audioDecoder, nullptr) < 0) {
            std::cerr << "Could not open audio decoder" << std::endl;
            avformat_close_input(&inputFormatContext);
            avformat_free_context(outputFormatContext);
            avcodec_free_context(&audioDecoderContext);
            return outputBuffer;
        }
    }

    // Open output codecs
    if (videoStreamIndex >= 0) {
        const AVCodec *videoEncoder = avcodec_find_encoder_by_name(job.getVideoCodec().c_str());
        videoEncoderContext = avcodec_alloc_context3(videoEncoder);
        videoEncoderContext->width = job.getWidth();
        videoEncoderContext->height = job.getHeight();
        AVRational time_base = av_d2q(job.getFps(), 100000);
        videoEncoderContext->time_base = time_base;
        videoEncoderContext->pix_fmt = AV_PIX_FMT_YUV420P;
        if (avcodec_open2(videoEncoderContext, videoEncoder, nullptr) < 0) {
            std::cerr << "Could not open video encoder" << std::endl;
            avformat_close_input(&inputFormatContext);
            avformat_free_context(outputFormatContext);
            avcodec_free_context(&videoEncoderContext);
            return outputBuffer;
        }
        avcodec_parameters_from_context(videoStream->codecpar, videoEncoderContext);
    }
    if (audioStreamIndex >= 0) {
        const AVCodec *audioEncoder = avcodec_find_encoder_by_name(job.getAudioCodec().c_str());
        audioEncoderContext = avcodec_alloc_context3(audioEncoder);
        audioEncoderContext->sample_rate = 48000;  // Example sample rate
        audioEncoderContext->sample_fmt = audioEncoder->sample_fmts[0];
        audioEncoderContext->time_base = (AVRational){1, audioEncoderContext->sample_rate};
        if (avcodec_open2(audioEncoderContext, audioEncoder, nullptr) < 0) {
            std::cerr << "Could not open audio encoder" << std::endl;
            avformat_close_input(&inputFormatContext);
            avformat_free_context(outputFormatContext);
            avcodec_free_context(&audioEncoderContext);
            return outputBuffer;
        }
        avcodec_parameters_from_context(audioStream->codecpar, audioEncoderContext);
    }

    // Create an AVIOContext to write to the output buffer
    uint8_t *buffer = nullptr;
    size_t bufferSize = 0;
    AVIOContext *avioContext = avio_alloc_context(
        buffer, bufferSize, 1, &outputBuffer,
        [](void *opaque, uint8_t *buf, int buf_size) {
            auto *outputBuffer = static_cast<std::vector<char> *>(opaque);
            outputBuffer->insert(outputBuffer->end(), buf, buf + buf_size);
            return buf_size;
        },
        nullptr, nullptr);
    if (!avioContext) {
        std::cerr << "Could not create AVIOContext" << std::endl;
        avformat_close_input(&inputFormatContext);
        avformat_free_context(outputFormatContext);
        return outputBuffer;
    }
    outputFormatContext->pb = avioContext;

    // Write the header
    if (avformat_write_header(outputFormatContext, nullptr) < 0) {
        std::cerr << "Error writing header" << std::endl;
        avformat_close_input(&inputFormatContext);
        avformat_free_context(outputFormatContext);
        avio_context_free(&avioContext);
        return outputBuffer;
    }

    // Clean up
    avcodec_free_context(&videoDecoderContext);
    avcodec_free_context(&audioDecoderContext);
    avcodec_free_context(&videoEncoderContext);
    avcodec_free_context(&audioEncoderContext);
    avformat_close_input(&inputFormatContext);
    avformat_free_context(outputFormatContext);
    avio_context_free(&avioContext);

    return outputBuffer;
}

// std::vector<char> Transcoder::transcode(const Job &job) const {
//     std::vector<char> result;
//     std::string str = "";
//     str += "URL: " + job.getUrl() + "\n";
//     str += "Bitrate: " + std::to_string(job.getBitrate()) + "\n";
//     str += "Width: " + std::to_string(job.getWidth()) + "\n";
//     str += "Height: " + std::to_string(job.getHeight()) + "\n";
//     str += "FPS: " + std::to_string(job.getFps()) + "\n";
//     str += "Start: " + std::to_string(job.getStart()) + "\n";
//     str += "Duration: " + std::to_string(job.getDuration()) + "\n";
//     str += "Video Codec: " + job.getVideoCodec() + "\n";
//     str += "Audio Codec: " + job.getAudioCodec() + "\n";

//     if (job.getFormat() == Format::MPEGTS) {
//         str += "Format: MPEGTS\n";
//     } else if (job.getFormat() == Format::MPEG4) {
//         str += "Format: MPEG4\n";
//     }

//     if (job.getMedia() == Media::VIDEO) {
//         str += "Media: Video\n";
//     } else if (job.getMedia() == Media::AUDIO) {
//         str += "Media: Audio\n";
//     } else if (job.getMedia() == Media::COMBINED) {
//         str += "Media: Combined\n";
//     }

//     result.insert(result.end(), str.begin(), str.end());

//     return result;
// }