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
	ofAddListener(ofEvents().draw, this, &Device::onUpdate);
}

void Device::exit()
{
	device.close();
	waitForThread();
}

void Device::onUpdate(ofEventArgs &e)
{
	for (int i = 0; i < streams.size(); i++)
	{
		streams[i]->is_frame_new = false;
	}
}

void Device::start()
{
	startThread();
}

void Device::threadedFunction()
{
	while (isThreadRunning() && device.isValid())
	{
		lock();
		
		vector<openni::VideoStream*> ni_streams;
		openni::Status rc;
		
		for (int i = 0; i < streams.size(); i++)
		{
			if (streams[i]->stream.isValid())
				ni_streams.push_back(&streams[i]->stream);
		}
		
		if (ni_streams.size())
		{
			int index;
			rc = openni::OpenNI::waitForAnyStream(&ni_streams[0], ni_streams.size(), &index, 1);
			
			if (rc == openni::STATUS_OK && index >= 0)
			{
				openni::VideoFrameRef frame;
				streams[index]->copyFrame();
			}
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
	
	check_error(stream.create(device, sensor_type));
	if (!stream.isValid())
	{
		return false;
	}
	
	device.lock();
	device.streams.push_back(this);
	device.unlock();
	
	return true;
}

void Stream::start()
{
	if (stream.isValid())
	{
		assert_error(stream.start());
	}
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
	is_frame_new = true;
}

void Stream::updateTextureIfNeeded()
{
}

void Stream::draw(float x, float y)
{
	draw(x, y, getWidth(), getHeight());
}

void Stream::draw(float x, float y, float w, float h)
{
	if (isFrameNew())
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
	if (!pix.isAllocated()) return;
	
	if (!tex.isAllocated()
		|| tex.getWidth() != getWidth()
		|| tex.getHeight() != getHeight())
	{
		tex.allocate(getWidth(), getHeight(), GL_LUMINANCE);
	}
	
	tex.loadData(pix);
	
	Stream::updateTextureIfNeeded();
}

// ColorStream

void ColorStream::setPixels(openni::VideoFrameRef frame)
{
	Stream::setPixels(frame);
	
	openni::VideoMode m = frame.getVideoMode();
	
	int w = m.getResolutionX();
	int h = m.getResolutionY();
	int num_pixels = w * h;
	
	pix.allocate(w, h, 3);
	
	if (m.getPixelFormat() == openni::PIXEL_FORMAT_RGB888)
	{
		const unsigned char *src = (const unsigned char*)frame.getData();
		unsigned char *dst = pix.getPixels();
		
		for (int i = 0; i < num_pixels; i++)
		{
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			src += 3;
			dst += 3;
		}
	}
}

void ColorStream::updateTextureIfNeeded()
{
	if (!pix.isAllocated()) return;
	
	if (!tex.isAllocated()
		|| tex.getWidth() != getWidth()
		|| tex.getHeight() != getHeight())
	{
		tex.allocate(getWidth(), getHeight(), GL_RGB);
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
	int num_pixels = w * h;
	raw.setFromPixels(pixels, w, h, OF_IMAGE_GRAYSCALE);
	
	pix.allocate(w, h, 1);
	
	{
		const unsigned short *src = pixels;
		unsigned char *dst = pix.getPixels();
		
		float d = 1. / (far_clip - near_clip);
		
		unsigned short m_min = INT_MAX, m_max = INT_MIN;
		for (int i = 0; i < num_pixels; i++)
		{
			float p = ((float)((float)*src - near_clip) * d);
			if (p < 0) p = 0;
			if (p > 1) p = 1;
			*dst = p * 255;
			src++;
			dst++;
		}
	}
}

void DepthStream::updateTextureIfNeeded()
{
	if (!pix.isAllocated()) return;
	
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
			|| tex.getHeight() != getHeight()
			|| tex.getTextureData().glType != GL_LUMINANCE)
		{
			tex.allocate(getWidth(), getHeight(), GL_LUMINANCE);
		}

		tex.loadData(pix);
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
		
		const unsigned char *src = (const unsigned char*)pix.getPixels();
		unsigned char *dst = temp.getPixels();
		
		for (int i = 0; i < num_pixels; i++)
		{
			ofColor c = ofColor::fromHsb(*src, 255, 255);
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

