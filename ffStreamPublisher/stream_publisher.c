#include "conio.h"

#define inline __inline

#ifdef __cplusplus
extern "C" {
#endif

#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h" 
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h" 
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"  
#include "libavutil/time.h"  
#include "libavutil/avutil.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/mathematics.h"

#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avdevice.lib")
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"swresample.lib")

#ifdef __cplusplus
};
#endif

typedef struct SwsContext SWSContext;

#define MAX_NAME_LEN 1024

#define CAP_SCREEN 1
#define CAP_CAMERA 2

#define CAP_DEVICE CAP_CAMERA

char* camera = "video=Lenovo EasyCamera"; // camera name

char* output = "rtmp://127.0.0.1/live/stream"; // output: RTMP publish stream
//char* output = "saved_video.mp4";            // output: saveas video file

const int fps = 20; // voide FPS

int main(int argc, char** argv)
{
	char URL[MAX_NAME_LEN] = { 0 };

	if (argc > 1)
	{
		strcpy(URL, argv[1]);
	}
	else
	{
		strcpy(URL, output);
	}

	av_register_all();
	avformat_network_init();
	avdevice_register_all();

	AVFormatContext* vgdiFormatContext = avformat_alloc_context();

	int width = 1280;
	int height = 720;

	AVDictionary* vgdiOptions = NULL;
	char c_fps[8] = { 0 };
	itoa(fps, c_fps, 10);
	av_dict_set(&vgdiOptions, "frame_rate", c_fps, 0);

#if(CAP_DEVICE == CAP_SCREEN) // captyre screen

	av_dict_set(&vgdiOptions, "offset_x", "0", 0);
	av_dict_set(&vgdiOptions, "offset_y", "0", 0);
	av_dict_set(&vgdiOptions, "video_size", "1280x720", 0);

	AVInputFormat* inputFormat = av_find_input_format("gdigrab");
	avformat_open_input(&vgdiFormatContext, "desktop", inputFormat, NULL);


#elif(CAP_DEVICE == CAP_CAMERA) // camera

	AVInputFormat* inputFormat = av_find_input_format("dshow");
	avformat_open_input(&vgdiFormatContext, camera, inputFormat, NULL);

#endif

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

	AVCodecContext* vgdiCodecContext = vgdiFormatContext->streams[vIdx]->codec;
	AVCodec* vgdiCodec = avcodec_find_decoder(vgdiCodecContext->codec_id);
	avcodec_open2(vgdiCodecContext, vgdiCodec, NULL);

	int voutPixfmt = AV_PIX_FMT_YUV420P;

	SWSContext* swsContext = sws_getContext(width, height, vgdiCodecContext->pix_fmt,
		width, height, voutPixfmt, SWS_BILINEAR, NULL, NULL, NULL);

	int buffLen = avpicture_get_size(voutPixfmt, width, height);
	uint8_t* buffer = (uint8_t *)av_malloc(buffLen);

	AVFrame* voutFrame = av_frame_alloc();
	avpicture_fill((AVPicture*)voutFrame, buffer, voutPixfmt, width, height);

	AVFormatContext* voutFormatContext = NULL;
	avformat_alloc_output_context2(&voutFormatContext, NULL, "flv", URL);

	avio_open(&(voutFormatContext->pb), URL, AVIO_FLAG_WRITE);

	AVStream* voutStream = avformat_new_stream(voutFormatContext, NULL);

	AVCodecContext* voutCodecContext = voutStream->codec;
	voutCodecContext->codec_id = voutFormatContext->oformat->video_codec;
	voutCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
	voutCodecContext->pix_fmt = voutPixfmt;
	voutCodecContext->width = width;
	voutCodecContext->height = height;
	voutCodecContext->bit_rate = 1 * 1024 * 1024;
	voutCodecContext->gop_size = 250;
	voutCodecContext->time_base.num = 1;
	voutCodecContext->time_base.den = fps;
	voutCodecContext->qmin = 10; //?
	voutCodecContext->qmax = 51; //?
	//voutCodecContext->max_b_frames = 3; //FLV is currently not supported

	AVDictionary* voutOptions = NULL;

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

	AVCodec* voutCodec = avcodec_find_encoder(voutCodecContext->codec_id);
	avcodec_open2(voutCodecContext, voutCodec, &voutOptions);

	avformat_write_header(voutFormatContext, NULL);

	int nFramesRecorded = 0;

	while (1)
	{
		if (kbhit()) break;

		AVPacket* gdiPacket = av_packet_alloc(); // ALLOC_PACKET_IN
		AVFrame* gdiFrame = av_frame_alloc();    // ALLOC_FRAME_IN

		av_read_frame(vgdiFormatContext, gdiPacket);

		int got;
		avcodec_decode_video2(vgdiCodecContext, gdiFrame, &got, gdiPacket);

		sws_scale(swsContext, gdiFrame->data, gdiFrame->linesize, 0, height,
			voutFrame->data, voutFrame->linesize);
		voutFrame->format = voutCodecContext->pix_fmt;
		voutFrame->width = width;
		voutFrame->height = height;
		voutFrame->pts = nFramesRecorded*(voutStream->time_base.den) / ((voutStream->time_base.num) * fps);

		AVPacket* outPacket = av_packet_alloc(); // ALLOC_PACKET_OUT
		avcodec_encode_video2(voutCodecContext, outPacket, voutFrame, &got);

		outPacket->stream_index = voutStream->index;

		av_write_frame(voutFormatContext, outPacket);

		++nFramesRecorded;

		printf("Frame = %05d, PacketSize = %d\n", nFramesRecorded, outPacket->size);

		av_frame_free(&gdiFrame);   // FREE_FRAME_IN
		av_free_packet(gdiPacket);  // FREE_PACKET_IN
		av_free_packet(outPacket);  // FREE_PACKET_OUT
	}

	av_write_trailer(voutFormatContext);

	av_frame_free(&voutFrame);
	sws_freeContext(swsContext);

	avcodec_close(voutCodecContext);
	avio_close(voutFormatContext->pb);
	avformat_free_context(voutFormatContext);

	avformat_close_input(&vgdiFormatContext);
	av_free(vgdiFormatContext);

	system("pause");
	return 0;
}