#pragma once

#include "ofMain.h"
#include "ofxNI2.h"
#include "ofxNiTE2.h"
#include "AssimpModel.h"

class testApp : public ofBaseApp{
public:
	void setup();
	void exit();
	void update();
	void draw();
	
	ofxNI2::Device device;
	ofxNiTE2::UserTracker tracker;
    ofxNiTE2::AssimpModel model;
    
    ofEasyCam cam;
    ofLight light;
    ofImage depthImage;
};