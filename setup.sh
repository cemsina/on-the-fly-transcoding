!#/bin/bash

sudo apt update

sudo apt-get install -y \
    autoconf \
    automake \
    build-essential \
    cmake \
    mediainfo \
    yasm \
    zlib1g-dev \
    libssl-dev \
    libass-dev \
    libfreetype6-dev \
    libmp3lame-dev \
    libvorbis-dev \
    libx264-dev \
    libx265-dev \
    libfdk-aac-dev \
    libnuma-dev \
    libvpx-dev \
    libfdk-aac-dev \
    libopus-dev \
    libvpx-dev \
    libssl-dev \
    pkg-config

# Install FFmpeg
mkdir -p ~/ffmpeg_sources
cd ~/ffmpeg_sources
wget https://ffmpeg.org/releases/ffmpeg-snapshot.tar.bz2
tar xjvf ffmpeg-snapshot.tar.bz2
cd ffmpeg

./configure --pkg-config-flags="--static" \
            --extra-cflags="-I/usr/local/include" \
            --extra-ldflags="-L/usr/local/lib" \
            --extra-libs="-lpthread -lm" \
            --bindir="/usr/local/bin" \
            --enable-gpl \
            --enable-libass \
            --enable-libfreetype \
            --enable-libmp3lame \
            --enable-libvorbis \
            --enable-libx264 \
            --enable-libfdk-aac \
            --enable-nonfree \
            --enable-shared \
            --enable-openssl
make
sudo make install
sudo ldconfig