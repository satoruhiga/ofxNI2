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
	
	void update();
	
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
	
	bool setup(ofxNI2::Device &device)
	{
		return Stream::setup(device, openni::SENSOR_DEPTH);
	}
	
	void updateTextureIfNeeded();
	
	const ofShortPixels& getPixelsRef() const { return pix; }

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
	
	ofShortPixels pix;
	
	void setPixels(openni::VideoFrameRef frame);

};
