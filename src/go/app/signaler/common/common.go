package common

import "math"

type VideoResolution struct {
	Name   string
	Width  int
	Height int
}

var RESOLUTIONS = [...]VideoResolution{
	{"High", 640, 480},
	{"Medium", 480, 360},
	{"Low", 320, 240},
}

func ResolutionIdx(resolution string) int {
	for i, current_resolution := range RESOLUTIONS {
		if resolution == current_resolution.Name {
			return i
		}
	}
	return -1
}

func MatchCapsToBranch(width, height int) int {
	capsResolution := width * height

	minDiff := math.MaxUint32
	idx := 0

	for i, r := range RESOLUTIONS {
		diff := int(math.Abs(float64(capsResolution) - float64(r.Width)*float64(r.Height)))
		if diff < minDiff {
			minDiff = diff
			idx = i
		}
	}

	return idx
}
