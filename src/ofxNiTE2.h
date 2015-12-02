#pragma once

#include "ofMain.h"
#include "ofxNI2.h"

#include "NiTE.h"

#ifdef HAVE_NITE2

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
	
	ofVec3f getCenterOfMass() const { return center_of_mass; }
	ofVec3f getCenterOfBone() const { return center_of_bone; }
	float getActivity() const { return activity; }
	
	inline bool isNew() const { return userdata.isNew(); }
	inline bool isVisible() const { return userdata.isVisible(); }
	inline bool isLost() const { return userdata.isLost(); }
	
	inline size_t getNumJoints() { return NITE_JOINT_COUNT; }
	const Joint& getJoint(size_t idx) { return joints[idx]; }
	const Joint& getJoint(nite::JointType type) { return joints[(nite::JointType)type]; }
	
	void draw();
	
	nite::UserData get() { return userdata; }
	const nite::UserData& get() const { return userdata; }
	
protected:

	string status_string;
	nite::UserData userdata;
	vector<Joint> joints;
	
	float activity;
	
	ofVec3f center_of_mass;
	ofVec3f center_of_bone;
	
	void buildSkeleton();
	void updateUserData(const nite::UserData& data);
	
};

class ofxNiTE2::UserTracker : public nite::UserTracker::NewFrameListener
{
public:
	
	ofEvent<User::Ref> newUser;
	ofEvent<User::Ref> lostUser;
	
	bool setup(ofxNI2::Device &device);
	void exit();
	
	void clear();
	
	ofShortPixels& getPixelsRef() { return pix.getFrontBuffer(); }
	ofPixels getPixelsRef(int near, int far, bool invert = false);
	
	void draw();
	
	ofCamera getOverlayCamera() { return overlay_camera; }
	
	size_t getNumUser() const { return users_arr.size(); }
	User::Ref getUser(size_t idx) { return users_arr.at(idx); }
	User::Ref getUserByID(nite::UserId id) { return users[id]; }
	
	void setSkeletonSmoothingFactor(float factor) { user_tracker.setSkeletonSmoothingFactor(factor); }
	float getSkeletonSmoothingFactor(float factor) { return user_tracker.getSkeletonSmoothingFactor(); }
	
	nite::UserTracker get() { return user_tracker; }
	const nite::UserTracker& get() const { return user_tracker; }
    
    inline nite::Plane getFloor() { return userTrackerFrame.getFloor(); }
    inline float getFloorConfidence() { return userTrackerFrame.getFloorConfidence(); }
	
	inline nite::UserTrackerFrameRef getFrame() const { return userTrackerFrame; }
	
protected:
	
	ofxNI2::DoubleBuffer<ofShortPixels> pix;
	
	ofxNI2::Device *device;
	
	nite::UserTracker user_tracker;
	nite::UserMap user_map;
	
	nite::UserTrackerFrameRef userTrackerFrame;
	
	vector<nite::UserData> users_data;
	
	map<nite::UserId, User::Ref> users;
	vector<User::Ref> users_arr;

	ofMutex *mutex;
	ofCamera overlay_camera;
	
	void onNewFrame(nite::UserTracker &tracker);
	void onUpdate(ofEventArgs&);
};

#endif
