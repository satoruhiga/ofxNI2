#include "ofxNI2.h"

#include "DepthRemapToRange.h"

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

		// initialize oF path, don't comment out
		ofToDataPath(".");
		
		if (ofFile::doesFileExist("Drivers", false))
		{
			string path = "Drivers";
			setenv("OPENNI2_DRIVERS_PATH", path.c_str(), 1);
			assert_error(openni::OpenNI::initialize());
		}
		else
		{
			ofLogError("ofxNI2") << "libs not found";
			ofExit(-1);
		}
	}
}

using namespace ofxNI2;

#pragma mark - Device

int Device::listDevices()
{
	ofxNI2::init();
	
	openni::Array<openni::DeviceInfo> deviceList;
	openni::OpenNI::enumerateDevices(&deviceList);
	
	for (int i = 0; i < deviceList.getSize(); ++i)
	{
		printf("[%d] %s [%s] (%s)\n", i, deviceList[i].getName(), deviceList[i].getVendor(), deviceList[i].getUri());
	}
	
	return deviceList.getSize();
}

Device::Device() : recorder(NULL)
{
}

Device::~Device()
{
}

void Device::setup()
{
	ofxNI2::init();
	
	assert_error(device.open(openni::ANY_DEVICE)); 
	assert_error(device.setDepthColorSyncEnabled(true));
}

void Device::setup(int device_id)
{
	ofxNI2::init();
	
	openni::Array<openni::DeviceInfo> deviceList;
	openni::OpenNI::enumerateDevices(&deviceList);
	
	if (device_id < 0
		|| device_id >= deviceList.getSize())
	{
		ofLogFatalError("ofxNI2::Device") << "invalid device id";
		
		listDevices();
		ofExit();
	}
	
	assert_error(device.open(deviceList[device_id].getUri()));
	assert_error(device.setDepthColorSyncEnabled(true));
}

void Device::setup(string oni_file_path)
{
	ofxNI2::init();
	
	oni_file_path = ofToDataPath(oni_file_path);
	
	assert_error(device.open(oni_file_path.c_str()));
	assert_error(device.setDepthColorSyncEnabled(true));
}

void Device::exit()
{
	if (!device.isValid()) return;
	
	stopRecord();
	
	for (int i = 0; i < streams.size(); i++)
		streams[i]->exit();
	
	streams.clear();
}

void Device::update()
{
	for (int i = 0; i < streams.size(); i++)
	{
		Stream *s = streams[i];
		s->is_frame_new = s->openni_timestamp != s->opengl_timestamp;
		s->opengl_timestamp = s->openni_timestamp;
	}
}

bool Device::isRegistrationSupported() const
{
	return device.isImageRegistrationModeSupported(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR);
}

void Device::setEnableRegistration()
{
	check_error(device.setImageRegistrationMode(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR));
}

bool Device::getEnableRegistration() const
{
	return device.getImageRegistrationMode() == openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR;
}

bool Device::startRecord(string filename, bool allowLossyCompression)
{
	if (recorder) return;

	if (filename == "")
		filename = ofToString(time(0)) + ".oni";

	ofLogVerbose("ofxNI2") << "recording started: " << filename;
	
	filename = ofToDataPath(filename);
	
	recorder = new openni::Recorder;
	recorder->create(filename.c_str());
	
	for (int i = 0; i < streams.size(); i++)
	{
		ofxNI2::Stream &s = *streams[i];
		recorder->attach(s.get(), allowLossyCompression);
	}
	
	recorder->start();
	return recorder->isValid();
}

void Device::stopRecord()
{
	if (!recorder) return;
	
	recorder->stop();
	recorder->destroy();
	
	delete recorder;
	recorder = NULL;
}

#pragma mark - Stream

Stream::Stream() {}
Stream::~Stream() {}

bool Stream::setup(ofxNI2::Device &device, openni::SensorType sensor_type)
{
	openni_timestamp = 0;
	opengl_timestamp = 0;
	
	check_error(stream.create(device, sensor_type));
	if (!stream.isValid()) return false;
	
	device.streams.push_back(this);
	this->device = &device;
	
	setMirror(false);
	
	stream.addNewFrameListener(this);
	
	return true;
}

void Stream::exit()
{
	if (!stream.isValid()) return;

	device->streams.erase(remove(device->streams.begin(),
								 device->streams.end(), this),
						  device->streams.end());

	stream.stop();
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


#pragma mark - IrStream

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

#pragma mark - ColorStream

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


#pragma mark - DepthStream

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
		|| tex.getTextureData().pixelType != GL_UNSIGNED_SHORT)
	{
		static ofTextureData data;
		
		data.pixelType = GL_UNSIGNED_SHORT;
		data.glType = GL_LUMINANCE;
		data.width = getWidth();
		data.height = getHeight();
		
		tex.allocate(data);
	}

	tex.loadData(pix.getFrontBuffer());
	Stream::updateTextureIfNeeded();
}

ofPixels DepthStream::getPixelsRef(int near, int far, bool invert)
{
	ofPixels pix;
	depthRemapToRange(getPixelsRef(), pix, near, far, invert);
	return pix;
}

void DepthStream::DepthShader::setup()
{
	setupShaderFromSource(GL_FRAGMENT_SHADER, getShaderCode());
	linkProgram();
}

ofVec3f DepthStream::getWorldCoordinateAt(int x, int y)
{
	ofVec3f v;
	
	const ofShortPixels& pix = getPixelsRef();
	const unsigned short *ptr = pix.getPixels();
	unsigned short z = ptr[pix.getWidth() * y + x];
	
	openni::CoordinateConverter::convertDepthToWorld(stream, x, y, z, &v.x, &v.y, &v.z);

	return v;
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
