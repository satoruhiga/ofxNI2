#include "ofxNiTE2.h"

namespace ofxNiTE2
{
	void init()
	{
		static bool inited = false;
		if (inited) return;
		inited = true;
		
		nite::NiTE::initialize();
	}
	
	void check_error(nite::Status rc)
	{
		if (rc == nite::STATUS_OK) return;
		ofLogError("ofxNiTE2") << openni::OpenNI::getExtendedError();
	}
	
	class UserTracker;
}

// ofxNiTE2::UserTracker

bool ofxNiTE2::UserTracker::setup(ofxNI2::Device &device)
{
	ofxNiTE2::init();
	
	openni::Device &dev = device;
	check_error(user_tracker.create(&dev));
	if (!user_tracker.isValid()) return false;
	
	user_tracker.addNewFrameListener(this);
	
	return true;
}

void ofxNiTE2::UserTracker::exit()
{
	if (user_tracker.isValid())
		user_tracker.destroy();
}

void ofxNiTE2::UserTracker::onNewFrame(nite::UserTracker &tracker)
{
	nite::UserTrackerFrameRef userTrackerFrame;
	nite::Status rc = tracker.readFrame(&userTrackerFrame);
	
	if (rc != nite::STATUS_OK)
	{
		check_error(rc);
		return;
	}
	
//	const nite::UserMap& userLabels = userTrackerFrame.getUserMap();
	
	const nite::Array<nite::UserData>& users = userTrackerFrame.getUsers();
	for (int i = 0; i < users.getSize(); ++i)
	{
		const nite::UserData& user = users[i];
	}
	
	cout << "num users: " << users.getSize() << endl;
}
