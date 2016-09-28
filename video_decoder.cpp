#include <iostream>
#include "video_decoder.h"
#include "ffmpeg.h"
extern "C" {
	#include <libavutil/imgutils.h>
}

VideoDecoder::VideoDecoder(AVCodecContext* codec_context) :
	codec_context_{codec_context},
	packet_queue_{new PacketQueue{queue_size_}},
	frame_queue_{new FrameQueue{queue_size_}},
	format_converter_ 
	{
		std::move(std::unique_ptr<FormatConverter>(new FormatConverter(
			this->width(),
			this->height(),
			this->pixel_format(),
			AV_PIX_FMT_YUV420P
		)))
	}
	{
	avcodec_register_all();
	const auto codec_video =
		avcodec_find_decoder(codec_context_->codec_id);
	if (!codec_video) {
		throw ffmpeg::Error{"Unsupported video codec"};
	}
	ffmpeg::check(avcodec_open2(
		codec_context_, codec_video, nullptr));
}

VideoDecoder::~VideoDecoder() {
	avcodec_close(codec_context_);
}

void VideoDecoder::operator()(
	AVFrame* frame, int &finished, AVPacket* packet) {
	ffmpeg::check(avcodec_decode_video2(
		codec_context_, frame, &finished, packet));
}

unsigned VideoDecoder::width() const {
	return codec_context_->width;
}

unsigned VideoDecoder::height() const {
	return codec_context_->height;
}

AVPixelFormat VideoDecoder::pixel_format() const {
	return codec_context_->pix_fmt;
}

AVRational VideoDecoder::time_base() const {
	return codec_context_->time_base;
}

void VideoDecoder::decode_video()
{
	const AVRational microseconds = {1, 1000000};

	try {
		for (;;) {
			// Create AVFrame and AVQueue
			std::unique_ptr<AVFrame, std::function<void(AVFrame*)>> frame_decoded
			{
				av_frame_alloc(),
				[](AVFrame* f) { av_frame_free(&f); }
			};
			std::unique_ptr<AVPacket, std::function<void(AVPacket*)>> packet
			{
				nullptr,
				[](AVPacket* p) { av_packet_unref(p); delete p; }
			};

			// Read packet from queue
			if (!packet_queue_->pop(packet)) {
				frame_queue_->finished();
				break;
			}

			// Decode packet
			int finished_frame;
			ffmpeg::check(avcodec_decode_video2(
				codec_context_, frame_decoded.get(), &finished_frame, packet.get()
			));

			// If a whole frame has been decoded,
			// adjust time stamps and add to queue
			if (finished_frame) {
				frame_decoded->pts = av_rescale_q(
					frame_decoded->pkt_dts,
					this->time_base(),
					microseconds);

				std::unique_ptr<AVFrame, std::function<void(AVFrame*)>> frame_converted
				{
					av_frame_alloc(),
					[](AVFrame* f) { av_free(f->data[0]); }
				};
				if (av_frame_copy_props(frame_converted.get(),
				    frame_decoded.get()) < 0) {
					throw std::runtime_error("Copying frame properties");
				}
				if (av_image_alloc(
					frame_converted->data, frame_converted->linesize,
					this->width(), this->height(),
					this->pixel_format(), 1) < 0) {
					throw std::runtime_error("Allocating picture");
				}	
				(*format_converter_)(
					frame_decoded.get(), frame_converted.get());

				if (!frame_queue_->push(move(frame_converted))) {
					break;
				}
			}
		}
	} catch (std::exception &e) {
		std::cerr << "Decoding error: " <<  e.what() << std::endl;
		exit(1);
	}
}
