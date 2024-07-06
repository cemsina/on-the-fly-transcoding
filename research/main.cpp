extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavfilter/avfilter.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <iostream>
#include <string>

void transcodeSegment(std::string url, float start, float duration, std::string output) {
    std::string videoCodec = "libx264";
    std::string audioCodec = "aac";
    int videoBitrate = 2000000;
    int audioBitrate = 128000;
    int width = 1280;
    int height = 720;
    int fps = 25;
    int audioSampleRate = 44100;
    int audioChannels = 2;

    avformat_network_init();
    AVFormatContext *inputFormatContext = nullptr;
    AVFormatContext *outputFormatContext = nullptr;
    AVCodecContext *videoCodecContext = nullptr;
    AVCodecContext *audioCodecContext = nullptr;
    AVStream *videoStream = nullptr;
    AVStream *audioStream = nullptr;
    AVPacket packet;
    int ret;

    // Open input file
    if (avformat_open_input(&inputFormatContext, url.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "Could not open input file." << std::endl;
        return;
    }

    if (avformat_find_stream_info(inputFormatContext, nullptr) < 0) {
        std::cerr << "Could not find stream information." << std::endl;
        avformat_close_input(&inputFormatContext);
        return;
    }

    // Open output file
    avformat_alloc_output_context2(&outputFormatContext, nullptr, nullptr, output.c_str());
    if (!outputFormatContext) {
        std::cerr << "Could not create output context." << std::endl;
        avformat_close_input(&inputFormatContext);
        return;
    }

    int videoStreamIndex = -1;
    int audioStreamIndex = -1;
    for (unsigned int i = 0; i < inputFormatContext->nb_streams; ++i) {
        if (inputFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
            videoStreamIndex == -1) {
            videoStreamIndex = i;
        } else if (inputFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO &&
                   audioStreamIndex == -1) {
            audioStreamIndex = i;
        }
    }

    if (videoStreamIndex == -1 && audioStreamIndex == -1) {
        std::cerr << "Could not find video or audio stream." << std::endl;
        avformat_close_input(&inputFormatContext);
        avformat_free_context(outputFormatContext);
        return;
    }

    // Copy video stream
    if (videoStreamIndex != -1) {
        AVStream *inStream = inputFormatContext->streams[videoStreamIndex];
        AVStream *outStream = avformat_new_stream(outputFormatContext, nullptr);
        if (!outStream) {
            std::cerr << "Failed allocating output stream." << std::endl;
            avformat_close_input(&inputFormatContext);
            avformat_free_context(outputFormatContext);
            return;
        }
        const AVCodec *encoder = avcodec_find_encoder_by_name(videoCodec.c_str());
        videoCodecContext = avcodec_alloc_context3(encoder);
        if (!videoCodecContext) {
            std::cerr << "Could not allocate video codec context." << std::endl;
            avformat_close_input(&inputFormatContext);
            avformat_free_context(outputFormatContext);
            return;
        }
        avcodec_parameters_to_context(videoCodecContext, inStream->codecpar);
        videoCodecContext->height = height;
        videoCodecContext->width = width;
        videoCodecContext->sample_aspect_ratio = inStream->codecpar->sample_aspect_ratio;
        if (encoder->pix_fmts) {
            videoCodecContext->pix_fmt = encoder->pix_fmts[0];
        } else {
            videoCodecContext->pix_fmt = static_cast<AVPixelFormat>(inStream->codecpar->format);
        }
        videoCodecContext->bit_rate = videoBitrate;
        videoCodecContext->time_base = av_inv_q(av_d2q(fps, 1001000));
        avcodec_parameters_from_context(outStream->codecpar, videoCodecContext);
        outStream->time_base = videoCodecContext->time_base;
    }

    // Copy audio stream
    if (audioStreamIndex != -1) {
        AVStream *inStream = inputFormatContext->streams[audioStreamIndex];
        AVStream *outStream = avformat_new_stream(outputFormatContext, nullptr);
        if (!outStream) {
            std::cerr << "Failed allocating output stream." << std::endl;
            avformat_close_input(&inputFormatContext);
            avformat_free_context(outputFormatContext);
            return;
        }
        const AVCodec *encoder = avcodec_find_encoder_by_name(audioCodec.c_str());
        audioCodecContext = avcodec_alloc_context3(encoder);
        if (!audioCodecContext) {
            std::cerr << "Could not allocate audio codec context." << std::endl;
            avformat_close_input(&inputFormatContext);
            avformat_free_context(outputFormatContext);
            return;
        }
        avcodec_parameters_to_context(audioCodecContext, inStream->codecpar);
        audioCodecContext->bit_rate = audioBitrate;
        audioCodecContext->sample_rate = audioSampleRate;
        audioCodecContext->ch_layout = AV_CHANNEL_LAYOUT_STEREO;
        audioCodecContext->sample_fmt = encoder->sample_fmts[0];
        avcodec_parameters_from_context(outStream->codecpar, audioCodecContext);
        outStream->time_base = {1, audioSampleRate};
    }

    // Open output file for writing
    if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&outputFormatContext->pb, output.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Could not open output file." << std::endl;
            avformat_close_input(&inputFormatContext);
            avformat_free_context(outputFormatContext);
            return;
        }
    }

    if (avformat_write_header(outputFormatContext, nullptr) < 0) {
        std::cerr << "Error occurred when writing header." << std::endl;
        avformat_close_input(&inputFormatContext);
        avformat_free_context(outputFormatContext);
        return;
    }

    // Seek to the start position

    int64_t startPtsVideo = av_rescale_q(start * AV_TIME_BASE, AV_TIME_BASE_Q,
                                         inputFormatContext->streams[videoStreamIndex]->time_base);
    int64_t endPtsVideo = av_rescale_q((start + duration) * AV_TIME_BASE, AV_TIME_BASE_Q,
                                       inputFormatContext->streams[videoStreamIndex]->time_base);
    int64_t startPtsAudio = av_rescale_q(start * AV_TIME_BASE, AV_TIME_BASE_Q,
                                         inputFormatContext->streams[audioStreamIndex]->time_base);
    int64_t endPtsAudio = av_rescale_q((start + duration) * AV_TIME_BASE, AV_TIME_BASE_Q,
                                       inputFormatContext->streams[audioStreamIndex]->time_base);
    av_seek_frame(inputFormatContext, videoStreamIndex, startPtsVideo, AVSEEK_FLAG_BACKWARD);
    std::cout << "start=" << start << ", duration=" << duration << std::endl;
    std::cout << "Input Video Timebase: den="
              << inputFormatContext->streams[videoStreamIndex]->time_base.den
              << ", num=" << inputFormatContext->streams[videoStreamIndex]->time_base.num
              << std::endl;
    std::cout << "Output Video Timebase: den="
              << outputFormatContext->streams[videoStreamIndex]->time_base.den
              << ", num=" << outputFormatContext->streams[videoStreamIndex]->time_base.num
              << std::endl;
    std::cout << "Input Audio Timebase: den="
              << inputFormatContext->streams[audioStreamIndex]->time_base.den
              << ", num=" << inputFormatContext->streams[audioStreamIndex]->time_base.num
              << std::endl;
    std::cout << "Output Audio Timebase: den="
              << outputFormatContext->streams[audioStreamIndex]->time_base.den
              << ", num=" << outputFormatContext->streams[audioStreamIndex]->time_base.num
              << std::endl;
    std::cout << "Video Start PTS: " << startPtsVideo << std::endl;
    std::cout << "Video End PTS: " << endPtsVideo << std::endl;
    std::cout << "Audio Start PTS: " << startPtsAudio << std::endl;
    std::cout << "Audio End PTS: " << endPtsAudio << std::endl;
    std::cout << "Duration: " << duration << std::endl;
    // Read frames and write to output file
    while (av_read_frame(inputFormatContext, &packet) >= 0) {
        if (packet.stream_index == videoStreamIndex || packet.stream_index == audioStreamIndex) {
            int endPts = packet.stream_index == videoStreamIndex ? endPtsVideo : endPtsAudio;
            int startPts = packet.stream_index == videoStreamIndex ? startPtsVideo : startPtsAudio;
            if (packet.pts >= endPts) {
                av_packet_unref(&packet);
                break;
            }
            if (packet.pts != AV_NOPTS_VALUE) {
                packet.pts =
                    av_rescale_q(packet.pts - startPts,
                                 inputFormatContext->streams[packet.stream_index]->time_base,
                                 outputFormatContext->streams[packet.stream_index]->time_base);
            }
            if (packet.dts != AV_NOPTS_VALUE) {
                packet.dts =
                    av_rescale_q(packet.dts - startPts,
                                 inputFormatContext->streams[packet.stream_index]->time_base,
                                 outputFormatContext->streams[packet.stream_index]->time_base);
            }

            packet.duration = av_rescale_q(
                packet.duration, inputFormatContext->streams[packet.stream_index]->time_base,
                outputFormatContext->streams[packet.stream_index]->time_base);
            packet.pos = -1;
            av_interleaved_write_frame(outputFormatContext, &packet);

            av_packet_unref(&packet);
        } else {
            av_packet_unref(&packet);
        }
    }

    // Write output file trailer
    av_write_trailer(outputFormatContext);

    if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&outputFormatContext->pb);
    }
    avformat_free_context(outputFormatContext);
    avcodec_free_context(&videoCodecContext);
    avcodec_free_context(&audioCodecContext);
    avformat_close_input(&inputFormatContext);
    avformat_network_deinit();

    // Fill the function to transcode the segment of the video from the given URL and save it to the
    // output file The segment should start at the given start time and have the given duration The
    // video should be encoded with the videoCodec and the audio with the audioCodec The output file
    // should be in the mp4 format Use the ffmpeg library to transcode the video Do not use exec or
    // system functions to run the ffmpeg command
    // Use latest ffmpeg libraries and functions (ex: there is no av_register_all for latest ffmpeg)
}

int main() {
    std::string url =
        "https://orange-space-orbit-97pg54j6wgrhxgq4-3000.app.github.dev/research/file.mp4";
    transcodeSegment(url, 3.3, 5.8, "output.mp4");
    std::cout << "Transcoding done." << std::endl;
    return 0;
}