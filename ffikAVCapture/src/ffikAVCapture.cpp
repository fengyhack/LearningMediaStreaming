#include "FFIKRecorder.h"
#include <conio.h>
#include <stdlib.h>

void main(void)
{
	system("title gdigrab");
	system("color 0e");
	system("title FFIKRecorder (Press ANY key to continue)");

	char* vFile = "F:\\demo\\gdigrab\\1.mp4";
	int fps = 10, x0 = 200, y0 = 100, w = 800, h = 500;

	FFIKRecorder recorder;

	recorder.SetVideoRecordOptions(vFile,fps, x0, y0, w, h); // if not set, defaults will be used
	recorder.StartRecordingVideo(); // start capturing video
	//recorder.SetAudioRecordOptions(aFile); // if not set, defaults will be used
	//recorder.StartRecordingAudio(); // start recording audio

	int n = 0;
	while (true)
	{
		if (kbhit()) break;
		recorder.GrabVideoFrame();
		printf(".");
		if (++n % 60 == 0) printf("\n");
	}
	
	//recorder.StopRecordingAudio(); // this will be called automatically
	//recorder.StopRecordingVideo(); // this will be called automatically
	
	//END
}