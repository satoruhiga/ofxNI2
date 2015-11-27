#include "ofxNI2.h"

#include "utils/DepthRemapToRange.h"

namespace ofxNI2
{
	bool assert_error(openni::Status rc)
	{
		if (rc == openni::STATUS_OK) return true;
		ofLogError("ofxNI2") << openni::OpenNI::getExtendedError();
		throw;
	}

	bool check_error(openni::Status rc)
	{
		if (rc == openni::STATUS_OK) return true;
		ofLogError("ofxNI2") << openni::OpenNI::getExtendedError();
		return false;
	}

	void init()
	{
		static bool inited = false;
		if (inited) return;
		inited = true;

        string path;
#ifndef TARGET_WIN32
        path = ofFilePath::getCurrentExeDir() + "/Drivers"; // osx / linux
#else
        path = ofFilePath::getCurrentExeDir() + "/OpenNI2/Drivers"; // windows
#endif
        if (ofFile::doesFileExist(path, false))
        {
#ifndef TARGET_WIN32
            setenv("OPENNI2_DRIVERS_PATH", path.c_str(), 1);
#endif
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

bool Device::setup()
{
	ofxNI2::init();
	
	if (!check_error(device.open(openni::ANY_DEVICE))) return false;
	if (!check_error(device.setDepthColorSyncEnabled(true))) return false;
	
	return true;
}

bool Device::setup(int device_id)
{
	ofxNI2::init();
	
	openni::Array<openni::DeviceInfo> deviceList;
	openni::OpenNI::enumerateDevices(&deviceList);
	
	if (device_id < 0
		|| device_id >= deviceList.getSize())
	{
		ofLogFatalError("ofxNI2::Device") << "invalid device id";
		
		listDevices();
		
		return false;
	}
	
	if (!check_error(device.open(deviceList[device_id].getUri()))) return false;
	if (!check_error(device.setDepthColorSyncEnabled(true))) return false;
	
	return true;
}

bool Device::setup(string oni_file_path)
{
	ofxNI2::init();
	
	oni_file_path = ofToDataPath(oni_file_path);
	
	assert_error(device.open(oni_file_path.c_str()));
	check_error(device.setDepthColorSyncEnabled(true));
	
	return true;
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
	
	static ofEventArgs e;
	ofNotifyEvent(updateDevice, e, this);
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
	if (recorder) return false;

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

bool DepthStream::setup(ofxNI2::Device &device)
{
	setupShader<Grayscale>();
	return Stream::setup(device, openni::SENSOR_DEPTH);
}

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
		|| tex.getHeight() != getHeight())
	{
#if OF_VERSION_MINOR <= 7
		static ofTextureData data;
		
		data.pixelType = GL_UNSIGNED_SHORT;
		data.glTypeInternal = GL_LUMINANCE16;
		data.width = getWidth();
		data.height = getHeight();
		
		tex.allocate(data);
#elif OF_VERSION_MINOR > 7
        tex.allocate(pix.getFrontBuffer());
#endif
	}

	tex.loadData(pix.getFrontBuffer());
	Stream::updateTextureIfNeeded();
}

ofPixels DepthStream::getPixelsRef(int _near, int _far, bool invert)
{
	ofPixels pix;
	depthRemapToRange(getPixelsRef(), pix, _near, _far, invert);
	return pix;
}

void DepthStream::draw(float x, float y, float w, float h)
{
	if (shader)
	{
		shader->begin();
		ofxNI2::Stream::draw(x, y, w, h);
		shader->end();
	}
	else
	{
		ofxNI2::Stream::draw(x, y, w, h);
	}
}

// depth shader

void DepthShader::setup(DepthStream &depth)
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

string Grayscale::getShaderCode() const
{
#define _S(src) #src
	
	const char *fs = _S(
		uniform sampler2DRect tex;
		uniform float near_value;
		uniform float far_value;

		void main()
		{
			float c = texture2DRect(tex, gl_TexCoord[0].xy).r;
			
			c = (near_value >= far_value) ? 0. : clamp((c-near_value)/(far_value-near_value), 0., 1.);
			
			gl_FragColor = gl_Color * vec4(c, c, c, 1.);
		}
	);
#undef _S
	
	return fs;
}

void Grayscale::begin()
{
	const float dd = 1. / numeric_limits<unsigned short>::max();
	
	DepthShader::begin();
	setUniform1f("near_value", dd * near_value);
	setUniform1f("far_value", dd * far_value);
}
