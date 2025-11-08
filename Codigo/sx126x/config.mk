# Compile options
VERBOSE=n
OPT=g
USE_NANO=n
SEMIHOST=n
USE_FPU=y

# Libraries
USE_LPCOPEN=y
USE_SAPI=y

PROJECT_INCLUDES = inc

PROJECT_SRCS = \
	src/main.c \
	src/sx126x.c \
	src/sx126x_hal.c \
	src/sx126x_driver_version.c \
	src/sx126x.c \
	src/sx126x_lr_fhss.c \
	src/sx126x_bpsk.c \
	src/lr_fhss_mac.c