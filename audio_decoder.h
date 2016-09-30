#pragma once
extern "C" {
	#include <libavcodec/avcodec.h>
    #include <ao/ao.h>
}

class AudioDecoder {
public:
	AudioDecoder(AVCodecParameters* codec_param);
	~AudioDecoder();
    void operator() (int &finished, AVPacket* packet);
    void play(int plane_size);
    int get_channels();
    AVSampleFormat  sfmt;
    int             buffer_size;
    AVFrame         *frame;
    uint8_t         *buffer;
private:
    AVCodecContext  *codec_context_{};
    ao_device       *adevice;
    uint8_t         *samples;
    uint16_t        *out;
    bool decode(AVFrame* frame, AVPacket* packet);
};
