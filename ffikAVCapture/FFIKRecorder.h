#pragma once

extern "C" {
#define __STDC_CONSTANT_MACROS
#include <libavdevice\avdevice.h>
#include <libavcodec\avcodec.h>
#include <libavformat\avformat.h>
#include <libavutil\avutil.h>
#include <libswscale\swscale.h>
};

#ifdef RECORD_SOUND

#include <irrKlang.h>

using namespace irrklang;

#endif

#pragma comment(lib,"avdevice.lib")
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"swscale.lib")

#ifdef RECORD_SOUND

#pragma comment(lib,"irrKlang.lib")

#endif

const int MAX_NAME_LEN = 1024;

class FFIKRecorder
{
public:
	FFIKRecorder();
	~FFIKRecorder();

public:
	void SetVideoRecordOptions(const char* fn="",int fps = 25, int x = 0, int y = 0, int w = 0, int h = 0);
	void StartRecordingVideo();
	bool GrabVideoFrame(); // return true if grabbed the frame, or false is failed
	void StopRecordingVideo();
	int FramesRecorded() { return nFramesRecorded; } // how many frames have been recorded

#ifdef RECORD_SOUND
	void SetAudioRecordOptions(const char* fn = "", int abr = 44100);
	void StartRecordingAudio();
	void StopRecordingAudio();
#endif

private:
	void Init();
	void Shutdown();

#ifdef RECORD_SOUND
	void SaveAudioAsWaveFile();
#endif

	void GetDateTime(char str[], const char* prefix = "", const char* suffix = "");
	void GetDateTimeSpec(char sDateTime[]);
	void WriteLog(FILE* fp, const char* msg = "");

private:
	AVDictionary* vgdiOptions;
	AVFormatContext* vgdiFormatContext;
	AVCodecContext* vgdiCodecContext;
	AVCodec* vgdiCodec;

	SwsContext* swsConvertContext;
	AVFrame* voutFrame;

	AVDictionary* voutOptions;
	AVFormatContext* voutFormatContext;
	AVCodecContext* voutCodecContext;
	AVCodec* voutCodec;
	AVPixelFormat voutPixFmt;
	AVStream* voutStream;

	bool isVideoRecordOptionsSet;
	bool isVideoRecordActive;
	int nFramesRecorded;
	int videoBitRate;
	int width;
	int height;

#ifdef RECORD_SOUND
	ISoundEngine* irrSoundEngine;
	IAudioRecorder* irrAudioRecorder;
	bool isAudioRecordOptionsSet;
	bool isAudioRecordActive;
	int audioBitRate;
#endif

	char videoFileName[MAX_NAME_LEN];

#ifdef RECORD_SOUND
	char audioFileName[MAX_NAME_LEN];
#endif
	
	FILE* fpLog;
};

