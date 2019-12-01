/*
 Copyright (c) 2013 ClearSky G.
 
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

/**
	 Need png library
 */
#include "PNG/png.h"
#include "PNG/pngconf.h"

#include "zen_image_png.h"
#include "zen_file.h"
#include "zen_endian.h"
#include "zen_buffer.h"
#include "zen_exception.h"
#include "zen_log.h"

namespace Zen
{
	namespace MyPNG {
		
		static void OnWarning(png_structp png_ptr, png_const_charp message)
		{
			Zen::LogW("libpng warning:%s", message);
		}
		
		static void OnError(png_structp png_ptr, png_const_charp message)
		{
			throws("libpng operation error");
		}
		
		struct ReadSource
		{
			std::vector<uint8_t> const * src;
			int offset;
		};
		struct WriteSource
		{
			std::vector<uint8_t> * dst;
		};
		struct ReadLib
		{
			png_structp png_ptr;
			png_infop info_ptr;
			
			ReadLib()
			{
				png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
				info_ptr = png_create_info_struct(png_ptr);
				
				musts(png_ptr && info_ptr, "failed to load png lib");
				
				png_set_error_fn(png_ptr, nullptr, &MyPNG::OnError, &MyPNG::OnWarning);
			}
			~ReadLib()
			{
				if (png_ptr)
				{
					png_destroy_read_struct(&png_ptr, (info_ptr ? &info_ptr : nullptr), 0);
				}
			}
		};
		struct WriteLib
		{
			png_structp png_ptr;
			png_infop info_ptr;
			
			WriteLib()
			{
				png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
				info_ptr = png_create_info_struct(png_ptr);
				
				musts(png_ptr && info_ptr, "failed to load png lib");
				
				png_set_error_fn(png_ptr, nullptr, &MyPNG::OnError, &MyPNG::OnWarning);
			}
			~WriteLib()
			{
				if (png_ptr)
				{
					png_destroy_write_struct(&png_ptr, (info_ptr ? &info_ptr : nullptr));
				}
			}
		};
		
		static void ReadCallback(png_structp png_ptr, png_bytep data, png_size_t length)
		{
			MyPNG::ReadSource * p = (MyPNG::ReadSource*)png_get_io_ptr(png_ptr);
			
			if (p->offset + length <= p->src->size())
			{
				memcpy(data, p->src->data() + p->offset, length);
				p->offset += length;
			}
			else
			{
				png_error(png_ptr, "pngReaderCallback failed");
			}
		}
		static void WriteCallback(png_structp png_ptr, png_bytep data, png_size_t length)
		{
			MyPNG::WriteSource * p = (MyPNG::WriteSource*)png_get_io_ptr(png_ptr);
			
			auto src = (const char *)data;
			p->dst->insert(p->dst->end(), src, src + length);
		}
	}
}

namespace Zen
{
	void ImagePNG::setAlpahPremultiplied(bool ap)
	{
		mAlpahPremultiplied = ap;
	}
	bool ImagePNG::isAlpahPremultiplied() const
	{
		return mAlpahPremultiplied;
	}
	void ImagePNG::load(ImageData & img, std::string const & file)
	{
		img.format = Zen::EImageFormat::None;
		
		auto data = Zen::ReadWholeFileToBuffer(file);
		musts(data.size(), "read file error");
		
		this->decode(img, data);
	}
	void ImagePNG::save(ImageData const & img, std::string const & file)
	{
		auto data = encode(img);
		
		std::fstream outs;
		outs.open(file, std::ios::out | std::ios::binary);
		musts(outs.good(), "open file error");
		outs.write((const char*)data.data(), data.size());
		
		musts(outs.good(), "write png file error");
	}

	void ImagePNG::decode(ImageData & img, std::vector<uint8_t> const & data)
	{
		img.format = Zen::EImageFormat::None;

		static int const HeadLen = 8;
		
		musts(data.size() >= HeadLen, "invalid png data");

		// check the data is png or not
		musts(png_sig_cmp((png_const_bytep)data.data(), 0, HeadLen) == 0, "not png file");
		
		MyPNG::ReadLib png;
		
		MyPNG::ReadSource imageSource;
		imageSource.offset = 0;
		imageSource.src = &data;
		
		::png_set_read_fn(png.png_ptr, &imageSource, &MyPNG::ReadCallback);
		
		::png_read_info(png.png_ptr, png.info_ptr);

		if(mAlpahPremultiplied)
		{
			::png_set_alpha_mode(png.png_ptr, PNG_ALPHA_PREMULTIPLIED, 1);
		}

		uint32_t width = ::png_get_image_width(png.png_ptr, png.info_ptr);
		uint32_t height = ::png_get_image_height(png.png_ptr, png.info_ptr);
		int bits = ::png_get_bit_depth(png.png_ptr, png.info_ptr);
		png_uint_32 color_type = ::png_get_color_type(png.png_ptr, png.info_ptr);
		
		// force palette images to be expanded to 24-bit RGB
		if (color_type == PNG_COLOR_TYPE_PALETTE)
		{
			::png_set_palette_to_rgb(png.png_ptr);
		}
		// low-bit-depth grayscale images are to be expanded to 8 bits
		if (color_type == PNG_COLOR_TYPE_GRAY && bits < 8)
		{
			::png_set_expand_gray_1_2_4_to_8(png.png_ptr);
		}
		// expand any tRNS chunk data into a full alpha channel
		if (png_get_valid(png.png_ptr, png.info_ptr, PNG_INFO_tRNS))
		{
			::png_set_tRNS_to_alpha(png.png_ptr);
		}
		// reduce images with 16-bit samples to 8 bits
		if (bits == 16)
		{
			::png_set_strip_16(png.png_ptr);
		}
		// expand grayscale images to RGB
		if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		{
			::png_set_gray_to_rgb(png.png_ptr);
		}
		
		
		std::vector<png_bytep> row_pointers(height, 0);
		
		::png_read_update_info(png.png_ptr, png.info_ptr);
		
		size_t rowbytes = png_get_rowbytes(png.png_ptr, png.info_ptr);
		
		size_t channel = rowbytes / width;
		
		musts(rowbytes % width == 0, "unsupported row stride");
		
		Zen::EImageFormat format = (channel == 4 ? Zen::EImageFormat::RGBA :
							(channel == 3 ? Zen::EImageFormat::RGB : Zen::EImageFormat::None));
		
		musts(format != Zen::EImageFormat::None, "unsupported pixel format");
		
		std::vector<uint8_t> buffer;
		buffer.resize(rowbytes * height);

		for (unsigned short i = 0; i < height; ++i)
		{
			row_pointers[i] = (unsigned char *)buffer.data() + i * rowbytes;
		}
		
		::png_read_image(png.png_ptr, row_pointers.data());
		
		::png_read_end(png.png_ptr, nullptr);
		
		img.width = width;
		img.height = height;
		img.format = format;
		img.buffer = std::move(buffer);
	}
	std::vector<uint8_t> ImagePNG::encode(ImageData const & img)
	{
		size_t bpp = GetBytesOfImageFormat(img.format);
		musts(bpp, "invalid pixel format");
		
		size_t rowbytes = bpp * img.width;
		
		std::vector<png_bytep> row_pointers(img.height);
		for (size_t i = 0; i < img.height; i++)
		{
			row_pointers[i] = (png_bytep)(img.buffer.data() + rowbytes * i);
		}
		
		int color_type = 0;
		switch (img.format)
		{
			case Zen::EImageFormat::Grey:
				color_type = PNG_COLOR_TYPE_GRAY;
				break;
			case Zen::EImageFormat::RGB:
				color_type = PNG_COLOR_TYPE_RGB;
				break;
			case Zen::EImageFormat::RGBA:
				color_type = PNG_COLOR_TYPE_RGBA;
				break;
			default:
				throws("invalid format");
		}
		
		MyPNG::WriteLib png;
		
		MyPNG::WriteSource source;
		std::vector<uint8_t> data;
		source.dst = &data;
		
		::png_set_write_fn(png.png_ptr, &source, &MyPNG::WriteCallback, nullptr);

//		if(mAlpahPremultiplied)
//		{
//			::png_set_alpha_mode(png.png_ptr, PNG_ALPHA_PREMULTIPLIED, 1);
//		}
		
		::png_set_IHDR(png.png_ptr, png.info_ptr, img.width, img.height, 8, (int)color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		::png_write_info(png.png_ptr, png.info_ptr);
		
		::png_write_image(png.png_ptr, row_pointers.data());
		::png_write_end(png.png_ptr, nullptr);
		
		return data;
	}
}

