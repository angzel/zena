/*
 Copyright (c) 2013 MeherTJ G.
 
 Permission is hereby granted, free of charge, to any person obtaining a copy of
 this software and associated documentation files (the "Software"), to deal in
 the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 the Software, and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "zen_audio_raw.h"
#include "zen_file.h"
#include "zen_exception.h"
#include "zen_endian.h"

namespace Zen {
	struct AudioRawHead
	{
#define Version Byte4('j','a','i','w').value
		
		// donnt change the order of these members.
		uint32_t sign;
		uint16_t channel;
		uint16_t sample_size;
		uint32_t frequency;
		uint32_t sample_count;
		
		AudioRawHead()
		{
		}
		void set(size_t channel, size_t sample_size, size_t frequency, size_t sample_count)
		{
			sign = Version;
			this->channel = HostNet16((uint16_t)channel);
			this->sample_size = HostNet16((uint16_t)sample_size);
			this->frequency = HostNet32((uint32_t)frequency);
			this->sample_count = HostNet32((uint32_t)sample_count);
		}
	};

	std::shared_ptr<Audio> AudioRawDecoder::decode(const std::vector<uint8_t> &data)
	{
//		Zen::BufferReader reader(&data);

		AudioRawHead head;

		musts(data.size() >= sizeof(head), "failed to read header");
		::memcpy(&head, data.data(), sizeof(head));

		auto channel = HostNet16(head.channel);
		auto sample_size = HostNet16(head.sample_size);
		auto frequency = HostNet32(head.frequency);
		auto sample_count = HostNet32(head.sample_count);

		uint32_t size = sample_size * sample_count;

		musts(size > 0, "invalid file format");

		musts(data.size() >= sizeof(head) + size, "invalid file content");

		auto audio = Audio::Create(channel, sample_size, frequency, sample_count);
		must(size == audio->size());
		::memcpy(audio->data(), data.data() + sizeof(head), size);
		return audio;
	}
	std::vector<uint8_t> AudioRawEncoder::encode(const Zen::Audio & audio)
	{
		AudioRawHead head;
		head.set(audio.channel(), audio.sampleSize(), audio.frequency(), audio.sampleCount());
		std::vector<uint8_t> data;
		data.reserve(sizeof(head) + audio.size());

		auto head_buf = reinterpret_cast<char const *>(&head);
		data.assign(head_buf, head_buf + sizeof(head));
		data.insert(data.end(), audio.data(), audio.data() + audio.size());
		return data;
	}
}

