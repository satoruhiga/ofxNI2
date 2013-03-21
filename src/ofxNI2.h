#pragma once

#include "ofMain.h"

#include "OpenNI.h"

namespace ofxNI2
{
	class Device;
	class Stream;

	class IrStream;
	class ColorStream;
	class DepthStream;
};

class ofxNI2::Device : public ofThread
{
	friend class ofxNI2::Stream;
	
public:
	
	void setup(const char* uri = openni::ANY_DEVICE);
	void exit();
	
	void start();
	
	operator const openni::Device&() const { return device; }
	
protected:
	
	openni::Device device;
	vector<ofxNI2::Stream*> streams;
	
	void threadedFunction();
};

class ofxNI2::Stream
{
	friend class ofxNI2::Device;
	
public:
	
	virtual ~Stream();
	
	void exit();
	
	void start();
	
	void update();
	
	int getWidth() const;
	int getHeight() const;
	
	bool setSize(int width, int height);
	bool setWidth(int v);
	bool setHeight(int v);
	
	int getFps();
	bool setFps(int v);
	
	bool isFrameNew() const { return is_new_frame; }

	void draw(float x = 0, float y = 0);
	void draw(float x, float y, float w, float h);

	virtual void updateTextureIfNeeded();
	
protected:

	openni::VideoStream stream;
	uint64_t openni_timestamp, opengl_timestamp;
	bool is_new_frame;
	
	ofTexture tex;
	
	Stream();

	bool setup(ofxNI2::Device &device, openni::SensorType sensor_type = openni::SENSOR_DEPTH);
	
	void copyFrame();
	
	virtual void setPixels(openni::VideoFrameRef frame);
};

class ofxNI2::IrStream : public ofxNI2::Stream
{
public:
	
	bool setup(ofxNI2::Device &device)
	{
		return Stream::setup(device, openni::SENSOR_IR);
	}
	
	void updateTextureIfNeeded();
	
protected:

	ofPixels pix;
	void setPixels(openni::VideoFrameRef frame);

};

class ofxNI2::DepthStream : public ofxNI2::Stream
{
public:
	
	enum ColorMode
	{
		RAW,
		GRAYSCALE,
		COLOR
	};
	
	bool setup(ofxNI2::Device &device)
	{
		color_mode = COLOR;
		return Stream::setup(device, openni::SENSOR_DEPTH);
	}
	
	void updateTextureIfNeeded();
	
protected:
	
	ColorMode color_mode;
	
	ofShortPixels pix;
	void setPixels(openni::VideoFrameRef frame);

};
