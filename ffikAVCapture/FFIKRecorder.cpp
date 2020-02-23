#include "FFIKRecorder.h"
#include <Windows.h> // GetSystemMetrics()

FFIKRecorder::FFIKRecorder()
{
	Init();
}

FFIKRecorder::~FFIKRecorder()
{
	Shutdown();
}

void FFIKRecorder::Init()
{
	fpLog = fopen("FFIK.log", "a");
	WriteLog(fpLog, "FFIK stratup");

	vgdiOptions = NULL;
	vgdiFormatContext = NULL;
	vgdiCodecContext = NULL;
	vgdiCodec = NULL;

	swsConvertContext = NULL;
	voutFrame = NULL;

	voutOptions = NULL;
	voutFormatContext = NULL;
	voutCodecContext = NULL;
	voutCodec = NULL;
	voutPixFmt = AV_PIX_FMT_YUV420P;
	voutStream = NULL;

	isVideoRecordOptionsSet = false;
	isVideoRecordActive = false;
	nFramesRecorded = 0;
	videoBitRate = 1 * 1024 * 1024;
	width = 0;
	height = 0;

#ifdef RECORD_SOUND
	irrSoundEngine = NULL;
	irrAudioRecorder = NULL;

	isAudioRecordOptionsSet = false;
	isAudioRecordActive = false;
	audioBitRate = 44100;
#endif

	memset(videoFileName, 0, MAX_NAME_LEN);

#ifdef RECORD_SOUND
	memset(audioFileName, 0, MAX_NAME_LEN);
#endif
}

void FFIKRecorder::Shutdown()
{
	if (isVideoRecordActive)
	{
		StopRecordingVideo();
	}

#ifdef RECORD_SOUND
	if (isAudioRecordActive)
	{
		StopRecordingAudio();
	}
#endif

	if (fpLog != NULL)
	{
		WriteLog(fpLog, "FFIK shutdown\r\n");
		fclose(fpLog);
	}
}

void FFIKRecorder::GetDateTime(char str[], const char* prefix, const char* suffix)
{
	SYSTEMTIME st;
	GetSystemTime(&st);
	int day = st.wDay;
	int hour = st.wHour + 8;
	day = hour >= 24 ? day + 1 : day;
	hour %= 24;

	sprintf(str, "%s%4d%02d%02d%02d%02d%02d%s",
		prefix, st.wYear, st.wMonth, day, hour, st.wMinute, st.wSecond, suffix);
}

void FFIKRecorder::GetDateTimeSpec(char sDateTime[])
{
	SYSTEMTIME st;
	GetSystemTime(&st);
	int day = st.wDay;
	int hour = st.wHour + 8;
	day = hour >= 24 ? day + 1 : day;
	hour %= 24;

	sprintf(sDateTime, "%4d-%02d-%02d %02d:%02d:%02d.%03d",
		st.wYear, st.wMonth, day, hour, st.wMinute, st.wSecond, st.wMilliseconds);
}

void FFIKRecorder::WriteLog(FILE* fp, const char* msg)
{
	if (fp != NULL)
	{
		char st[MAX_NAME_LEN] = { 0 };
		GetDateTimeSpec(st);
		fprintf(fp, "[%s] %s\r\n", st, msg);
	}
}

void FFIKRecorder::SetVideoRecordOptions(const char* fn, int fps, int x, int y, int w, int h)
{
	if (isVideoRecordOptionsSet)
	{
		return;
	}

	// Check filename
	if (strcmp(fn, ""))
	{
		// if fn is not empty
		strcpy(videoFileName, fn);
	}
	else
	{
		// if fn is empty
		GetDateTime(videoFileName, "FFIKRec_", ".mp4");
	}

	// check fps
	if (fps < 2 || fps>200)
	{
		fps = 25;
	}

	// check x,y,cx,cy
	//int cx = GetSystemMetrics(SM_CXSCREEN);
	//int cy = GetSystemMetrics(SM_CYSCREEN);

	int cx = 2998;
	int cy = 2000;

	cx = (cx >> 2) << 2; // (cx/4)*4 --> cx = 4*n

	if (x<0 || x>cx - 50)
	{
		x = 0;
	}

	if (y<0 || y>cy - 50)
	{
		y = 0;
	}

	if (w < 50 || w>cx-x)
	{
		w = cx - x;
	}

	if (h<50 || h>cy - y)
	{
		h = cy - y;
	}

	char FPS[8] = { 0 };
	char offX[8] = { 0 };
	char offY[8] = { 0 };
	char vSize[16] = { 0 };

	sprintf(FPS, "%d", fps);
	sprintf(offX, "%d", (x >> 2) << 2); // x--> x_4a
	sprintf(offY, "%d", y);
	sprintf(vSize, "%dx%d", w, h);

	av_dict_set(&vgdiOptions, "list_devices", "true", 0);
	av_dict_set(&vgdiOptions, "frame_rate", FPS, 0);
	av_dict_set(&vgdiOptions, "offset_x", offX, 0);
	av_dict_set(&vgdiOptions, "offset_y", offY, 0);
	av_dict_set(&vgdiOptions, "video_size", vSize, 0);

	isVideoRecordOptionsSet = true;

	char msg[MAX_NAME_LEN] = { 0 };
	sprintf(msg, "Video file: %s",	videoFileName);
	WriteLog(fpLog, msg);
	memset(msg, 0, MAX_NAME_LEN);
	sprintf(msg, "Video options: fps = %s, offset_x = %s, offset_y = %s, video_size = %s",
		FPS, offX, offY, vSize);
	WriteLog(fpLog, msg);
}

void FFIKRecorder::StartRecordingVideo()
{
	if (isVideoRecordActive)
	{
		return;
	}

	av_register_all();
	avformat_network_init();
	avdevice_register_all();

	if (!isVideoRecordOptionsSet)
	{
		WriteLog(fpLog, "Set \'VideoRecordOptions\' to default");
		SetVideoRecordOptions();
	}

	AVInputFormat *videoInputFormat = av_find_input_format("gdigrab");
	vgdiFormatContext = avformat_alloc_context();
	avformat_open_input(&vgdiFormatContext, "desktop", videoInputFormat, &vgdiOptions);
	avformat_find_stream_info(vgdiFormatContext, NULL);

	int vIdx = -1;
	for (int i = 0; i < vgdiFormatContext->nb_streams; ++i)
	{
		if (vgdiFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			vIdx = i;
			break;
		}
	}

	vgdiCodecContext = vgdiFormatContext->streams[vIdx]->codec;
	vgdiCodec = avcodec_find_decoder(vgdiCodecContext->codec_id);
	avcodec_open2(vgdiCodecContext, vgdiCodec, &vgdiOptions);

	width = vgdiCodecContext->width;
	height = vgdiCodecContext->height;

	swsConvertContext = sws_getContext(width, height, vgdiCodecContext->pix_fmt,
		width, height, voutPixFmt, SWS_BILINEAR, NULL, NULL, NULL);

	int buffLen = avpicture_get_size(voutPixFmt, width, height);
	uint8_t* buffer = (uint8_t *)av_malloc(buffLen);

	voutFrame = av_frame_alloc();
	avpicture_fill((AVPicture*)voutFrame, buffer, voutPixFmt, width, height);

	avformat_alloc_output_context2(&voutFormatContext, NULL, NULL, videoFileName);

	avio_open(&(voutFormatContext->pb), videoFileName, AVIO_FLAG_WRITE);

	voutStream = avformat_new_stream(voutFormatContext, NULL);

	voutCodecContext = voutStream->codec;
	voutCodecContext->codec_id = voutFormatContext->oformat->video_codec;
	voutCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
	voutCodecContext->pix_fmt = voutPixFmt;
	voutCodecContext->width = width;
	voutCodecContext->height = height;
	voutCodecContext->bit_rate = 1 * 1024 * 1024;
	voutCodecContext->gop_size = 250;
	voutCodecContext->time_base.num = 1;
	voutCodecContext->time_base.den = 25;
	voutCodecContext->qmin = 10; //?
	voutCodecContext->qmax = 51; //?
	voutCodecContext->max_b_frames = 3; //?

	//H.264  
	if (voutCodecContext->codec_id == AV_CODEC_ID_H264) 
	{
		av_dict_set(&voutOptions, "preset", "slow", 0);
		av_dict_set(&voutOptions, "tune", "zerolatency", 0);
		//av_dict_set(&voutOptions, "profile", "main", 0);  
	}
	//H.265  
	if (voutCodecContext->codec_id == AV_CODEC_ID_H265)
	{
		av_dict_set(&voutOptions, "preset", "ultrafast", 0);
		av_dict_set(&voutOptions, "tune", "zero-latency", 0);
	}

	voutCodec = avcodec_find_encoder(voutCodecContext->codec_id);
	avcodec_open2(voutCodecContext, voutCodec, &voutOptions);

	avformat_write_header(voutFormatContext, NULL);

	isVideoRecordActive = true;

	WriteLog(fpLog, "Start recording video...");
}

bool FFIKRecorder::GrabVideoFrame()
{
	if (!isVideoRecordActive)
	{
		WriteLog(fpLog,"[ERROR] Please call \'StartRecordingVideo\' (once) before \'GrabFrame\'");
		return false;
	}

	AVPacket* gdiPacket = av_packet_alloc(); // ALLOC_PACKET_IN
	AVFrame* gdiFrame = av_frame_alloc();   // ALLOC_FRAME_IN

	av_read_frame(vgdiFormatContext, gdiPacket);
	
	int got;
	avcodec_decode_video2(vgdiCodecContext, gdiFrame, &got, gdiPacket);

	sws_scale(swsConvertContext, gdiFrame->data, gdiFrame->linesize, 0, height, 
		voutFrame->data, voutFrame->linesize);
	voutFrame->format = voutCodecContext->pix_fmt;
	voutFrame->width = width;
	voutFrame->height = height;
	voutFrame->pts = nFramesRecorded*(voutStream->time_base.den) / ((voutStream->time_base.num) * 25);
	
	AVPacket* outPacket = av_packet_alloc(); // ALLOC_PACKET_OUT
	avcodec_encode_video2(voutCodecContext, outPacket, voutFrame, &got);

	outPacket->stream_index = voutStream->index;
	
	av_write_frame(voutFormatContext, outPacket);

	++nFramesRecorded;

	char msg[MAX_NAME_LEN] = { 0 };
	sprintf(msg, "Frame #%05d, PacketSize = %d", nFramesRecorded, outPacket->size);
	WriteLog(fpLog, msg);

	av_frame_free(&gdiFrame); // FREE_FRAME_IN
	av_free_packet(gdiPacket);  // FREE_PACKET_IN
	av_free_packet(outPacket);  // FREE_PACKET_OUT

	return true;
}

void FFIKRecorder::StopRecordingVideo()
{
	if (!isVideoRecordActive)
	{
		return;
	}

	av_write_trailer(voutFormatContext);

	av_frame_free(&voutFrame);
	sws_freeContext(swsConvertContext);

	avcodec_close(voutCodecContext);
	avio_close(voutFormatContext->pb);
	avformat_free_context(voutFormatContext);

	avformat_close_input(&vgdiFormatContext);
	av_free(vgdiFormatContext);

	isVideoRecordActive = false;

	WriteLog(fpLog, "Stop recording video");
}

#ifdef RECORD_SOUND

void FFIKRecorder::SetAudioRecordOptions(const char* fn,int abr)
{
	if (isAudioRecordOptionsSet)
	{
		return;
	}

	if (strcmp(fn, ""))
	{
		// if fn is not empty
		strcpy(audioFileName, fn);
	}
	else
	{
		// if fn is empty
		GetDateTime(audioFileName, "FFIKRec_", ".wav");
	}

	if (abr < 12800 || abr>6400000)
	{
		abr = 44100;
	}

	audioBitRate = abr;

	isAudioRecordOptionsSet = true;

	char msg[MAX_NAME_LEN] = { 0 };
	sprintf(msg, "Audio file: %s", audioFileName,audioBitRate);
	WriteLog(fpLog, msg);
	memset(msg, 0, MAX_NAME_LEN);
	sprintf(msg, "Audio options: BitRate = %d", audioBitRate);
	WriteLog(fpLog, msg);
}

void FFIKRecorder::StartRecordingAudio()
{
	if (isAudioRecordActive)
	{
		return;
	}

	if (!isAudioRecordOptionsSet)
	{
		WriteLog(fpLog, "Set \'AudioRecordOptions\' to default");
		SetAudioRecordOptions();
	}

	irrSoundEngine = createIrrKlangDevice();
	irrAudioRecorder = createIrrKlangAudioRecorder(irrSoundEngine);
	irrAudioRecorder->startRecordingBufferedAudio(audioBitRate);

	isAudioRecordActive = true;
	WriteLog(fpLog, "Start recording audio...");
}

void FFIKRecorder::StopRecordingAudio()
{
	if (!isAudioRecordActive)
	{
		return;
	}

	irrAudioRecorder->stopRecordingAudio();
	SaveAudioAsWaveFile();

	isAudioRecordActive = false;
	WriteLog(fpLog, "Stop recording audio");
}

void FFIKRecorder::SaveAudioAsWaveFile()
{
	if (irrAudioRecorder->getRecordedAudioData() == NULL)
	{
		WriteLog(fpLog,"[ERROR] No audio recorded");
		return;
	}

	SAudioStreamFormat format = irrAudioRecorder->getAudioFormat();
	void* data = irrAudioRecorder->getRecordedAudioData();
	unsigned short formatType = 1;
	unsigned short numChannels = format.ChannelCount;
	unsigned long  sampleRate = format.SampleRate;
	unsigned short bitsPerChannel = format.getSampleSize() * 8;
	unsigned short bytesPerSample = format.getFrameSize();
	unsigned long  bytesPerSecond = format.getBytesPerSecond();
	unsigned long  dataLen = format.getSampleDataSize();

	const int fmtChunkLen = 16;
	const int waveHeaderLen = 4 + 8 + fmtChunkLen + 8;

	unsigned long totalLen = waveHeaderLen + dataLen;
		
	FILE* fpAudio = fopen(audioFileName, "wb");

	// write wave header 
	fwrite("RIFF", 4, 1, fpAudio);
	fwrite(&totalLen, 4, 1, fpAudio);
	fwrite("WAVE", 4, 1, fpAudio);
	fwrite("fmt ", 4, 1, fpAudio);
	fwrite(&fmtChunkLen, 4, 1, fpAudio);
	fwrite(&formatType, 2, 1, fpAudio);
	fwrite(&numChannels, 2, 1, fpAudio);
	fwrite(&sampleRate, 4, 1, fpAudio);
	fwrite(&bytesPerSecond, 4, 1, fpAudio);
	fwrite(&bytesPerSample, 2, 1, fpAudio);
	fwrite(&bitsPerChannel, 2, 1, fpAudio);
	// write data
	fwrite("data", 4, 1, fpAudio);
	fwrite(&dataLen, 4, 1, fpAudio);
	fwrite(data, dataLen, 1, fpAudio);

	fclose(fpAudio);
}

#endif
