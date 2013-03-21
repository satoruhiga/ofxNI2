#include "ofxNI2.h"

namespace ofxNI2
{
	void assert_error(openni::Status rc)
	{
		if (rc == openni::STATUS_OK) return;
		ofLogError("ofxNI2") << openni::OpenNI::getExtendedError();
		throw;
	}

	void check_error(openni::Status rc)
	{
		if (rc == openni::STATUS_OK) return;
		ofLogError("ofxNI2") << openni::OpenNI::getExtendedError();
	}

	void init()
	{
		static bool inited = false;
		if (inited) return;
		inited = true;
		
		setenv("OPENNI2_DRIVERS_PATH", "data/OpenNI2/lib/Drivers", 1);
		assert_error(openni::OpenNI::initialize());
	}
}

using namespace ofxNI2;

// Device

void Device::setup(const char* uri)
{
	ofxNI2::init();
	
	assert_error(device.open(uri));
	
	device.setDepthColorSyncEnabled(true);
}

void Device::exit()
{
	waitForThread();
	device.close();
}

void Device::start()
{
	startThread();
}

void Device::threadedFunction()
{
	while (isThreadRunning())
	{
		lock();
		
		vector<openni::VideoStream*> ni_streams;
		openni::Status rc;
		
		for (int i = 0; i < this->streams.size(); i++)
		{
			ni_streams.push_back(&streams[i]->stream);
		}
		
		int index;
		rc = openni::OpenNI::waitForAnyStream(&ni_streams[0], ni_streams.size(), &index);
		
		if (rc == openni::STATUS_OK && index >= 0)
		{
			openni::VideoFrameRef frame;
			streams[index]->copyFrame();
		}
		
		unlock();
	}
}

// Stream

Stream::Stream() {}
Stream::~Stream() {}

bool Stream::setup(ofxNI2::Device &device, openni::SensorType sensor_type)
{
	openni_timestamp = 0;
	opengl_timestamp = 0;
	
	assert_error(stream.create(device, sensor_type));
	assert(stream.isValid());
	
	device.lock();
	device.streams.push_back(this);
	device.unlock();
	
	return true;
}

void Stream::start()
{
	assert_error(stream.start());
	assert(stream.isValid());
}

void Stream::exit()
{
	stream.stop();
	stream.destroy();
}

bool Stream::setSize(int width, int height)
{
	openni::VideoMode m = stream.getVideoMode();
	m.setResolution(width, height);
	openni::Status rc = stream.setVideoMode(m);
	
	if (rc == openni::STATUS_OK)
	{
		return true;
	}
	
	check_error(rc);
	return false;
}

int Stream::getWidth() const
{
	return stream.getVideoMode().getResolutionX();
}

int Stream::getHeight() const
{
	return stream.getVideoMode().getResolutionY();
}

bool Stream::setWidth(int v)
{
	return setSize(v, getHeight());
}

bool Stream::setHeight(int v)
{
	return setSize(getWidth(), v);
}

int Stream::getFps()
{
	return stream.getVideoMode().getFps();
}

bool Stream::setFps(int v)
{
	openni::VideoMode m = stream.getVideoMode();
	m.setFps(v);
	openni::Status rc = stream.setVideoMode(m);
	
	if (rc == openni::STATUS_OK)
	{
		return true;
	}
	
	check_error(rc);
	return false;
}

void Stream::copyFrame()
{
	openni::Status rc;
	openni::VideoFrameRef frame;
	
	rc = stream.readFrame(&frame);
	assert_error(rc);
	
	assert(frame.getCroppingEnabled() == false);
	
	setPixels(frame);
}

void Stream::setPixels(openni::VideoFrameRef frame)
{
	openni_timestamp = frame.getTimestamp();
}

void Stream::update()
{
	is_new_frame = opengl_timestamp != openni_timestamp;
}

void Stream::updateTextureIfNeeded()
{
	opengl_timestamp = openni_timestamp;
}

void Stream::draw(float x, float y)
{
	draw(x, y, getWidth(), getHeight());
}

void Stream::draw(float x, float y, float w, float h)
{
	if (is_new_frame)
		updateTextureIfNeeded();

	if (tex.isAllocated())
		tex.draw(x, y, w, h);
}


// IrStream

void IrStream::setPixels(openni::VideoFrameRef frame)
{
	Stream::setPixels(frame);
	
	openni::VideoMode m = frame.getVideoMode();
	
	int w = m.getResolutionX();
	int h = m.getResolutionY();
	int num_pixels = w * h;
	
	pix.allocate(w, h, 1);

	if (m.getPixelFormat() == openni::PIXEL_FORMAT_GRAY8)
	{
		const unsigned char *src = (const unsigned char*)frame.getData();
		unsigned char *dst = pix.getPixels();

		for (int i = 0; i < num_pixels; i++)
		{
			dst[0] = src[0];
			src++;;
			dst++;
		}
	}
	else if (m.getPixelFormat() == openni::PIXEL_FORMAT_GRAY16)
	{
		const unsigned short *src = (const unsigned short*)frame.getData();
		unsigned char *dst = pix.getPixels();

		for (int i = 0; i < num_pixels; i++)
		{
			dst[0] = src[0] >> 2;
			src++;;
			dst++;
		}
	}
}

void IrStream::updateTextureIfNeeded()
{
	if (!tex.isAllocated()
		|| tex.getWidth() != getWidth()
		|| tex.getHeight() != getHeight())
	{
		tex.allocate(getWidth(), getHeight(), GL_LUMINANCE);
	}
	
	tex.loadData(pix);
	
	Stream::updateTextureIfNeeded();
}

// DepthStream

void DepthStream::setPixels(openni::VideoFrameRef frame)
{
	Stream::setPixels(frame);
	
	const unsigned short *pixels = (const unsigned short*)frame.getData();
	int w = frame.getVideoMode().getResolutionX();
	int h = frame.getVideoMode().getResolutionY();
	pix.setFromPixels(pixels, w, h, OF_IMAGE_GRAYSCALE);
}

void DepthStream::updateTextureIfNeeded()
{
	if (color_mode == RAW)
	{
		if (!tex.isAllocated()
			|| tex.getWidth() != getWidth()
			|| tex.getHeight() != getHeight()
			|| tex.getTextureData().pixelType != GL_SHORT)
		{
			static ofTextureData data;
			
			data.pixelType = GL_SHORT;
			data.glType = GL_LUMINANCE;
			data.width = getWidth();
			data.height = getHeight();
			
			tex.allocate(data);
		}

		tex.loadData(pix);
	}
	else if (color_mode == GRAYSCALE)
	{
		if (!tex.isAllocated()
			|| tex.getWidth() != getWidth()
			|| tex.getHeight() != getHeight())
		{
			tex.allocate(getWidth(), getHeight(), GL_RGB);
		}

		ofPixels temp;
		temp.allocate(getWidth(), getHeight(), 3);
		
		openni::VideoMode m = stream.getVideoMode();
		
		int w = m.getResolutionX();
		int h = m.getResolutionY();
		int num_pixels = w * h;
		
		int min = stream.getMinPixelValue();
		int max = stream.getMaxPixelValue();
		float d = 1. / (max - min);
		
		const unsigned short *src = (const unsigned short*)pix.getPixels();
		unsigned char *dst = temp.getPixels();
		
		for (int i = 0; i < num_pixels; i++)
		{
			int p = (1 - (float)(*src - min) * d) * 255;
			p = ofClamp(p, 0, 255);
			dst[0] = p;
			dst[1] = p;
			dst[2] = p;
			dst += 3;
			src++;
		}
		
		tex.loadData(temp);
	}
	else if (color_mode == COLOR)
	{
		if (!tex.isAllocated()
			|| tex.getWidth() != getWidth()
			|| tex.getHeight() != getHeight())
		{
			tex.allocate(getWidth(), getHeight(), GL_RGB);
		}

		ofPixels temp;
		temp.allocate(getWidth(), getHeight(), 3);
		
		openni::VideoMode m = stream.getVideoMode();
		
		int w = m.getResolutionX();
		int h = m.getResolutionY();
		int num_pixels = w * h;
		
		int min = stream.getMinPixelValue();
		int max = stream.getMaxPixelValue();
		float d = 1. / (max - min);
		
		const unsigned short *src = (const unsigned short*)pix.getPixels();
		unsigned char *dst = temp.getPixels();
		
		for (int i = 0; i < num_pixels; i++)
		{
			int p = ((float)(*src - min) * d) * 255;
			p = ofClamp(p, 0, 255);
			ofColor c = ofColor::fromHsb(p, 255, 255);
			dst[0] = c.r;
			dst[1] = c.g;
			dst[2] = c.b;
			dst += 3;
			src++;
		}
		
		tex.loadData(temp);

	}
	
	Stream::updateTextureIfNeeded();
}

