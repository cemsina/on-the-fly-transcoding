package main

import (
	"encoding/base64"
	"net/http"
	"os"
	"regexp"
	"strconv"
	"strings"
)

func RouteHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Access-Control-Allow-Origin", "*")
	w.Header().Set("Access-Control-Allow-Methods", "GET, OPTIONS, HEAD")
	w.Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization, Range")
	// Define regex patterns for the routes
	segmentPattern := regexp.MustCompile(`^/([^/]+)/combined/(\d+)/(\d+x\d+)/(\d+(\.\d+)?:\d+(\.\d+)?)/segment\.(ts|mp4)$`)
	videoInitPattern := regexp.MustCompile(`^/([^/]+)/video/(\d+)/(\d+x\d+)/init\.mp4$`)
	audioInitPattern := regexp.MustCompile(`^/([^/]+)/audio/(\d+)/init\.mp4$`)
	videoSegmentPattern := regexp.MustCompile(`^/([^/]+)/video/(\d+)/(\d+x\d+)/(\d+(\.\d+)?:\d+(\.\d+)?)/segment\.mp4$`)
	audioSegmentPattern := regexp.MustCompile(`^/([^/]+)/audio/(\d+)/(\d+(\.\d+)?:\d+(\.\d+)?)/segment\.mp4$`)

	path := r.URL.Path
	var job Job
	switch {
	case segmentPattern.MatchString(path):
		matches := segmentPattern.FindStringSubmatch(path)
		job = handleSegment(matches)
	case videoInitPattern.MatchString(path):
		matches := videoInitPattern.FindStringSubmatch(path)
		job = handleVideoInit(matches)
	case audioInitPattern.MatchString(path):
		matches := audioInitPattern.FindStringSubmatch(path)
		job = handleAudioInit(matches)
	case videoSegmentPattern.MatchString(path):
		matches := videoSegmentPattern.FindStringSubmatch(path)
		job = handleVideoSegment(matches)
	case audioSegmentPattern.MatchString(path):
		matches := audioSegmentPattern.FindStringSubmatch(path)
		job = handleAudioSegment(matches)
	default:
		http.Error(w, "Invalid request", http.StatusBadRequest)
		return
	}

	transcodeNServe(r, w, job)
}

func transcodeNServe(r *http.Request, w http.ResponseWriter, job Job) {
	jobID := job.ID()
	cache.Protect(jobID)

	done := make(chan string)
	errChan := make(chan error)

	go func() {
		filePath, err := Transcode(job)
		if err != nil {
			errChan <- err
			return
		}
		done <- filePath
	}()

	select {
	case filePath := <-done:
		serveFile(w, r, filePath, job.Format)
		cache.Unprotect(jobID)
	case err := <-errChan:
		http.Error(w, err.Error(), http.StatusInternalServerError)
	}
}

func handleSegment(matches []string) Job {
	job := Job{
		URL:        decodeBase64URL(matches[1]),
		Bitrate:    atoi(matches[2]),
		Width:      parseWidthHeight(matches[3])[0],
		Height:     parseWidthHeight(matches[3])[1],
		Start:      parseStartDuration(matches[4])[0],
		Duration:   parseStartDuration(matches[4])[1],
		Format:     parseFormat(matches[5]),
		VideoCodec: "libx264",
		AudioCodec: "aac",
		FPS:        25,
	}

	return job
}

func handleVideoInit(matches []string) Job {
	job := Job{
		URL:        decodeBase64URL(matches[1]),
		Bitrate:    atoi(matches[2]),
		Width:      parseWidthHeight(matches[3])[0],
		Height:     parseWidthHeight(matches[3])[1],
		Format:     MPEG4,
		VideoCodec: "libx264",
		AudioCodec: "aac",
		FPS:        25,
	}

	return job
}

func handleAudioInit(matches []string) Job {
	job := Job{
		URL:        decodeBase64URL(matches[1]),
		Bitrate:    atoi(matches[2]),
		Width:      -1,
		Height:     -1,
		Format:     MPEG4,
		VideoCodec: "libx264",
		AudioCodec: "aac",
		FPS:        25,
	}

	return job
}

func handleVideoSegment(matches []string) Job {
	job := Job{
		URL:        decodeBase64URL(matches[1]),
		Bitrate:    atoi(matches[2]),
		Width:      parseWidthHeight(matches[3])[0],
		Height:     parseWidthHeight(matches[3])[1],
		Start:      parseStartDuration(matches[4])[0],
		Duration:   parseStartDuration(matches[4])[1],
		Format:     MPEG4,
		VideoCodec: "libx264",
		AudioCodec: "aac",
		FPS:        25,
	}

	return job
}

func handleAudioSegment(matches []string) Job {
	job := Job{
		URL:        decodeBase64URL(matches[1]),
		Bitrate:    atoi(matches[2]),
		Start:      parseStartDuration(matches[3])[0],
		Duration:   parseStartDuration(matches[3])[1],
		Width:      -1,
		Height:     -1,
		Format:     MPEG4,
		VideoCodec: "libx264",
		AudioCodec: "aac",
		FPS:        25,
	}

	return job
}

// Utility functions
func decodeBase64URL(encoded string) string {
	decoded, _ := base64.URLEncoding.DecodeString(encoded)
	return string(decoded)
}

func atoi(s string) int {
	i, _ := strconv.Atoi(s)
	return i
}

func parseWidthHeight(s string) [2]int {
	parts := strings.Split(s, "x")
	width, _ := strconv.Atoi(parts[0])
	height, _ := strconv.Atoi(parts[1])
	return [2]int{width, height}
}

func parseStartDuration(s string) [2]float64 {
	parts := strings.Split(s, ":")
	start, _ := strconv.ParseFloat(parts[0], 64)
	duration, _ := strconv.ParseFloat(parts[1], 64)
	return [2]float64{start, duration}
}

func parseFormat(s string) Format {
	if s == "ts" {
		return MPEGTS
	}
	return MPEG4
}

func serveFile(w http.ResponseWriter, r *http.Request, filePath string, format Format) {
	var contentType string
	if format == MPEGTS {
		contentType = "video/MP2T"
	} else {
		contentType = "video/mp4"
	}

	w.Header().Set("Content-Type", contentType)
	file, err := os.Open(filePath)
	if err != nil {
		http.Error(w, "File not found.", http.StatusNotFound)
		return
	}
	defer file.Close()

	stat, err := file.Stat()
	if err != nil {
		http.Error(w, "File stat error.", http.StatusInternalServerError)
		return
	}

	http.ServeContent(w, r, filePath, stat.ModTime(), file)
}
