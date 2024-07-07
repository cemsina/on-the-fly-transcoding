package main

import (
	"crypto/md5"
	"encoding/hex"
	"fmt"
)

// Format enum
type Format int

const (
	MPEGTS Format = iota
	MP4
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
	Profile    Profile
	Start      float64
	Duration   float64
	Media      Media
	Format     Format
	Fragmented bool
}

func (job *Job) ID() string {
	data := fmt.Sprintf("%s-%s-%f-%f-%d-%d-%t",
		job.URL, job.Profile.Name,
		job.Start, job.Duration, job.Media, job.Format, job.Fragmented)
	hash := md5.Sum([]byte(data))
	return hex.EncodeToString(hash[:])
}

func GetFormatExtension(format Format) string {
	switch format {
	case MPEGTS:
		return "ts"
	case MP4:
		return "mp4"
	default:
		return "mp4"
	}
}
