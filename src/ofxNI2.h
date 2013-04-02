#pragma once

#include "ofMain.h"

#include "OpenNI.h"

namespace ofxNI2
{
	class Device;
	class Stream;

	template <typename PixelType>
	struct DoubleBuffer;
	
	class IrStream;
	class ColorStream;
	class DepthStream;
};

class ofxNI2::Device
{
	friend class ofxNI2::Stream;
	
public:
	
	void setup(const char* uri = openni::ANY_DEVICE);
	void exit();
	
	void update();
	
	operator const openni::Device&() const { return device; }
	
protected:
	
	openni::Device device;
	vector<ofxNI2::Stream*> streams;
};

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


class ofxNI2::Stream : public openni::VideoStream::NewFrameListener
{
	friend class ofxNI2::Device;
	
public:
	
	virtual ~Stream();
	
	void exit();
	
	void start();
	
	int getWidth() const;
	int getHeight() const;
	
	bool setSize(int width, int height);
	bool setWidth(int v);
	bool setHeight(int v);
	ofTexture& getTextureReference() { return tex; }
	
	int getFps();
	bool setFps(int v);
	
	void setMirror(bool v = true);
	bool getMirror();
	
	inline bool isFrameNew() const { return is_frame_new; }

	void draw(float x = 0, float y = 0);
	void draw(float x, float y, float w, float h);

	virtual void updateTextureIfNeeded();
	
protected:

	openni::VideoStream stream;
	uint64_t openni_timestamp;
	bool is_frame_new, texture_needs_update;
	
	ofTexture tex;
	
	Stream();

	bool setup(ofxNI2::Device &device, openni::SensorType sensor_type);
	virtual void setPixels(openni::VideoFrameRef frame);
	
	void onNewFrame(openni::VideoStream&);
};

class ofxNI2::IrStream : public ofxNI2::Stream
{
public:
	
	bool setup(ofxNI2::Device &device)
	{
		return Stream::setup(device, openni::SENSOR_IR);
	}
	
	void updateTextureIfNeeded();
	
	const ofPixels& getPixelsRef() const { return pix.getFrontBuffer(); }
	
protected:

	DoubleBuffer<ofPixels> pix;
	void setPixels(openni::VideoFrameRef frame);

};

class ofxNI2::ColorStream : public ofxNI2::Stream
{
public:
	
	bool setup(ofxNI2::Device &device)
	{
		return Stream::setup(device, openni::SENSOR_COLOR);
	}
	
	void updateTextureIfNeeded();
	
	const ofPixels& getPixelsRef() const { return pix.getFrontBuffer(); }
	
protected:
	
	DoubleBuffer<ofPixels> pix;
	void setPixels(openni::VideoFrameRef frame);
	
};


class ofxNI2::DepthStream : public ofxNI2::Stream
{
public:
	
	bool setup(ofxNI2::Device &device)
	{
		return Stream::setup(device, openni::SENSOR_DEPTH);
	}
	
	void updateTextureIfNeeded();
	
	const ofShortPixels& getPixelsRef() const { return pix.getFrontBuffer(); }

	// shader
	
	class DepthShader : public ofShader
	{
	public:
		
		void setup();
		
	protected:
		
		virtual string getShaderCode() const = 0;
	};
	
	class Grayscale : public DepthShader
	{
	public:
		
		Grayscale() : min_value(50), max_value(10000) {}
		
		void begin();
		
	protected:
		
		float min_value, max_value;
		string getShaderCode() const;
	};

protected:
	
	DoubleBuffer<ofShortPixels> pix;
	void setPixels(openni::VideoFrameRef frame);
};
