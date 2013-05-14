#pragma once

#include "ofxNI2.h"

namespace ofxNI2
{
	class MeshGenerator;
};

class ofxNI2::MeshGenerator
{
public:
	
	MeshGenerator() : downsampling_level(1) {}
	
	void setup(DepthStream& depth_stream)
	{
		float fovH = depth_stream.get().getHorizontalFieldOfView();
		float fovV = depth_stream.get().getVerticalFieldOfView();
		
		xzFactor = tan(fovH * 0.5) * 2;
		yzFactor = tan(fovV * 0.5) * 2;
	}
	
	const ofMesh& update(const ofShortPixels& depth, const ofPixels& color = ofPixels())
	{
		assert(depth.getNumChannels() == 1);
		
		const int W = depth.getWidth();
		const int H = depth.getHeight();
		const float invW = 1. / W;
		const float invH = 1. / H;
		
		const unsigned short *depth_pix = depth.getPixels();
		
		bool has_color = color.isAllocated();
		const float inv_byte = 1. / 255.;
		
		mesh.clear();
		mesh.setMode(OF_PRIMITIVE_POINTS);
		
		const int DS = downsampling_level;
		
		if (has_color)
		{
			const unsigned char *color_pix = color.getPixels();
			
			if (color.getNumChannels() == 1)
			{
				for (int y = 0; y < H; y += DS)
				{
					for (int x = 0; x < W; x += DS)
					{
						const int idx = y * W + x;
						
						const float Z = depth_pix[idx];
						const float normX = x * invW - 0.5;
						const float normY = y * invH - 0.5;
						const float X = normX * xzFactor * Z;
						const float Y = normY * yzFactor * Z;
						mesh.addVertex(ofVec3f(X, Y, Z));
						
						const unsigned char *C = &color_pix[idx];
						mesh.addColor(ofFloatColor(C[0] * inv_byte));
					}
				}
			}
			else if (color.getNumChannels() == 3)
			{
				for (int y = 0; y < H; y += DS)
				{
					for (int x = 0; x < W; x += DS)
					{
						const int idx = y * W + x;
						
						const float Z = depth_pix[idx];
						const float normX = x * invW - 0.5;
						const float normY = y * invH - 0.5;
						const float X = normX * xzFactor * Z;
						const float Y = normY * yzFactor * Z;
						mesh.addVertex(ofVec3f(X, Y, Z));
						
						const unsigned char *C = &color_pix[idx * 3];
						mesh.addColor(ofFloatColor(C[0] * inv_byte,
												   C[1] * inv_byte,
												   C[2] * inv_byte));
					}
				}
			}
			else throw;
		}
		else
		{
			for (int y = 0; y < H; y += DS)
			{
				for (int x = 0; x < W; x += DS)
				{
					int idx = y * W + x;
					
					float Z = depth_pix[idx];
					float X = (x * invW - 0.5) * xzFactor * Z;
					float Y = (y * invH - 0.5) * yzFactor * Z;
					mesh.addVertex(ofVec3f(X, Y, Z));
				}
			}
		}
		
		return mesh;
	}
	
	void draw()
	{
		mesh.draw();
	}
	
	void setDownsamplingLevel(int level) { downsampling_level = level; }
	int getDownsamplingLevel() const { return downsampling_level; }
	
protected:
	
	int downsampling_level;
	
	ofMesh mesh;
	float xzFactor, yzFactor;
};
