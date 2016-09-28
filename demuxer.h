#pragma once
#include <string>
extern "C" {
	#include "libavformat/avformat.h"
}

class Demuxer {
public:
	Demuxer(const std::string &file_name);
	~Demuxer();
	AVCodecContext* video_codec();
	AVCodecContext* audio_codec();
	int get_video_stream_index() const;
	int get_audio_stream_index() const;
	AVRational time_base() const;
	bool operator()(AVPacket &packet);

private:
	AVFormatContext* avcontainer{};
	int video_stream_index{};
	int audio_stream_index{};
};
