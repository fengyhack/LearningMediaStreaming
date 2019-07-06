#include <stdio.h>
#include "include/librtmp/rtmp_sys.h"
#include "include/librtmp/log.h"

#pragma comment(lib,"lib/librtmp")
#pragma comment(lib,"ws2_32.lib")

#define MAX_NAME_LEN 1024

//10 Mega
#define MAX_BUFF_SIZE 10485760

char* stream = "rtmp://127.0.0.1/live/stream";
char* output = "saved_video.flv";

int main(int argc, char** argv)
{
	char outFile[MAX_NAME_LEN] = { 0 };
	char URL[MAX_NAME_LEN] = { 0 };

	if (argc > 2)
	{
		strcpy(outFile, argv[1]);
		strcpy(URL, argv[2]);
	}
	else if (argc>1)
	{
		strcpy(outFile, argv[1]);
		strcpy(URL, stream);
	}
	else
	{
		strcpy(outFile, output);
		strcpy(URL, stream);
	}

	printf("Saveas \'%s\', RTMP \'%s\'\n", outFile, URL);

	WORD version = MAKEWORD(1, 1);
	WSADATA wsaData;
	WSAStartup(version, &wsaData);		

	RTMP *rtmp = RTMP_Alloc();
	RTMP_Init(rtmp);
	//rtmp->Link.timeout=10;	

	if(!RTMP_SetupURL(rtmp,URL))
	{
		printf("RTMP_SetupURL Error\n");
		RTMP_Free(rtmp);
		WSACleanup();
		return -1;
	}

	rtmp->Link.lFlags|=RTMP_LF_LIVE;
	
	//1hour
	RTMP_SetBufferMS(rtmp, 3600*1000);		
	
	if(!RTMP_Connect(rtmp,NULL))
	{
		RTMP_Free(rtmp);
		WSACleanup();
		return -1;
	}

	if(!RTMP_ConnectStream(rtmp,0))
	{
		RTMP_Close(rtmp);
		RTMP_Free(rtmp);
		WSACleanup();
		return -1;
	}

	int n, total = 0;

	char *buffer = (char*)malloc(MAX_BUFF_SIZE);
	memset(buffer, 0, MAX_BUFF_SIZE);

	FILE *fp = fopen(outFile, "wb");

	double mega = 1024.0 * 1024;

	while(1)
	{
		n = RTMP_Read(rtmp, buffer, MAX_BUFF_SIZE);

		if (n < 1) break;

		fwrite(buffer,1,n,fp);

		total += n;

		printf("Receive: %6dB, Total: %5.3fMB\n", n, total / mega);
	}

	if (fp!=NULL)
	{
		fclose(fp);
	}

	if(buffer!=NULL)
	{
		free(buffer);
	}

	if(rtmp!=NULL)
	{
		RTMP_Close(rtmp);
		RTMP_Free(rtmp);
		WSACleanup();
	}	

	system("pause");

	return 0;
}