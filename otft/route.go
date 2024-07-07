package main

import (
	"encoding/base64"
	"fmt"
	"net/http"
	"os"
	"regexp"
	"strconv"
	"strings"
)

var Patterns = map[string]*regexp.Regexp{}

func PreparePatterns() {
	combinedPattern := regexp.MustCompile(fmt.Sprintf(`^/([^/]+)/combined/%s/(\d+(\.\d+)?:\d+(\.\d+)?)/(full|fragmented)\.(ts|mp4)$`, ProfileRegex))
	videoInitPattern := regexp.MustCompile(fmt.Sprintf(`^/([^/]+)/video/%s/init\.mp4$`, ProfileRegex))
	audioInitPattern := regexp.MustCompile(fmt.Sprintf(`^/([^/]+)/audio/%s/init\.mp4$`, ProfileRegex))
	videoSegmentPattern := regexp.MustCompile(fmt.Sprintf(`^/([^/]+)/video/%s/(\d+(\.\d+)?:\d+(\.\d+)?)/(full|fragmented)\.mp4$`, ProfileRegex))
	audioSegmentPattern := regexp.MustCompile(fmt.Sprintf(`^/([^/]+)/audio/%s/(\d+(\.\d+)?:\d+(\.\d+)?)/(full|fragmented)\.mp4$`, ProfileRegex))

	Patterns["combined"] = combinedPattern
	Patterns["videoInit"] = videoInitPattern
	Patterns["audioInit"] = audioInitPattern
	Patterns["videoSegment"] = videoSegmentPattern
	Patterns["audioSegment"] = audioSegmentPattern
}

func RouteHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Access-Control-Allow-Origin", "*")
	w.Header().Set("Access-Control-Allow-Methods", "GET, OPTIONS, HEAD")
	w.Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization, Range")

	path := r.URL.Path
	var job *Job
	switch {
	case Patterns["combined"].MatchString(path):
		matches := Patterns["combined"].FindStringSubmatch(path)
		job = handleCombined(matches)
	case Patterns["videoInit"].MatchString(path):
		matches := Patterns["videoInit"].FindStringSubmatch(path)
		job = handleVideoInit(matches)
	case Patterns["audioInit"].MatchString(path):
		matches := Patterns["audioInit"].FindStringSubmatch(path)
		job = handleAudioInit(matches)
	case Patterns["videoSegment"].MatchString(path):
		matches := Patterns["videoSegment"].FindStringSubmatch(path)
		job = handleVideoSegment(matches)
	case Patterns["audioSegment"].MatchString(path):
		matches := Patterns["audioSegment"].FindStringSubmatch(path)
		job = handleAudioSegment(matches)
	default:
		http.Error(w, "Invalid request", http.StatusBadRequest)
		return
	}

	if job == nil {
		http.Error(w, "Invalid profile", http.StatusBadRequest)
		return
	}

	transcodeNServe(r, w, *job)
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

func handleCombined(matches []string) *Job {
	profile, exists := GetProfile(matches[2])
	if !exists {
		return nil
	}
	job := &Job{
		URL:        decodeBase64URL(matches[1]),
		Profile:    profile,
		Start:      parseStartDuration(matches[3])[0],
		Duration:   parseStartDuration(matches[3])[1],
		Fragmented: matches[4] == "fragmented",
		Format:     parseFormat(matches[5]),
		Media:      COMBINED,
	}

	return job
}

func handleVideoInit(matches []string) *Job {
	profile, exists := GetProfile(matches[2])
	if !exists {
		return nil
	}
	job := &Job{
		URL:      decodeBase64URL(matches[1]),
		Profile:  profile,
		Start:    -1,
		Duration: -1,
		Format:   MP4,
		Media:    VIDEO,
	}

	return job
}

func handleAudioInit(matches []string) *Job {
	profile, exists := GetProfile(matches[2])
	if !exists {
		return nil
	}
	job := &Job{
		URL:      decodeBase64URL(matches[1]),
		Profile:  profile,
		Start:    -1,
		Duration: -1,
		Format:   MP4,
		Media:    AUDIO,
	}

	return job
}

func handleVideoSegment(matches []string) *Job {
	profile, exists := GetProfile(matches[2])
	if !exists {
		return nil
	}
	job := &Job{
		URL:        decodeBase64URL(matches[1]),
		Profile:    profile,
		Start:      parseStartDuration(matches[3])[0],
		Duration:   parseStartDuration(matches[3])[1],
		Fragmented: matches[4] == "fragmented",
		Format:     MP4,
		Media:      VIDEO,
	}

	return job
}

func handleAudioSegment(matches []string) *Job {
	profile, exists := GetProfile(matches[2])
	if !exists {
		return nil
	}
	job := &Job{
		URL:      decodeBase64URL(matches[1]),
		Profile:  profile,
		Start:    parseStartDuration(matches[3])[0],
		Duration: parseStartDuration(matches[3])[1],
		Fragmented: matches[4] == "fragmented",
		Format:   MP4,
		Media:    AUDIO,
	}

	return job
}

// Utility functions
func decodeBase64URL(encoded string) string {
	decoded, _ := base64.URLEncoding.DecodeString(encoded)
	return string(decoded)
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
	return MP4
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
