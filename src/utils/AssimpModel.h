//
//  AssimpModel.h
//
//  Created by kikko.fr / @kikko_fr on 7/4/13.
//
//

#ifndef __example_user_tracker__AssimpModel__
#define __example_user_tracker__AssimpModel__

#include "ofMain.h"
#include "ofxNiTE2.h"
#include "ofxAssimpModelLoader.h"
#include "aiMesh.h"
#include "aiScene.h"

using namespace nite;

namespace ofxNiTE2
{
    class AssimpJoint;
    class AssimpModel;
}

class ofxNiTE2::AssimpJoint{
public:
    AssimpJoint(JointType type_):type(type_){}
    JointType type;
    aiMatrix4x4 transform;
};


class ofxNiTE2::AssimpModel : public ofxAssimpModelLoader{

public:
    
    AssimpModel(){}
    virtual ~AssimpModel(){};
    
    void setupBones(bool bSetDefaultBoneNames=true){
        
        if(bSetDefaultBoneNames){
            set("neck", JOINT_NECK);
            set("torso", JOINT_TORSO);
            set("right_shoulder", JOINT_RIGHT_SHOULDER);
            set("left_shoulder", JOINT_LEFT_SHOULDER);
            set("right_elbow", JOINT_RIGHT_ELBOW);
            set("left_elbow", JOINT_LEFT_ELBOW);
            set("right_hip", JOINT_RIGHT_HIP);
            set("left_hip", JOINT_LEFT_HIP);
            set("right_knee", JOINT_RIGHT_KNEE);
            set("left_knee", JOINT_LEFT_KNEE);
        }
        
        for(unsigned int i = 0; i < getNumMeshes(); ++i){
            
            const aiMesh * mesh = getMeshHelper(i).mesh;
            
            for(unsigned int a = 0; a < mesh->mNumBones; ++a){
                
                const aiBone * bone = mesh->mBones[a];
                string bname = bone->mName.data;
                
                if(!isSet(bname)){
                    continue;
                }
                
                aiNode * node = getAssimpScene()->mRootNode->FindNode(bone->mName);
                jointsMap[bname]->transform = node->mTransformation;
            }
        }
    }
    
    
    void transformBones(User::Ref user){
        
        for(unsigned int i = 0; i < getNumMeshes(); ++i){
            
            const aiMesh * mesh = getMeshHelper(i).mesh;
            
            for(unsigned int a = 0; a < mesh->mNumBones; ++a){
                
                const aiBone * bone = mesh->mBones[a];
                string bname = bone->mName.data;
                
                if(!isSet(bname)){
                    continue;
                }
                
                JointType type = jointsMap[bname]->type;
                const Joint & joint = user->getJoint(type);
                
                aiMatrix4x4 transform;
                
                // The voodoo trick(s), found using bruteforce trial and error
                // TODO: Translate this into some proper quaternion multipication
                ofVec3f r = joint.getOrientationQuat().getEuler() / 180 * PI;
                switch(type){
                    case JOINT_TORSO: {
                        aiMatrix4x4 rx, ry, rz;
                        rx.RotationX(r.z, rx);
                        ry.RotationY(r.y, ry);
                        rz.RotationZ(r.x, rz);
                        transform = ry * rx * rz;
                    } break;
                    case JOINT_NECK:
                        transform.FromEulerAnglesXYZ(-r.z, r.y, -r.x);
                        break;
                    case JOINT_RIGHT_SHOULDER:
                    case JOINT_RIGHT_ELBOW:
                        transform.FromEulerAnglesXYZ(-r.x, -r.z, -r.y);
                        break;
                    case JOINT_LEFT_SHOULDER:
                    case JOINT_LEFT_ELBOW:
                        transform.FromEulerAnglesXYZ(r.x, r.z, -r.y);
                        break;
                    default:
                        transform.FromEulerAnglesXYZ(-r.z, r.y, r.x);
                        break;
                }
                
                if(type == JOINT_TORSO){
                    ofVec3f jointPos = joint.getPosition();
                    float scale = getNormalizedScale();
                    transform.a4 = jointPos.x / scale;
                    transform.b4 = jointPos.y / scale;
                    transform.c4 = jointPos.z / scale;
                }
                
                aiNode * node = getAssimpScene()->mRootNode->FindNode(bone->mName);
                node->mTransformation = jointsMap[bname]->transform * transform;
            }
        }
        
        updateBones();
        updateGLResources();
    }
    
    inline void set(string name, nite::JointType type) {
        jointsMap[name] = ofPtr<AssimpJoint>(new AssimpJoint(type));
    }
    inline bool isSet(string name) {
        return jointsMap.find(name) != jointsMap.end();
    }
    
protected:
    
    map<string, ofPtr<AssimpJoint> > jointsMap;
};

#endif /* defined(__example_user_tracker__ofxNiTE2AssimpModel__) */
