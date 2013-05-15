#pragma once

#include "ofMain.h"

namespace ofxNI2
{
	inline void depthRemapToRange(const ofShortPixels &src, ofPixels &dst, int near, int far, int invert)
	{
		int N = src.getWidth() * src.getHeight();
		dst.allocate(src.getWidth(), src.getHeight(), 1);
		
		const unsigned short *src_ptr = src.getPixels();
		unsigned char *dst_ptr = dst.getPixels();
		
		float inv_range = 1. / (far - near);
		
		if (invert)
			std::swap(near, far);
		
		for (int i = 0; i < N; i++)
		{
			unsigned short C = *src_ptr;
			*dst_ptr = ofMap(C, near, far, 0, 255, true);
			src_ptr++;
			dst_ptr++;
		}
	}
}