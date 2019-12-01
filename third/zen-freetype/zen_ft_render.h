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

#pragma once

#include "zen_ft_library.h"
#include "zen_image.h"
#include <string>

namespace Zen {
	struct FontRenderLineInfo
	{
		int points = 0;
		std::vector<std::shared_ptr<FontCharacter> > chars;
	};
	
	class FontRender
	{
	public:
		static std::shared_ptr<FontRender> Create
		(std::shared_ptr<Font>,
		 FontConfig const & config,
		 std::u32string const & text,
		 float alignment, /* 0.0 left(top), to 1.0 right(bottom) */
		 int letter_spacing = 0, // px as font-size
		 int max_line_size = 0,
		 bool auto_break = false);

		virtual std::shared_ptr<FontCharacter> getCharacter(char32_t unicode) = 0;

		/*
		 image: dest image, will be modified
		 image_width: final image width, 0 for auto set
		 image_height: final image height, 0 for auto set
		 */
		virtual void renderToImage
		(ImageData & image,
		 int image_width,
		 int image_height,
		 int x_off, int y_off) = 0;

		virtual int getOutputWidth() = 0;

		virtual int getOutputHeight() = 0;

		virtual size_t getLinesCount() = 0;
	};
}
