#pragma once

#include "ofMain.h"
#include "ofxNI2.h"

#include "NiTE.h"

namespace ofxNiTE2
{
	class UserTracker;
}

class ofxNiTE2::UserTracker : public nite::UserTracker::NewFrameListener
{
public:
	
	bool setup(ofxNI2::Device &device);
	void exit();
	
protected:
	
	nite::UserTracker user_tracker;
	
	void onNewFrame(nite::UserTracker &tracker);
};
