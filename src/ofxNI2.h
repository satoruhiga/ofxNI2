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
	
	void onUpdate(ofEventArgs &e);
};

class ofxNI2::Stream
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
	
	int getFps();
	bool setFps(int v);
	
	inline bool isFrameNew() const { return is_frame_new; }

	void draw(float x = 0, float y = 0);
	void draw(float x, float y, float w, float h);

	virtual void updateTextureIfNeeded();
	
protected:

	openni::VideoStream stream;
	uint64_t openni_timestamp;
	bool is_frame_new;
	
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
	
	const ofPixels& getPixelsRef() const { return pix; }
	
protected:

	ofPixels pix;
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
	
	const ofPixels& getPixelsRef() const { return pix; }
	
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
		near_clip = 500;
		far_clip = 4000;
		
		color_mode = GRAYSCALE;
		return Stream::setup(device, openni::SENSOR_DEPTH);
	}
	
	void updateTextureIfNeeded();
	
	const ofPixels& getPixelsRef() const { return pix; }
	const ofShortPixels& getRawPixelsRef() const { return raw; }
	
	void setDepthClipping(float nearClip=500, float farClip=4000)
	{
		near_clip = nearClip;
		far_clip = farClip;
	}
	
protected:
	
	ColorMode color_mode;
	
	ofPixels pix;
	ofShortPixels raw;
	
	float near_clip, far_clip;
	
	void setPixels(openni::VideoFrameRef frame);

};
