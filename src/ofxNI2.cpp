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
		
		if (ofFile::doesFileExist("OpenNI2"))
		{
			string path = ofToDataPath("OpenNI2/lib/Drivers");
			setenv("OPENNI2_DRIVERS_PATH", path.c_str(), 1);
			assert_error(openni::OpenNI::initialize());
		}
		else
		{
			ofLogError("ofxNI2") << "data/OpenNI2 folder not found";
			ofExit(-1);
		}
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
	for (int i = 0; i < streams.size(); i++)
		streams[i]->exit();
	
	streams.clear();
	device.close();
}

void Device::update()
{
	for (int i = 0; i < streams.size(); i++)
	{
		Stream *s = streams[i];
		s->is_frame_new = false;
	}
}

// Stream

Stream::Stream() {}
Stream::~Stream() { exit(); }

bool Stream::setup(ofxNI2::Device &device, openni::SensorType sensor_type)
{
	openni_timestamp = 0;
	
	check_error(stream.create(device, sensor_type));
	if (!stream.isValid()) return false;
	
	device.streams.push_back(this);
	
	setMirror(false);
	
	stream.addNewFrameListener(this);
	
	return true;
}

void Stream::exit()
{
	stream.stop();
	stream.destroy();
}

void Stream::start()
{
	if (stream.isValid())
	{
		assert_error(stream.start());
	}
}

void Stream::onNewFrame(openni::VideoStream&)
{
	openni::VideoFrameRef frame;
	check_error(stream.readFrame(&frame));
	setPixels(frame);
	
	openni_timestamp = frame.getTimestamp();
	
	is_frame_new = true;
	texture_needs_update = true;
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

void Stream::setMirror(bool v)
{
	stream.setMirroringEnabled(v);
}

bool Stream::getMirror()
{
	return stream.getMirroringEnabled();
}

void Stream::setPixels(openni::VideoFrameRef frame)
{
	openni_timestamp = frame.getTimestamp();
}

void Stream::updateTextureIfNeeded()
{
	texture_needs_update = false;
}

void Stream::draw(float x, float y)
{
	draw(x, y, getWidth(), getHeight());
}

void Stream::draw(float x, float y, float w, float h)
{
	if (texture_needs_update)
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
		unsigned char *dst = pix.getBackBuffer().getPixels();

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
		unsigned char *dst = pix.getBackBuffer().getPixels();

		for (int i = 0; i < num_pixels; i++)
		{
			dst[0] = src[0] >> 2;
			src++;;
			dst++;
		}
	}
	
	pix.swap();
}

void IrStream::updateTextureIfNeeded()
{
	if (!tex.isAllocated()
		|| tex.getWidth() != getWidth()
		|| tex.getHeight() != getHeight())
	{
		tex.allocate(getWidth(), getHeight(), GL_LUMINANCE);
	}
	
	tex.loadData(pix.getFrontBuffer());
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
		unsigned char *dst = pix.getBackBuffer().getPixels();
		
		for (int i = 0; i < num_pixels; i++)
		{
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			src += 3;
			dst += 3;
		}
	}
	
	pix.swap();
}

void ColorStream::updateTextureIfNeeded()
{
	if (!tex.isAllocated()
		|| tex.getWidth() != getWidth()
		|| tex.getHeight() != getHeight())
	{
		tex.allocate(getWidth(), getHeight(), GL_RGB);
	}
	
	tex.loadData(pix.getFrontBuffer());
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
	
	pix.allocate(w, h, 1);
	pix.getBackBuffer().setFromPixels(pixels, w, h, OF_IMAGE_GRAYSCALE);
	pix.swap();
}

void DepthStream::updateTextureIfNeeded()
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

	tex.loadData(pix.getFrontBuffer());
	Stream::updateTextureIfNeeded();
}


void DepthStream::DepthShader::setup()
{
	setupShaderFromSource(GL_FRAGMENT_SHADER, getShaderCode());
	linkProgram();
}

string DepthStream::Grayscale::getShaderCode() const
{
#define _S(src) #src
	
	const char *fs = _S(
		uniform sampler2DRect tex;
		uniform float min_value;
		uniform float max_value;

		void main()
		{
			float c = texture2DRect(tex, gl_TexCoord[0].xy).r;
			
			c -= min_value;
			if (c > 0.)
			{
				c /= max_value;
				if (c > 1.)
					c = 1.;
			}
			else
			{
				c = 0.;
			}
			
			gl_FragColor = gl_Color * vec4(c, c, c, 1.);
		}
	);
#undef _S
	
	return fs;
}

void DepthStream::Grayscale::begin()
{
	const float dd = 1. / numeric_limits<unsigned short>::max();
	
	DepthStream::DepthShader::begin();
	setUniform1f("min_value", dd * min_value);
	setUniform1f("max_value", dd * max_value);
}

