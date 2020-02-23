#include "FFIKRecorder.h"
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>

const int X0 = 0;
const int Y0 = 0;
const int CX = 3000;
const int CY = 2000;

void main(void)
{
	system("title gdigrab");
	system("color 0e");
	//system("title FFIKRecorder (Press ANY key to continue)");
	system("title ffcap (http://fengyh.cn/)");

	//char vFile[256] = { 0 };

	int fps = 10, x0 = 0, y0 = 0, w = 2544, h = 1612;

	FFIKRecorder recorder;

	recorder.SetVideoRecordOptions("",fps, x0, y0, w, h); // if not set, defaults will be used
	recorder.StartRecordingVideo(); // start capturing video
	//recorder.SetAudioRecordOptions(aFile); // if not set, defaults will be used
	//recorder.StartRecordingAudio(); // start recording audio

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
	
	//recorder.StopRecordingAudio(); // this will be called automatically
	//recorder.StopRecordingVideo(); // this will be called automatically
	
	//END
}