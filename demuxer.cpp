#include "demuxer.h"
#include "ffmpeg.h"

Demuxer::Demuxer(const std::string &file_name) {
	av_register_all();
	avformat_network_init();
	ffmpeg::check(
		avformat_open_input(&avcontainer, file_name.c_str(), nullptr, nullptr)
	);
	ffmpeg::check(avformat_find_stream_info(avcontainer, nullptr));
	// Log file metadata info
    av_dump_format(avcontainer, 0, file_name.c_str(), 0);

	video_stream_index = ffmpeg::check(av_find_best_stream(
		avcontainer, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0));
	audio_stream_index = ffmpeg::check(av_find_best_stream(
		avcontainer, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0));
}

Demuxer::~Demuxer() {
	avformat_close_input(&avcontainer);
}

AVCodecParameters* Demuxer::video_codec() {
	return avcontainer->streams[video_stream_index]->codecpar;
}

AVCodecParameters* Demuxer::audio_codec() {
	return avcontainer->streams[audio_stream_index]->codecpar;
}

int Demuxer::get_video_stream_index() const {
	return video_stream_index;
}

int Demuxer::get_audio_stream_index() const {
	return audio_stream_index;
}

AVRational Demuxer::time_base() const {
	return avcontainer->streams[video_stream_index]->time_base;
}

bool Demuxer::operator()(AVPacket &packet) {
	return av_read_frame(avcontainer, &packet) >= 0;
}
