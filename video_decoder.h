#pragma once
#include "queue.h"
#include "format_converter.h"
extern "C" {
	#include "libavcodec/avcodec.h"
}

class VideoDecoder {
public:
	VideoDecoder(AVCodecContext* codec_context);
	~VideoDecoder(); void operator()(AVFrame* frame, int &finished, AVPacket* packet);
	void decode_video();
	unsigned width() const;
	unsigned height() const;
	AVPixelFormat pixel_format() const;
	AVRational time_base() const;
private:
	AVCodecContext* codec_context_{};
	std::unique_ptr<PacketQueue> packet_queue_;
	std::unique_ptr<FrameQueue> frame_queue_;
	std::unique_ptr<FormatConverter> format_converter_;
	static const size_t queue_size_{5};
};
