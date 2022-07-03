package bon_utils

import (
	"fmt"
	"os"
	"strconv"

	"golang.org/x/exp/constraints"
)

type Number interface {
	constraints.Integer | constraints.Float
}

func MakeRange[T constraints.Integer](min, max T) []T {
	arr := make([]T, max-min)
	for i := range arr {
		arr[i] = min + T(i)
	}
	return arr
}

func GetEnv(name string) (string, error) {
	valStr, ok := os.LookupEnv(name)
	if !ok {
		return "", fmt.Errorf("%s not set", name)
	}

	return valStr, nil
}

func GetEnvInt(name string) (int, error) {
	valStr, err := GetEnv(name)
	if err != nil {
		return 0, err
	}

	return strconv.Atoi(valStr)
}

func CopyMap[K comparable, V any](from map[K]V) map[K]V {
	to := make(map[K]V)
	for k, v := range from {
		to[k] = v
	}
	return to
}
