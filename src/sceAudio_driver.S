	.set noreorder

#include "pspimport.s"

	IMPORT_START	"sceAudio_driver",0x00010000
	IMPORT_FUNC	"sceAudio_driver",0x136CAF51,sceAudioOutputBlocking
	IMPORT_FUNC	"sceAudio_driver",0x5EC81C55,sceAudioChReserve
	IMPORT_FUNC	"sceAudio_driver",0x6FC46853,sceAudioChRelease
