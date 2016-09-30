#include "audio_decoder.h"
#include "ffmpeg.h"
#include <limits>

#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000

AudioDecoder::AudioDecoder(AVCodecParameters* codec_param)
{
	const auto codec_ =
		avcodec_find_decoder(codec_param->codec_id);
	if (!codec_) {
		throw ffmpeg::Error{"Not found audio decoder"};
	}
	codec_context_ = avcodec_alloc_context3(codec_);
	avcodec_parameters_to_context(codec_context_, codec_param);
	ffmpeg::check(avcodec_open2(
		codec_context_, codec_, nullptr)
	);

	ao_initialize();
	
    int driver = ao_default_driver_id();
    ao_sample_format sformat;
    
    sfmt = (AVSampleFormat)codec_param->format;

    if(sfmt == AV_SAMPLE_FMT_U8 || sfmt == AV_SAMPLE_FMT_U8P) {
    	sformat.bits	= 8;
    } else if (sfmt == AV_SAMPLE_FMT_S16 || sfmt == AV_SAMPLE_FMT_S16P) {
    	sformat.bits	= 16;
    } else if (sfmt == AV_SAMPLE_FMT_S32 || sfmt == AV_SAMPLE_FMT_S32P) {
    	sformat.bits	= 32;
    } else if (sfmt == AV_SAMPLE_FMT_FLT || sfmt == AV_SAMPLE_FMT_FLTP) {
    	sformat.bits	= 16;
    } else if (sfmt == AV_SAMPLE_FMT_DBL || sfmt == AV_SAMPLE_FMT_DBLP) {
        sformat.bits	= 16;
    } else {
    	throw std::runtime_error("Unsupported format");
    }

	frame				= av_frame_alloc();

    sformat.channels	= codec_context_->channels;
    sformat.rate 		= codec_context_->sample_rate;
    sformat.byte_format	= AO_FMT_NATIVE;
    sformat.matrix		= 0;

    adevice				= ao_open_live(driver, &sformat, NULL);
    buffer_size 		= AVCODEC_MAX_AUDIO_FRAME_SIZE + AV_INPUT_BUFFER_PADDING_SIZE;
    buffer				= new uint8_t[buffer_size];
    samples				= new uint8_t[buffer_size];
    out					= (uint16_t *)samples;
}

int AudioDecoder::get_channels()
{
	return codec_context_->channels;
}

AudioDecoder::~AudioDecoder()
{
    ao_shutdown();
	avcodec_free_context(&codec_context_);
    delete buffer;
    delete samples;
}

void AudioDecoder::operator()(int &finished, AVPacket* packet)
{
	finished = decode(frame, packet);
}

bool AudioDecoder::decode(AVFrame* frame, AVPacket* packet)
{
	int send_packet = avcodec_send_packet(codec_context_, packet);
	int receive_frame = avcodec_receive_frame(codec_context_, frame);
	
	return (send_packet == 0) && (receive_frame == 0);
}

void AudioDecoder::play(int plane_size)
{
    int write_p = 0;
    switch (sfmt) {
        case AV_SAMPLE_FMT_S16P:
            for (int nb=0; nb < plane_size/sizeof(uint16_t); nb++){
                for (int ch = 0; ch < get_channels(); ch++) {
                    out[write_p] = ((uint16_t *) frame->extended_data[ch])[nb];
                    write_p++;
                }
            }
            ao_play(
            	adevice,
            	(char*)samples,
            	(plane_size) * get_channels()
            );
            break;
        case AV_SAMPLE_FMT_FLTP:
            for (int nb=0; nb < plane_size/sizeof(float); nb++) {
                for (int ch = 0; ch < get_channels(); ch++) {
                    out[write_p] = ((float *)frame->extended_data[ch])[nb] * std::numeric_limits<short>::max();
                    write_p++;
                }
            }
            ao_play(
            	adevice,
            	(char*)samples,
            	(plane_size/sizeof(float))  * sizeof(uint16_t) * get_channels()
            );
            break;
        case AV_SAMPLE_FMT_S16:
            ao_play(
            	adevice,
            	(char*)frame->extended_data[0],
            	frame->linesize[0]
            );
            break;
        case AV_SAMPLE_FMT_FLT:
            for (int nb=0; nb < plane_size/sizeof(float); nb++) {
                out[nb] = static_cast<short>(
                			((float *)frame->extended_data[0])[nb] * std::numeric_limits<short>::max()
                		);
            }
            ao_play(
            	adevice,
            	(char*)samples,
            	(plane_size/sizeof(float))  * sizeof(uint16_t)
            );
            break;
        case AV_SAMPLE_FMT_U8P:
            for (int nb=0; nb < plane_size/sizeof(uint8_t); nb++) {
                for (int ch = 0; ch < get_channels(); ch++) {
                    out[write_p] = (((uint8_t *) frame->extended_data[0])[nb] - 127) *
                    				std::numeric_limits<short>::max() / 127;
                    write_p++;
                }
            }
            ao_play(
            	adevice,
            	(char*)samples,
            	(plane_size/sizeof(uint8_t)) * sizeof(uint16_t) * get_channels()
            );
            break;
        case AV_SAMPLE_FMT_U8:
            for (int nb=0; nb < plane_size/sizeof(uint8_t); nb++) {
                out[nb] = static_cast<short>(
                			(((uint8_t *)frame->extended_data[0])[nb] - 127) *
                			std::numeric_limits<short>::max() / 127
                		);
            }
            ao_play(
            	adevice,
            	(char*)samples,
            	(plane_size/sizeof(uint8_t) ) * sizeof(uint16_t)
            );
            break;
        default:
            throw ffmpeg::Error("PCM type not supported");
    }
}