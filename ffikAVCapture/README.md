# ffikAVCapture
AudioVidoCapture based on FFmpeg (GDI Grab) & irrKlang

### How to use

    //BEGIN
    FFIKRecorder recorder;
    //recorder.SetVideoRecordOptions(); // if not set, defaults will be used
    recorder.StartRecordingVideo(); // start capturing video
    //recorder.StartRecordingAudio(); // start recording audio
    int n = 0;
    while (true)
    {
      if (kbhit()) break;
      recorder.GrabVideoFrame();
    }
    //END
