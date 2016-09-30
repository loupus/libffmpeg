#include "video_decoder.h"
#include "ffmpeg.h"

VideoDecoder::VideoDecoder(AVCodecParameters* codec_param)
{
	const auto codec_ =
		avcodec_find_decoder(codec_param->codec_id);
	if (!codec_) {
		throw ffmpeg::Error{"Not found decoder for codec"};
	}
	codec_context_ = avcodec_alloc_context3(codec_);
	avcodec_parameters_to_context(codec_context_, codec_param);
	ffmpeg::check(avcodec_open2(
		codec_context_, codec_, nullptr)
	);
}

VideoDecoder::~VideoDecoder() 
{
	avcodec_free_context(&codec_context_);
}

unsigned VideoDecoder::width() const
{
	return codec_context_->width;
}

unsigned VideoDecoder::height() const
{
	return codec_context_->height;
}

AVPixelFormat VideoDecoder::pixel_format() const
{
	return codec_context_->pix_fmt;
}

void VideoDecoder::operator()(
	AVFrame* frame, int &finished, AVPacket* packet)
{
	finished = decode(frame, packet);
}

bool VideoDecoder::decode(AVFrame* frame, AVPacket* packet)
{
	int send_packet = avcodec_send_packet(codec_context_, packet);
	int receive_frame = avcodec_receive_frame(codec_context_,frame);

	return (send_packet == 0) && (receive_frame == 0);
}