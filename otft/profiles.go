package main

type Profile struct {
	Name         string
	Width        int
	Height       int
	VideoBitrate int
	AudioBitrate int
	VideoCodec   string
	AudioCodec   string
	FPS          float64
	AudioSample  int
}

var Profiles = map[string]Profile{
	"720p": {
		Name:         "720p",
		Width:        1280,
		Height:       720,
		VideoBitrate: 2200,
		AudioBitrate: 128,
		VideoCodec:   "libx264",
		AudioCodec:   "aac",
		FPS:          25,
		AudioSample:  44100,
	},
	"480p": {
		Name:         "480p",
		Width:        854,
		Height:       480,
		VideoBitrate: 1500,
		AudioBitrate: 128,
		VideoCodec:   "libx264",
		AudioCodec:   "aac",
		FPS:          25,
		AudioSample:  44100,
	},
	"360p": {
		Name:         "360p",
		Width:        640,
		Height:       360,
		VideoBitrate: 800,
		AudioBitrate: 64,
		VideoCodec:   "libx264",
		AudioCodec:   "aac",
		FPS:          25,
		AudioSample:  44100,
	},
	"240p": {
		Name:         "240p",
		Width:        426,
		Height:       240,
		VideoBitrate: 400,
		AudioBitrate: 64,
		VideoCodec:   "libx264",
		AudioCodec:   "aac",
		FPS:          25,
		AudioSample:  44100,
	},
}

var ProfileRegex string

func GetProfile(name string) (Profile, bool) {
	profile, found := Profiles[name]
	return profile, found
}

func InitProfiles() {
	ProfileRegex = "("
	for name := range Profiles {
		ProfileRegex += name + "|"
	}
	ProfileRegex = ProfileRegex[:len(ProfileRegex)-1] + ")"
}
