#pragma once
#include "igl/opengl/glfw/Viewer.h"
#include "igl/aabb.h"
#include <igl/shortest_edge_and_midpoint.h>
#include <igl/parallel_for.h>
#include <igl/edge_flaps.h>
#include <igl/collapse_edge.h>

class SandBox : public igl::opengl::glfw::Viewer
{
public:
	SandBox();
	~SandBox();
	double Xdir;
	double Ydir;
	void Init(const std::string& config);
	double doubletemp;
private:
	// Prepare array-based edge data structures and priority queue
	
	void ccd_impl();
	void fabrik_impl();
	void Animate();

};



