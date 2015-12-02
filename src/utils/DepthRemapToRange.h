#pragma once

#include "ofMain.h"

namespace ofxNI2
{
	inline void depthRemapToRange(const ofShortPixels &src, ofPixels &dst, int _near, int _far, int invert)
	{
		int N = src.getWidth() * src.getHeight();
		dst.allocate(src.getWidth(), src.getHeight(), 1);
		
		const unsigned short *src_ptr = src.getPixels();
		unsigned char *dst_ptr = dst.getPixels();
		
		float inv_range = 1. / (_far - _near);
		
		if (invert)
			std::swap(_near, _far);
		
		for (int i = 0; i < N; i++)
		{
			unsigned short C = *src_ptr;
			*dst_ptr = ofMap(C, _near, _far, 0, 255, true);
			src_ptr++;
			dst_ptr++;
		}
	}
}