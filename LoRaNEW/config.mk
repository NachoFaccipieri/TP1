# LoRa SX1262 Simple Test

VERBOSE=n
OPT=g
USE_NANO=n
SEMIHOST=n
USE_FPU=y

USE_LPCOPEN=y
USE_SAPI=y

PROJECT_INCLUDES = inc

PROJECT_SRCS = \
	src/main.c \
	src/sx126x.c \
	src/radio.c \
	src/LoRaMac.c \
	src/aes.c
