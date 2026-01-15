# EDU-CIAA LoRa Transmitter

VERBOSE=n
OPT=g
USE_NANO=n
SEMIHOST=n
USE_FPU=y

USE_LPCOPEN=y
USE_SAPI=y

PROJECT_INCLUDES = src

PROJECT_SRCS = \
	src/program_main.c \
	src/leer_adc.c \
	src/sx1262_lora.c
