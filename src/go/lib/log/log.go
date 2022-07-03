package bon_log

import (
	"log"
	"os"
)

var flags = log.LstdFlags|log.Lmsgprefix|log.Lshortfile

// Info writes logs in the color blue with "INFO: " as prefix
var Info = log.New(os.Stdout, "\033[32m[info] \033[0m", flags)

// Warning writes logs in the color yellow with "WARNING: " as prefix
var Warn = log.New(os.Stdout, "\033[33m[warn] \033[0m", flags)

// Error writes logs in the color red with "ERROR: " as prefix
var Error = log.New(os.Stdout, "\033[31m[error] \033[0m", flags)

// Debug writes logs in the color cyan with "DEBUG: " as prefix
var Debug = log.New(os.Stdout, "\033[36m[debug] \033[0m", flags)