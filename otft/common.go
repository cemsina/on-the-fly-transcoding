package main

import (
	"crypto/md5"
	"encoding/hex"
	"fmt"
)

// Format enum
type Format int

const (
	MPEG4 Format = iota
	MPEGTS
)

// Media enum
type Media int

const (
	VIDEO Media = iota
	AUDIO
	COMBINED
)

type Job struct {
	URL        string
	Bitrate    int
	Width      int
	Height     int
	FPS        float64
	Start      float64
	Duration   float64
	Media      Media
	Format     Format
	VideoCodec string
	AudioCodec string
}

func (job *Job) ID() string {
	data := fmt.Sprintf("%s-%d-%d-%d-%f-%f-%f-%d-%d-%s-%s",
		job.URL, job.Bitrate, job.Width, job.Height, job.FPS,
		job.Start, job.Duration, job.Media, job.Format, job.VideoCodec, job.AudioCodec)
	hash := md5.Sum([]byte(data))
	return hex.EncodeToString(hash[:])
}

func GetFormatExtension(format Format) string {
	switch format {
	case MPEGTS:
		return "ts"
	case MPEG4:
		return "mp4"
	default:
		return "mp4"
	}
}
