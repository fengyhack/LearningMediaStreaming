#include "FFIKRecorder.h"
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>

const int X0 = 0;
const int Y0 = 0;
const int CX = 1920;
const int CY = 1080;

void main(void)
{
	system("title gdigrab");
	system("color 0e");
	system("title ffcap (https://fengyh.cn/)");

	//char vFile[256] = { 0 };

	int fps = 10, x0 = 0, y0 = 0, w = 1280, h = 720;

	FFIKRecorder recorder;

	recorder.SetVideoRecordOptions("",fps, x0, y0, w, h); // if not set, defaults will be used
	recorder.StartRecordingVideo(); // start capturing video
	
#ifdef RECORD_SOUND	
	char* aFile = "soundRec.mp3";
    recorder.SetAudioRecordOptions(aFile); // if not set, defaults will be used
	recorder.StartRecordingAudio(); // start recording audio
#endif
	int n = 0;
	while (true)
	{
		if (kbhit()) break;
		recorder.GrabVideoFrame();
		if (n < 1)
		{
			system("cls");
		}
		printf(".");
		if (++n % 100 == 0) printf("\n");
	}
	
#ifdef RECORD_SOUND	
	recorder.StopRecordingAudio(); // this will be called automatically
	recorder.StopRecordingVideo(); // this will be called automatically
#endif

	//END
}