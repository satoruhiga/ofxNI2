#pragma once

#include "ofMain.h"
#include "ofxNI2.h"

#include "NiTE.h"

namespace ofxNiTE2
{
	class UserTracker;
	class User;
	class Joint;
}

class ofxNiTE2::Joint : public ofNode
{
	friend class User;
	
public:
	
	inline float getPositionConfidence() const { return joint.getPositionConfidence(); }
	inline float getOrientationConfidence() const { return joint.getOrientationConfidence(); }
	
	void draw();
	
	nite::SkeletonJoint get() { return joint; }
	const nite::SkeletonJoint& get() const { return joint; }

protected:
	
	nite::SkeletonJoint joint;
	
	void updateJointData(const nite::SkeletonJoint& data);
	
};

class ofxNiTE2::User
{
	friend class UserTracker;
	
public:
	
	typedef ofPtr<User> Ref;
	
	User() { buildSkeleton(); }
	
	inline nite::UserId getId() const { return userdata.getId(); }
	
	inline bool isNew() const { return userdata.isNew(); }
	inline bool isVisible() const { return userdata.isVisible(); }
	inline bool isLost() const { return userdata.isLost(); }
	
	inline size_t getNumJoints() { return NITE_JOINT_COUNT; }
	const ofNode& getJoint(size_t idx) { return joints[idx]; }
	const ofNode& getJoint(nite::JointType type) { return joints[(nite::JointType)type]; }
	
	void draw();
	
	nite::UserData get() { return userdata; }
	const nite::UserData& get() const { return userdata; }
	
protected:

	string status_string;
	nite::UserData userdata;
	vector<Joint> joints;
	
	void buildSkeleton();
	void updateUserData(const nite::UserData& data);
	
};

class ofxNiTE2::UserTracker : public nite::UserTracker::NewFrameListener
{
public:
	bool setup(ofxNI2::Device &device);
	void exit();
	
	ofShortPixels& getPixelsRef() { return pix.getFrontBuffer(); }
	ofPixels getPixelsRef(int near, int far, bool invert = false);
	
	void draw();
	
	ofCamera getOverlayCamera() { return overlay_camera; }
	
	void setSkeletonSmoothingFactor(float factor) { user_tracker.setSkeletonSmoothingFactor(factor); }
	float getSkeletonSmoothingFactor(float factor) { return user_tracker.getSkeletonSmoothingFactor(); }
	
	nite::UserTracker get() { return user_tracker; }
	const nite::UserTracker& get() const { return user_tracker; }
	
protected:
	
	ofxNI2::DoubleBuffer<ofShortPixels> pix;
	
	nite::UserTracker user_tracker;
	nite::UserMap user_map;
	
	map<nite::UserId, User::Ref> users;
	
	ofMutex *mutex;
	ofCamera overlay_camera;
	
	void onNewFrame(nite::UserTracker &tracker);
	
};
