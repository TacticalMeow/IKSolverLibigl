// This file is part of libigl, a simple c++ geometry processing library.
//
// Copyright (C) 2014 Daniele Panozzo <daniele.panozzo@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public License
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.

#include "Viewer.h"

//#include <chrono>
#include <thread>

#include <Eigen/LU>


#include <cmath>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <limits>
#include <cassert>
#include <math.h>

#include <igl/project.h>
//#include <igl/get_seconds.h>
#include <igl/readOBJ.h>
#include <igl/readOFF.h>
#include <igl/adjacency_list.h>
#include <igl/writeOBJ.h>
#include <igl/writeOFF.h>
#include <igl/massmatrix.h>
#include <igl/file_dialog_open.h>
#include <igl/file_dialog_save.h>
#include <igl/quat_mult.h>
#include <igl/axis_angle_to_quat.h>
#include <igl/trackball.h>
#include <igl/two_axis_valuator_fixed_up.h>
#include <igl/snap_to_canonical_view_quat.h>
#include <igl/unproject.h>
#include <igl/serialize.h>
#include "igl/edge_flaps.h"
#include "igl/collapse_edge.h"
#include <igl/shortest_edge_and_midpoint.h>
#include <igl/parallel_for.h>
#include <igl/vertex_triangle_adjacency.h>

// Internal global variables used for glfw event handling
//static igl::opengl::glfw::Viewer * __viewer;
static double highdpi = 1;
static double scroll_x = 0;
static double scroll_y = 0;


namespace igl
{
	namespace opengl
	{
		namespace glfw
		{

			void Viewer::Init(const std::string config)
			{

			}

			IGL_INLINE Viewer::Viewer() :
				data_list(1),
				selected_data_index(0),
				next_data_id(1),
				isPicked(false),
				isActive(false)
			{
				data_list.front().id = 0;



				// Temporary variables initialization
			   // down = false;
			  //  hack_never_moved = true;
				scroll_position = 0.0f;

				// Per face
				data().set_face_based(false);


#ifndef IGL_VIEWER_VIEWER_QUIET
				const std::string usage(R"(igl::opengl::glfw::Viewer usage:
  [drag]  Rotate scene
  A,a     Toggle animation (tight draw loop)
  F,f     Toggle face based
  I,i     Toggle invert normals
  L,l     Toggle wireframe
  O,o     Toggle orthographic/perspective projection
  T,t     Toggle filled faces
  [,]     Toggle between cameras
  1,2     Toggle between models
  ;       Toggle vertex labels
  :       Toggle face labels)"
				);
				std::cout << usage << std::endl;
#endif
			}

			IGL_INLINE Viewer::~Viewer()
			{
			}

			IGL_INLINE bool Viewer::load_mesh_from_file(
				const std::string& mesh_file_name_string)
			{

				// Create new data slot and set to selected
				if (!(data().F.rows() == 0 && data().V.rows() == 0))
				{
					append_mesh();
				}
				data().clear();

				size_t last_dot = mesh_file_name_string.rfind('.');
				if (last_dot == std::string::npos)
				{
					std::cerr << "Error: No file extension found in " <<
						mesh_file_name_string << std::endl;
					return false;
				}

				std::string extension = mesh_file_name_string.substr(last_dot + 1);
				std::string objName = mesh_file_name_string.substr(last_dot - 6, last_dot);

				if (extension == "off" || extension == "OFF")
				{
					Eigen::MatrixXd V;
					Eigen::MatrixXi F;
					if (!igl::readOFF(mesh_file_name_string, V, F))
						return false;


					data().set_mesh(V, F);

					data().set_face_based(true);

					data().dirty |= MeshGL::DIRTY_UV;

				}
				else if (extension == "obj" || extension == "OBJ")
				{
					Eigen::MatrixXd corner_normals;
					Eigen::MatrixXi fNormIndices;

					Eigen::MatrixXd UV_V;
					Eigen::MatrixXi UV_F;
					Eigen::MatrixXd V;
					Eigen::MatrixXi F;

					if (!(
						igl::readOBJ(
							mesh_file_name_string,
							V, UV_V, corner_normals, F, UV_F, fNormIndices)))
					{
						return false;
					}

					data().set_mesh(V, F);
					if (UV_V.rows() > 0)
					{
						data().set_uv(UV_V, UV_F);
					}


				}
				else
				{
					// unrecognized file type
					printf("Error: %s is not a recognized file type.\n", extension.c_str());
					return false;
				}

				data().compute_normals();
				//data().uniform_colors(Eigen::Vector3d(51.0 / 255.0, 43.0 / 255.0, 33.3 / 255.0),
				//	Eigen::Vector3d(255.0 / 255.0, 228.0 / 255.0, 58.0 / 255.0),
				//	Eigen::Vector3d(255.0 / 255.0, 235.0 / 255.0, 80.0 / 255.0));

				 //Alec: why?
				if (data().V_uv.rows() == 0)
				{
					data().grid_texture();
				}

				//initSimplification();
				//initCosts();
				//initTree();
				//initAxes();
				//initLinkAxes();
				data().point_size = 7.5;
				data().line_width = 1.5;


				//for (unsigned int i = 0; i<plugins.size(); ++i)
				//  if (plugins[i]->post_load())
				//    return true;

				return true;
			}

			IGL_INLINE bool Viewer::save_mesh_to_file(
				const std::string& mesh_file_name_string)
			{
				// first try to load it with a plugin
				//for (unsigned int i = 0; i<plugins.size(); ++i)
				//  if (plugins[i]->save(mesh_file_name_string))
				//    return true;

				size_t last_dot = mesh_file_name_string.rfind('.');
				if (last_dot == std::string::npos)
				{
					// No file type determined
					std::cerr << "Error: No file extension found in " <<
						mesh_file_name_string << std::endl;
					return false;
				}
				std::string extension = mesh_file_name_string.substr(last_dot + 1);
				if (extension == "off" || extension == "OFF")
				{
					return igl::writeOFF(
						mesh_file_name_string, data().V, data().F);
				}
				else if (extension == "obj" || extension == "OBJ")
				{
					Eigen::MatrixXd corner_normals;
					Eigen::MatrixXi fNormIndices;

					Eigen::MatrixXd UV_V;
					Eigen::MatrixXi UV_F;

					return igl::writeOBJ(mesh_file_name_string,
						data().V,
						data().F,
						corner_normals, fNormIndices, UV_V, UV_F);
				}
				else
				{
					// unrecognized file type
					printf("Error: %s is not a recognized file type.\n", extension.c_str());
					return false;
				}
				return true;
			}

			IGL_INLINE bool Viewer::load_scene()
			{
				std::string fname = igl::file_dialog_open();
				if (fname.length() == 0)
					return false;
				return load_scene(fname);
			}

			IGL_INLINE bool Viewer::load_scene(std::string fname)
			{
				// igl::deserialize(core(),"Core",fname.c_str());
				igl::deserialize(data(), "Data", fname.c_str());
				return true;
			}

			IGL_INLINE bool Viewer::save_scene()
			{
				std::string fname = igl::file_dialog_save();
				if (fname.length() == 0)
					return false;
				return save_scene(fname);
			}

			IGL_INLINE bool Viewer::save_scene(std::string fname)
			{
				//igl::serialize(core(),"Core",fname.c_str(),true);
				igl::serialize(data(), "Data", fname.c_str());

				return true;
			}

			IGL_INLINE void Viewer::open_dialog_load_mesh()
			{
				int savedIndx = selected_data_index;
				load_mesh_from_file("C:/Dev/EngineForAnimationCourse/tutorial/data/zcylinder.obj");
				data().add_points(Eigen::RowVector3d(0, 0, 0), Eigen::RowVector3d(0, 0, 1));
				data().show_overlay_depth = false;
				data().show_overlay = true;
				data().MyTranslate(Eigen::Vector3d(0, 0, (-1.6)), true);
				data().set_visible(false, 1);
				data().set_mesh(data().V, data().F);
				data().set_visible(true, 2);
				data().show_faces = 3;
				data().set_face_based(true);

				parents.push_back(data_list.size() - 2);
				data().parentId = data_list.size() - 2;

				data().SetCenter(Eigen::Vector3d(0, 0, 0.8));
				data().show_texture = 3;
				tipPositions.push_back(Eigen::Vector4d(0, 0, 0, 1));
				update_tips();
				selected_data_index = savedIndx;
				initLinkAxes();
				linksNum++;
			}

			IGL_INLINE void Viewer::open_dialog_save_mesh()
			{
				std::string fname = igl::file_dialog_save();

				if (fname.length() == 0)
					return;

				this->save_mesh_to_file(fname.c_str());
			}

			IGL_INLINE ViewerData& Viewer::data(int mesh_id /*= -1*/)
			{
				assert(!data_list.empty() && "data_list should never be empty");
				int index;
				if (mesh_id == -1)
					index = selected_data_index;
				else
					index = mesh_index(mesh_id);

				assert((index >= 0 && index < data_list.size()) &&
					"selected_data_index or mesh_id should be in bounds");
				return data_list[index];
			}

			IGL_INLINE const ViewerData& Viewer::data(int mesh_id /*= -1*/) const
			{
				assert(!data_list.empty() && "data_list should never be empty");
				int index;
				if (mesh_id == -1)
					index = selected_data_index;
				else
					index = mesh_index(mesh_id);

				assert((index >= 0 && index < data_list.size()) &&
					"selected_data_index or mesh_id should be in bounds");
				return data_list[index];
			}

			IGL_INLINE int Viewer::append_mesh(bool visible /*= true*/)
			{
				assert(data_list.size() >= 1);

				data_list.emplace_back();
				selected_data_index = data_list.size() - 1;
				data_list.back().id = next_data_id++;
				//if (visible)
				//    for (int i = 0; i < core_list.size(); i++)
				//        data_list.back().set_visible(true, core_list[i].id);
				//else
				//    data_list.back().is_visible = 0;
				return data_list.back().id;
			}

			IGL_INLINE bool Viewer::erase_mesh(const size_t index)
			{
				assert((index >= 0 && index < data_list.size()) && "index should be in bounds");
				assert(data_list.size() >= 1);
				if (data_list.size() == 1)
				{
					// Cannot remove last mesh
					return false;
				}
				data_list[index].meshgl.free();
				data_list.erase(data_list.begin() + index);
				if (selected_data_index >= index && selected_data_index > 0)
				{
					selected_data_index--;
				}

				return true;
			}

			IGL_INLINE size_t Viewer::mesh_index(const int id) const {
				for (size_t i = 0; i < data_list.size(); ++i)
				{
					if (data_list[i].id == id)
						return i;
				}
				return 0;
			}

			Eigen::Matrix4d Viewer::CalcParentsTrans(int indx)
			{
				Eigen::Matrix4d prevTrans = Eigen::Matrix4d::Identity();

				for (int i = indx; parents[i] >= 0; i = parents[i])
				{
					//std::cout << "parent matrix:\n" << data_list[parents[i]].MakeTransd() << std::endl;
					prevTrans = data_list[parents[i]].MakeTransd() * prevTrans;
				}

				return prevTrans;
			}

			//Assignment 3

			void Viewer::initAxes() {

				Eigen::MatrixXd V_box(6, 3);
				V_box <<
					1.6, 0, 0,
					-1.6, 0, 0,
					0, 1.6, 0,
					0, -1.6, 0,
					0, 0, 1.6,
					0, 0, -1.6;

				data().add_points(V_box, Eigen::RowVector3d(1, 0, 0));


				Eigen::MatrixXi E_box(3, 2);
				E_box <<
					0, 1,
					2, 3,
					4, 5;

				for (unsigned i = 0; i < E_box.rows(); ++i)
					data().add_edges
					(
						V_box.row(E_box(i, 0)),
						V_box.row(E_box(i, 1)),
						Eigen::RowVector3d(1, 0, 0)
					);
			}

			void Viewer::initLinkAxes() {

				Eigen::MatrixXd V_box(6, 3);
				V_box <<
					0.8, 0, -0.8,
					-0.8, 0, -0.8,
					0, 0.8, -0.8,
					0, -0.8, -0.8,
					0, 0, -0.8,
					0, 0, -2.4;

				data().add_points(V_box, Eigen::RowVector3d(0, 0, 1));


				Eigen::MatrixXi E_box(3, 2);
				E_box <<
					0, 1,
					2, 3,
					4, 5;

				for (unsigned i = 0; i < E_box.rows(); ++i)
					data().add_edges
					(
						V_box.row(E_box(i, 0)),
						V_box.row(E_box(i, 1)),
						Eigen::RowVector3d(0, 0, 1)
					);
			}


			void Viewer::update_tips() {
				Eigen::Vector3d c =data_list[1].GetCenter();
				Eigen::Vector4d center(c.x(), c.y(), c.z(), 1);
				Eigen::Matrix3d rot= Eigen::Matrix3d().Identity();

				Eigen::Vector4d O = data_list[1].MakeTransd()* center - Eigen::Vector4d(0, 0, 0.8, 0);
				tipPositions[0] = O;
				for (int i = 1; i < data_list.size(); i++) {
					rot = rot * data_list[i].GetRotation();
					Eigen::Vector3d tmp = (rot * Eigen::Vector3d(0, 0, -1.6));
					O = O + Eigen::Vector4d(tmp(0), tmp(1), tmp(2), 0);
					tipPositions[i] = O;
				}
			}

			void Viewer::update_destination() {
				destPos = (data_list[0].MakeTransd() * Eigen::Vector4d(0, 0, 0, 1));
			}


			void Viewer::updateLinksToTips(std::vector<Eigen::Vector4d> newPos) {
				double angle,A;
				Eigen::Vector4d currVec;
				Eigen::Vector4d newVec;
				Eigen::Vector4d rotationVector;
				for (int i = 0; i < linksNum; i++)
				{
					currVec = tipPositions[i + 1] - tipPositions[i];
					newVec = newPos[i + 1] - newPos[i];
					A = currVec.normalized().dot(newVec.normalized());
					if (A > 1) {
						A = 1;
					}
					if (A < -1) {
						A = -1;
					}
					angle = acos(A);
					rotationVector = currVec.cross3(newVec);
					data_list[i+1].MyRotate(((CalcParentsTrans(i+1) * data_list[i+1].MakeTransd()).inverse() * rotationVector).head(3), angle/10);
					update_tips();
				}

			}

			void Viewer::rotateAroundY(bool clockwise) {
				double direction_amt = 0;
				if (clockwise) {
					direction_amt = 0.1;
				}
				else {
					direction_amt = -0.1;
				}

				data().MyRotate(data().GetRotation().inverse() * Eigen::Vector3d(0, 0, 1), direction_amt);

				update_tips();
			}

			void Viewer::rotateAroundX(bool clockwise) {
				double direction_amt = 0;
				if (clockwise) {
					direction_amt = 0.1;
				}
				else {
					direction_amt = -0.1;
				}
				Eigen::Matrix3d rot = Eigen::Matrix3d::Identity();
				for (int i = 1; i < selected_data_index; i++)
					rot = data_list[i].GetRotation() * rot;
				data().MyRotate(data().GetRotation().inverse() * Eigen::Vector3d(1, 0, 0), direction_amt);
			}


			//calculate the euler matrix as seen in class
			Eigen::Matrix3d Viewer::get_euler_mat(Eigen::Vector3d angles) {
				double phi, theta, ksi;
				phi = angles.x();
				theta = angles.y();
				ksi = angles.z();
				Eigen::Matrix3d A1(3, 3);
				A1 <<
					1, 0, 0,
					0, cos(phi), -sin(phi),
					0, sin(phi), cos(phi);
				Eigen::Matrix3d A2(3, 3);
				A2 <<
					cos(theta), -sin(theta), 0,
					sin(theta), cos(theta), 0,
					0, 0, 1;
				Eigen::Matrix3d A3(3, 3);
				A3 <<
					1, 0, 0,
					0, cos(ksi), -sin(ksi),
					0, sin(ksi), cos(ksi);

				Eigen::Matrix3d A = A1 * A2 * A3;
				return A;
			}

			// Printing functions for Assignment3 
			void Viewer::print_tip_position() {
				for (size_t i = 0; i < tipPositions.size(); i++)
				{
					if (i != 0) {
						printf("tip position %d: (%f,%f,%f) \n", i, tipPositions[i](0), tipPositions[i](1), tipPositions[i](2));
					}
					else {
						printf("base position: (%f,%f,%f) \n", tipPositions[i](0), tipPositions[i](1), tipPositions[i](2));
					}
				}
			}

			void Viewer::print_rotation_mats() {
				Eigen::Matrix3d rotMat;
				if (isPicked || selected_data_index == 0) {
					printf("scene rotation matrix:\n");
					rotMat = GetRotation();
				}
				else {
					printf("link %d rotation matrix:\n", selected_data_index);
					rotMat = data().GetRotation();
				}
				for (int i = 0; i < 3; i++) {
					printf("(%f\t%f\t%f)\n", rotMat(i, 0), rotMat(i, 1), rotMat(i, 2));
				}
			}
			void Viewer::print_destination_position() {
				printf("destination position: (%f,%f,%f) \n", destPos(0), destPos(1), destPos(2));
			}
			//End printing functions Assignment3 

		} // end namespace
	} // end namespace
}
