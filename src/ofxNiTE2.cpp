#include "ofxNiTE2.h"

#include "utils/DepthRemapToRange.h"

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

using namespace ofxNiTE2;

#pragma mark - UserTracker

bool UserTracker::setup(ofxNI2::Device &device)
{
	ofxNiTE2::init();
	
	mutex = new ofMutex;
	
	{
		// get device FOV for overlay camera;
		
		openni::VideoStream stream;
		stream.create(device, openni::SENSOR_DEPTH);
		
		float fov = stream.getVerticalFieldOfView();
		overlay_camera.setFov(ofRadToDeg(fov));
		overlay_camera.setNearClip(500);
		
		stream.destroy();
	}
	
	openni::Device &dev = device;
	check_error(user_tracker.create(&dev));
	if (!user_tracker.isValid()) return false;
	
	user_tracker.addNewFrameListener(this);
	user_tracker.setSkeletonSmoothingFactor(0.9);
	
	return true;
}

void UserTracker::exit()
{
	map<nite::UserId, User::Ref>::iterator it = users.begin();
	while (it != users.end())
	{
		user_tracker.stopSkeletonTracking(it->first);
		it++;
	}
	
	users.clear();

	if (user_tracker.isValid())
	{
		user_tracker.removeNewFrameListener(this);
		user_tracker.destroy();
	}
}

void UserTracker::onNewFrame(nite::UserTracker &tracker)
{
	nite::UserTrackerFrameRef userTrackerFrame;
	nite::Status rc = tracker.readFrame(&userTrackerFrame);
	
	if (rc != nite::STATUS_OK)
	{
		check_error(rc);
		return;
	}
	
	user_map = userTrackerFrame.getUserMap();
	
	mutex->lock();
	
	users_arr.clear();
	
	{
		const nite::Array<nite::UserData>& users_data = userTrackerFrame.getUsers();
		for (int i = 0; i < users_data.getSize(); i++)
		{
			const nite::UserData& user = users_data[i];
			
			User::Ref user_ptr;
			
			if (user.isNew())
			{
				user_ptr = User::Ref(new User);
				users[user.getId()] = user_ptr;
				user_tracker.startSkeletonTracking(user.getId());
			}
			else if (user.isLost())
			{
				// emit lost user event
				ofNotifyEvent(lostUser, users[user.getId()], this);
				
				user_tracker.stopSkeletonTracking(user.getId());
				users.erase(user.getId());
				continue;
			}
			else
			{
				user_ptr = users[user.getId()];
			}
			
			user_ptr->updateUserData(user);
			users_arr.push_back(user_ptr);
			
			if (user.isNew())
			{
				// emit new user event
				ofNotifyEvent(newUser, user_ptr, this);
			}
		}
	}
	
	mutex->unlock();
	
	{
		openni::VideoFrameRef frame = userTrackerFrame.getDepthFrame();
		
		const unsigned short *pixels = (const unsigned short*)frame.getData();
		int w = frame.getVideoMode().getResolutionX();
		int h = frame.getVideoMode().getResolutionY();
		int num_pixels = w * h;
		
		pix.allocate(w, h, 1);
		pix.getBackBuffer().setFromPixels(pixels, w, h, OF_IMAGE_GRAYSCALE);
		pix.swap();
	}
}

ofPixels UserTracker::getPixelsRef(int near, int far, bool invert)
{
	ofPixels pix;
	ofxNI2::depthRemapToRange(getPixelsRef(), pix, near, far, invert);
	return pix;
}

void UserTracker::draw()
{
	mutex->lock();
	
	map<nite::UserId, User::Ref>::iterator it = users.begin();
	while (it != users.end())
	{
		it->second->draw();
		it++;
	}
	
	mutex->unlock();
}

#pragma mark - User

void User::updateUserData(const nite::UserData& data)
{
	userdata = data;
	
	for (int i = 0; i < NITE_JOINT_COUNT; i++)
	{
		const nite::SkeletonJoint &o = data.getSkeleton().getJoint((nite::JointType)i);
		joints[i].updateJointData(o);
	}
	
	stringstream ss;
	ss << "[" << data.getId() << "]" << endl;
	
	ss << (data.isVisible() ? "Visible" : "Out of Scene") << endl;;
	
	switch (data.getSkeleton().getState())
	{
		case nite::SKELETON_NONE:
			ss << "Stopped tracking.";
			break;
		case nite::SKELETON_CALIBRATING:
			ss << "Calibrating...";
			break;
		case nite::SKELETON_TRACKED:
			ss << "Tracking!";
			break;
		case nite::SKELETON_CALIBRATION_ERROR_NOT_IN_POSE:
		case nite::SKELETON_CALIBRATION_ERROR_HANDS:
		case nite::SKELETON_CALIBRATION_ERROR_LEGS:
		case nite::SKELETON_CALIBRATION_ERROR_HEAD:
		case nite::SKELETON_CALIBRATION_ERROR_TORSO:
			ss << "Calibration Failed... :-|";
			break;
	}
	
	status_string = ss.str();
}

void User::draw()
{
	for (int i = 0; i < joints.size(); i++)
	{
		joints[i].draw();
	}
	
	const nite::Point3f& pos = userdata.getCenterOfMass();
	ofDrawBitmapString(status_string, pos.x, pos.y, -pos.z);
}

void User::buildSkeleton()
{
	joints.resize(NITE_JOINT_COUNT);
	
	using namespace nite;
	
#define BIND(parent, child) joints[child].setParent(joints[parent]);
	
	BIND(JOINT_TORSO, JOINT_NECK);
	BIND(JOINT_NECK, JOINT_HEAD);
	
	BIND(JOINT_TORSO, JOINT_LEFT_SHOULDER);
	BIND(JOINT_LEFT_SHOULDER, JOINT_LEFT_ELBOW);
	BIND(JOINT_LEFT_ELBOW, JOINT_LEFT_HAND);
	
	BIND(JOINT_TORSO, JOINT_RIGHT_SHOULDER);
	BIND(JOINT_RIGHT_SHOULDER, JOINT_RIGHT_ELBOW);
	BIND(JOINT_RIGHT_ELBOW, JOINT_RIGHT_HAND);
	
	BIND(JOINT_TORSO, JOINT_LEFT_HIP);
	BIND(JOINT_LEFT_HIP, JOINT_LEFT_KNEE);
	BIND(JOINT_LEFT_KNEE, JOINT_LEFT_FOOT);
	
	BIND(JOINT_TORSO, JOINT_RIGHT_HIP);
	BIND(JOINT_RIGHT_HIP, JOINT_RIGHT_KNEE);
	BIND(JOINT_RIGHT_KNEE, JOINT_RIGHT_FOOT);
	
#undef BIND
	
}

#pragma mark - Joint

inline static void billboard()
{
    ofMatrix4x4 m;
    glGetFloatv(GL_MODELVIEW_MATRIX, m.getPtr());
    
    ofVec3f s = m.getScale();
    
    m(0, 0) = s.x;
    m(0, 1) = 0;
    m(0, 2) = 0;
    
    m(1, 0) = 0;
    m(1, 1) = s.y;
    m(1, 2) = 0;
    
    m(2, 0) = 0;
    m(2, 1) = 0;
    m(2, 2) = s.z;
    
    glLoadMatrixf(m.getPtr());
}

void Joint::draw()
{
	ofNode *parent = getParent();
	
	if (parent)
	{
		parent->transformGL();
		ofLine(ofVec3f(0, 0, 0), getPosition());
		parent->restoreTransformGL();
	}

	transformGL();
	ofDrawAxis(100);
	
	billboard();
	
	ofPushStyle();
	ofFill();
	ofSetColor(255);
	ofCircle(0, 0, 20 * getPositionConfidence());
	ofPopStyle();
	
	restoreTransformGL();
}

void Joint::updateJointData(const nite::SkeletonJoint& data)
{
	joint = data;
	
	const nite::Point3f& pos = data.getPosition();
	const nite::Quaternion& rot = data.getOrientation();
	
	setGlobalOrientation(ofQuaternion(-rot.x, -rot.y, rot.z, rot.w));
	setGlobalPosition(pos.x, pos.y, -pos.z);
}
