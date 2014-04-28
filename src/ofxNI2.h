#pragma once

#include "ofMain.h"

#include "OpenNI.h"
#include <assert.h>

#include "utils/DoubleBuffer.h"

namespace ofxNI2
{
	void init();
	
	class Device;
	class Stream;
	
	class IrStream;
	class ColorStream;
	class DepthStream;
	
	class DepthShader;
	class Grayscale;
};

// device

class ofxNI2::Device
{
	friend class ofxNI2::Stream;
	
public:
	
	ofEvent<ofEventArgs> updateDevice;
	
	Device();
	~Device();
	
	static int listDevices();
	
	bool setup();
	bool setup(int device_id);
	bool setup(string oni_file_path);
	
	void exit();
	
	void update();
	
	bool isRegistrationSupported() const;
	void setEnableRegistration();
	bool getEnableRegistration() const;
	
	bool startRecord(string filename = "", bool allowLossyCompression = false);
	void stopRecord();
	bool isRecording() const { return recorder != NULL; }
	
	void setDepthColorSyncEnabled(bool b = true) { device.setDepthColorSyncEnabled(b); }
	
	operator openni::Device&() { return device; }
	operator const openni::Device&() const { return device; }

	openni::Device& get() { return device; }
	const openni::Device& get() const { return device; }

protected:
	
	openni::Device device;
	vector<ofxNI2::Stream*> streams;
	
	openni::Recorder *recorder;
};

// stream

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
	ofTexture& getTextureReference() {
		
		if (texture_needs_update)
			updateTextureIfNeeded();
			
		return tex;
	}
	
	int getFps();
	bool setFps(int v);
	
	void setMirror(bool v = true);
	bool getMirror();
	
	inline float getHorizontalFieldOfView() const { return ofRadToDeg(stream.getHorizontalFieldOfView()); }
	inline float getVerticalFieldOfView() const { return ofRadToDeg(stream.getVerticalFieldOfView()); }

	inline bool isFrameNew() const { return is_frame_new; }

	void draw(float x = 0, float y = 0);
	virtual void draw(float x, float y, float w, float h);

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
	
	void setAutoExposureEnabled(bool yn = true) { stream.getCameraSettings()->setAutoExposureEnabled(yn); }
	bool getAutoExposureEnabled() { return stream.getCameraSettings()->getAutoExposureEnabled(); }

	void setAutoWhiteBalanceEnabled(bool yn = true) { stream.getCameraSettings()->setAutoWhiteBalanceEnabled(yn); }
	bool getAutoWhiteBalanceEnabled() { return stream.getCameraSettings()->getAutoWhiteBalanceEnabled(); }

protected:
	
	DoubleBuffer<ofPixels> pix;
	void setPixels(openni::VideoFrameRef frame);
	
};

class ofxNI2::DepthStream : public ofxNI2::Stream
{
public:

	bool setup(ofxNI2::Device &device);
	
	void updateTextureIfNeeded();
	
	ofShortPixels& getPixelsRef() { return pix.getFrontBuffer(); }
	ofPixels getPixelsRef(int near, int far, bool invert = false);
	
	ofVec3f getWorldCoordinateAt(int x, int y);
	
	inline void draw(float x = 0, float y = 0) { ofxNI2::Stream::draw(x, y); }
	void draw(float x, float y, float w, float h);
	
	template <typename T>
	inline ofPtr<DepthShader> setupShader();
	
	template <typename T>
	ofPtr<T> getShader() const { return dynamic_pointer_cast<T>(shader); }

protected:
	
	DoubleBuffer<ofShortPixels> pix;
	void setPixels(openni::VideoFrameRef frame);
	
	ofPtr<DepthShader> shader;
};

// depth shader

class ofxNI2::DepthShader : public ofShader
{
public:
	
	virtual void setup(DepthStream &depth);
	virtual void begin() { ofShader::begin(); }
	virtual void end() { ofShader::end(); }
	
protected:
	
	virtual string getShaderCode() const = 0;
};

template <typename T>
inline ofPtr<ofxNI2::DepthShader> ofxNI2::DepthStream::setupShader()
{
	shader = ofPtr<ofxNI2::DepthShader>(new T);
	shader->setup(*this);
	return shader;
}

class ofxNI2::Grayscale : public DepthShader
{
public:
	
	Grayscale() : near_value(50), far_value(10000) {}
	
	void begin();
	
	inline void setNear(float near) { near_value = near; }
	inline float getNear() const { return near_value; }
	
	inline void setFar(float far) { far_value = far; }
	inline float getFar() const { return far_value; }
	
protected:
	
	float near_value, far_value;
	string getShaderCode() const;
};

