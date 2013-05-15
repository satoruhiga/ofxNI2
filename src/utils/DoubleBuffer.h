#pragma once

#include "ofMain.h"

namespace ofxNI2
{
	template <typename PixelType>
	struct DoubleBuffer;
}

template <typename PixelType>
struct ofxNI2::DoubleBuffer
{
public:
	
	DoubleBuffer() : front_buffer_index(0), back_buffer_index(1), allocated(false) {}
	
	void allocate(int w, int h, int channels)
	{
		if (allocated) return;
		allocated = true;
		
		pix[0].allocate(w, h, channels);
		pix[1].allocate(w, h, channels);
	}
	
	PixelType& getFrontBuffer() { return pix[front_buffer_index]; }
	const PixelType& getFrontBuffer() const { return pix[front_buffer_index]; }
	
	PixelType& getBackBuffer() { return pix[back_buffer_index]; }
	const PixelType& getBackBuffer() const { return pix[back_buffer_index]; }
	
	void swap()
	{
		std::swap(front_buffer_index, back_buffer_index);
	}
	
private:
	
	PixelType pix[2];
	int front_buffer_index, back_buffer_index;
	bool allocated;
};
