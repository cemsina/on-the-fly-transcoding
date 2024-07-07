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
		"-vf", fmt.Sprintf("scale=%d:%d", job.Width, job.Height),
		"-b:v", fmt.Sprintf("%dk", job.Bitrate),
		"-r", fmt.Sprintf("%f", job.FPS),
		"-ac", "2",
		"-ar", "44100",
		"-c:v", job.VideoCodec,
		"-c:a", job.AudioCodec,
		"-ss", fmt.Sprintf("%f", job.Start),
		"-t", fmt.Sprintf("%f", job.Duration),
	}

	if job.Format == MPEGTS {
		args = append(args, "-f", "mpegts", outputFile)
	} else {
		args = append(args, outputFile)
	}
	
	cmd := exec.Command("ffmpeg", args...)
	output, err := cmd.CombinedOutput()
	if err != nil {
		return "", fmt.Errorf("ffmpeg command failed: %v, output: %s", err, output)
	}

	cache.Put(jobID, outputFile)
	return outputFile, nil
}