#pragma once
extern "C" {
	#include "libavcodec/avcodec.h"
}

class AudioDecoder {
public:
	AudioDecoder(AVCodecContext* codec_context);
	~AudioDecoder();
    void operator() (AVFrame* frame, int &finished, AVPacket* packet);
	AVRational time_base() const;
private:
	AVCodecContext* codec_context_{};
};
