#include "audio_decoder.h"
#include "ffmpeg.h"

AudioDecoder::AudioDecoder(AVCodecContext* codec_context) :
	codec_context_{codec_context} {
	const auto codec_audio =
		avcodec_find_decoder(codec_context_->codec_id);
	if (!codec_audio) {
		throw ffmpeg::Error{"Unsupported audio codec"};
	}
	ffmpeg::check(avcodec_open2(
		codec_context_, codec_audio, nullptr));
}

AudioDecoder::~AudioDecoder() {
	avcodec_close(codec_context_);
}

void AudioDecoder::operator()(
	AVFrame* frame, int &finished, AVPacket* packet) {
	ffmpeg::check(avcodec_decode_audio4(
		codec_context_, frame, &finished, packet));
}

AVRational AudioDecoder::time_base() const {
	return codec_context_->time_base;
}
