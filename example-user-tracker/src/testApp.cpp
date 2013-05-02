#include "testApp.h"

#include "ofxNiTE2.h"

ofxNI2::DepthStream depth;
ofxNiTE2::UserTracker tracker;

//--------------------------------------------------------------
void testApp::setup()
{
	ofSetFrameRate(60);
	ofSetVerticalSync(true);
	ofBackground(0);
	
	device.setup();
	
	if (tracker.setup(device))
	{
		cout << "tracker inited" << endl;
	}
}

void testApp::exit()
{
}

//--------------------------------------------------------------
void testApp::update()
{
	device.update();
}

//--------------------------------------------------------------
void testApp::draw()
{
	// tracker.draw(0, 0);
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