#include "testApp.h"

#include "ofxNI2.h"

ofxNI2::Device *device;
ofxNI2::IrStream ir;
ofxNI2::ColorStream color;
ofxNI2::DepthStream depth;

//--------------------------------------------------------------
void testApp::setup()
{
	ofSetFrameRate(60);
	ofSetVerticalSync(true);
	ofBackground(0);
	
	device = new ofxNI2::Device;
	device->setup();
	
	if (depth.setup(*device))
	{
		depth.setSize(320, 240);
		depth.setFps(30);
		depth.start();
	}

	if (ir.setup(*device)) // only for xtion device (OpenNI2-FreenectDriver issue)
	{
		ir.setSize(320, 240);
		ir.setFps(30);
		ir.start();
	}
	
//	if (color.setup(*device)) // only for kinect device
//	{
//		color.setSize(320, 240);
//		color.setFps(60);
//		color.start();
//	}
}

void testApp::exit()
{
	device->exit();
	delete device;
}

//--------------------------------------------------------------
void testApp::update()
{
}

//--------------------------------------------------------------
void testApp::draw()
{
	ir.draw();
//	color.draw();
	depth.draw(320, 0);
}

//--------------------------------------------------------------
void testApp::keyPressed(int key)
{
}

//--------------------------------------------------------------
void testApp::keyReleased(int key)
{

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y)
{

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button)
{

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button)
{

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button)
{

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h)
{

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg)
{

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo)
{

}