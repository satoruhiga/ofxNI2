#pragma once

#include "ofMain.h"

#include "OpenNI.h"
#include <assert.h>

#include "DoubleBuffer.h"

namespace ofxNI2
{
	void init();
	
	class Device;
	class Stream;
	
	class IrStream;
	class ColorStream;
	class DepthStream;
};

class ofxNI2::Device
{
	friend class ofxNI2::Stream;
	
public:
	
	Device() : recorder(NULL) {}
	
	void setup(const char* uri = openni::ANY_DEVICE);
	void exit();
	
	void update();
	
	bool isRegistrationSupported() const;
	void setEnableRegistration();
	bool getEnableRegistration() const;
	
	bool startRecord(string filename = "", bool allowLossyCompression = false);
	void stopRecord();
	bool isRecording() const { return recorder != NULL; }
	
	operator openni::Device&() { return device; }
	operator const openni::Device&() const { return device; }

	openni::Device& get() { return device; }
	const openni::Device& get() const { return device; }

protected:
	
	openni::Device device;
	vector<ofxNI2::Stream*> streams;
	
	openni::Recorder *recorder;
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
	
	inline float getHorizontalFieldOfView() const { return ofRadToDeg(stream.getHorizontalFieldOfView()); }
	inline float getVerticalFieldOfView() const { return ofRadToDeg(stream.getVerticalFieldOfView()); }

	inline bool isFrameNew() const { return is_frame_new; }

	void draw(float x = 0, float y = 0);
	void draw(float x, float y, float w, float h);

	virtual void updateTextureIfNeeded();
	
	operator openni::VideoStream& () { return stream; }
	operator const openni::VideoStream& () const { return stream; }

	openni::VideoStream& get() { return stream; }
	const openni::VideoStream& get() const { return stream; }

protected:

	openni::VideoStream stream;
	uint64_t openni_timestamp, opengl_timestamp;
	bool is_frame_new, texture_needs_update;
	
	ofTexture tex;
	Device *device;
	
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
	
	ofPixels& getPixelsRef() { return pix.getFrontBuffer(); }
	
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
	
	ofPixels& getPixelsRef() { return pix.getFrontBuffer(); }
	
protected:
	
	DoubleBuffer<ofPixels> pix;
	void setPixels(openni::VideoFrameRef frame);
	
};

class ofxNI2::DepthStream : public ofxNI2::Stream
{
public:

	class DepthShader;
	class Grayscale;

	bool setup(ofxNI2::Device &device)
	{
		return Stream::setup(device, openni::SENSOR_DEPTH);
	}
	
	void updateTextureIfNeeded();
	
	ofShortPixels& getPixelsRef() { return pix.getFrontBuffer(); }
	ofPixels getPixelsRef(int near, int far, bool invert = false);
	
	ofVec3f getWorldCoordinateAt(int x, int y);

protected:
	
	DoubleBuffer<ofShortPixels> pix;
	void setPixels(openni::VideoFrameRef frame);
};

// shader
class ofxNI2::DepthStream::DepthShader : public ofShader
{
public:
	
	void setup();
	
protected:
	
	virtual string getShaderCode() const = 0;
};

class ofxNI2::DepthStream::Grayscale : public DepthShader
{
public:
	
	Grayscale() : min_value(50), max_value(10000) {}
	
	void begin();
	
protected:
	
	float min_value, max_value;
	string getShaderCode() const;
};
