package main

import (
	"fmt"
	"os"
	"os/exec"
	"sync"
)

var (
	jobLocks   = make(map[string]*sync.Mutex)
	jobLocksMu sync.Mutex
	cache      = NewLRUCache(50, func(key, value string) {
		os.Remove(value)
	})
)

func getJobLock(jobID string) *sync.Mutex {
	jobLocksMu.Lock()
	defer jobLocksMu.Unlock()

	if _, exists := jobLocks[jobID]; !exists {
		jobLocks[jobID] = &sync.Mutex{}
	}
	return jobLocks[jobID]
}

func Transcode(job Job) (string, error) {
	jobID := job.ID()
	outputFile := fmt.Sprintf("/tmp/%s.%s", jobID, GetFormatExtension(job.Format))

	jobLock := getJobLock(jobID)
	jobLock.Lock()
	defer jobLock.Unlock()

	if cachedFile, found := cache.Get(jobID); found {
		return cachedFile, nil // Return cached file path if it exists
	}

	if _, err := os.Stat(outputFile); err == nil {
		cache.Put(jobID, outputFile)
		return outputFile, nil // File already exists
	}

	args := []string{
		"-i", job.URL,
		"-preset", "ultrafast",
		"-vf", fmt.Sprintf("scale=%d:%d", job.Profile.Width, job.Profile.Height),
		"-b:v", fmt.Sprintf("%dk", job.Profile.VideoBitrate),
		"-b:a", fmt.Sprintf("%dk", job.Profile.AudioBitrate),
		"-r", fmt.Sprintf("%f", job.Profile.FPS),
		"-pix_fmt", "yuv420p",
		"-ac", "2",
		"-ar", fmt.Sprintf("%d", job.Profile.AudioSample),
		"-c:v", job.Profile.VideoCodec,
		"-c:a", job.Profile.AudioCodec,
	}

	initSegment := true
	if job.Start >= 0 {
		args = append(args, "-ss", fmt.Sprintf("%f", job.Start))
		initSegment = false
	}

	if job.Duration >= 0 {
		args = append(args, "-t", fmt.Sprintf("%f", job.Duration))
	}

	if initSegment && job.Format == MP4 {
		args = append(args, "-movflags", "+faststart")
	}

	if !initSegment && job.Fragmented && job.Format == MP4 {
		args = append(args, "-movflags", "+frag_keyframe+empty_moov")
	}

	if !initSegment {
		if job.Format == MPEGTS {
			args = append(args, "-f", "mpegts", outputFile)
		} else {
			args = append(args, "-f", "mp4", outputFile)
		}
	} else {
		args = append(args, "-t", fmt.Sprintf("%f", 0.1))
		args = append(args, "-f", "mp4", outputFile)
	}

	if job.Media == AUDIO {
		args = append([]string{"-vn"}, args...)
	} else if job.Media == VIDEO {
		args = append([]string{"-an"}, args...)
	}

	cmd := exec.Command("ffmpeg", args...)
	output, err := cmd.CombinedOutput()
	if err != nil {
		return "", fmt.Errorf("ffmpeg command failed: %v, output: %s", err, output)
	}

	cache.Put(jobID, outputFile)
	return outputFile, nil
}
