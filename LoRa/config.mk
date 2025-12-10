# Configuración de compilación para LoRaWAN

VERBOSE=n
OPT=g
USE_NANO=n
SEMIHOST=n
USE_FPU=y

USE_LPCOPEN=y
USE_SAPI=y

PROJECT_INCLUDES = inc

PROJECT_SRCS = \
	main.c \
	src/sx126x.c \
	src/sx126x-hal.c \
	src/radio_adapter.c \
	inc/LoRaMac.c \
	inc/LoRaMacAdr.c \
	inc/LoRaMacClassB.c \
	inc/LoRaMacCommands.c \
	inc/LoRaMacConfirmQueue.c \
	inc/LoRaMacCrypto.c \
	inc/LoRaMacParser.c \
	inc/LoRaMacSerializer.c \
	inc/region/Region.c \
	inc/region/RegionCommon.c \
	inc/region/RegionAU915.c \
	inc/timer.c \
	inc/systime.c \
	inc/delay.c
