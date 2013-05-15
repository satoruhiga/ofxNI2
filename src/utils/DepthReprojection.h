#pragma once

#include "ofxNI2.h"

namespace ofxNI2
{
	class DepthReprojection;
}

class ofxNI2::DepthReprojection
{
public:
	
	void setup(DepthStream& depth_stream)
	{
		cam.setFov(depth_stream.getVerticalFieldOfView());
		
		ofFbo::Settings s;
		s.width = depth_stream.getWidth();
		s.height = depth_stream.getHeight();
		s.internalformat = GL_RGB16F;
		s.numColorbuffers = 2;
		s.useDepth = true;
		
		fbo.allocate(s);
		fbo.begin();
		ofClear(0);
		fbo.end();
		
#define _S(src) #src
		
		const char *vs = _S(
			varying float depth;
			void main()
			{
				gl_FrontColor = gl_Color;
				gl_TexCoord[0] = gl_MultiTexCoord0;
				gl_Position = ftransform();
				depth = gl_Vertex.z;
			}
		);
		
		const char *fs = _S(
			varying float depth;
			void main()
			{
				gl_FragData[0] = gl_Color;
				
				float d = depth / 65025.;
				gl_FragData[1] = vec4(d, d, d, 1.);
			}
		);
		
		shader.setupShaderFromSource(GL_VERTEX_SHADER, vs);
		shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fs);
		shader.linkProgram();
		
#undef _S
	}
	
	void update(ofMesh &mesh)
	{
		fbo.begin();
		
		vector<int> ids;
		ids.push_back(0);
		ids.push_back(1);
		
		fbo.setActiveDrawBuffers(ids);
		
		ofPushView();
		{
			ofViewport(0, 0, fbo.getWidth(), fbo.getHeight(), true);
			ofSetupScreenPerspective(fbo.getWidth(), fbo.getHeight(),
									 OF_ORIENTATION_DEFAULT, false);
			
			shader.begin();
			{
				ofClear(0, 0);
				
				glPushAttrib(GL_ALL_ATTRIB_BITS);
				
				glEnable(GL_DEPTH_TEST);
				glPointSize(2);
				
				ofPushMatrix();
				{
					cam.begin();
					glScalef(1, 1, -1);
					mesh.draw();
					cam.end();
				}
				ofPopMatrix();
				
				glPopAttrib();
			}
			shader.end();
		}
		ofPopView();

		fbo.end();
	}
	
	void drawColor(int x, int y) { getColorTextureReference().draw(x, y); }
	void drawColor(int x, int y, int w, int h) { getColorTextureReference().draw(x, y, w, h); }
	
	void drawDepth(int x, int y) { getDepthTextureReference().draw(x, y); }
	void drawDepth(int x, int y, int w, int h) { getDepthTextureReference().draw(x, y, w, h); }
	
	inline ofTexture& getColorTextureReference() { return fbo.getTextureReference(0); }
	inline ofTexture& getDepthTextureReference() { return fbo.getTextureReference(1); }
	
	void setTransformMatrix(const ofMatrix4x4& mat) { cam.setTransformMatrix(mat); }
	ofMatrix4x4 getTransformMatrix() const { return cam.getLocalTransformMatrix(); }
	
protected:
	
	ofCamera cam;
	ofFbo fbo;
	ofShader shader;
	
};
