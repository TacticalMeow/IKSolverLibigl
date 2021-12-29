#include "tutorial/sandBox/sandBox.h"
#include "igl/edge_flaps.h"
#include "igl/collapse_edge.h"
#include "Eigen/dense"
#include <functional>
#include <math.h>

#define EPSILON 0.1f

SandBox::SandBox()
{


}

void SandBox::Init(const std::string& config)
{
	std::string item_name;
	std::ifstream nameFileout;
	doubletemp = 0;
	Xdir = 0;
	Ydir = 0;
	linksNum = 3;

	//Init sphere (destination position)
	load_mesh_from_file("E:/Repositories/Animation_igl_engine_build_assignment3.5/bin/tutorial/data/sphere.obj");
	parents.push_back(-1);
	data().add_points(Eigen::RowVector3d(0, 0, 0), Eigen::RowVector3d(0, 0, 1));
	data().show_overlay_depth = false;
	data().show_overlay = true;
	data().MyTranslate(Eigen::Vector3d(4.0f, 0, 0), true);
	data().set_visible(false, 1);
	data().set_face_based(true);



	initAxes();
	tipPositions.push_back(Eigen::Vector4d(0, 0, 0,1));
	for (int i = 0; i < linksNum; i++) {
		load_mesh_from_file("E:/Repositories/Animation_igl_engine_build_assignment3.5/bin/tutorial/data/zcylinder.obj");

		data().add_points(Eigen::RowVector3d(0, 0, 0), Eigen::RowVector3d(0, 0, 1));
		data().show_overlay_depth = false;
		data().show_overlay = true;
		if (i != 0) {
			data().MyTranslate(Eigen::Vector3d(0, 0, (-1.6)), true);
		}

		data().set_visible(false, 1);
		initLinkAxes();
		Eigen::Vector3d center = data().GetCenter();
		data().SetCenter(Eigen::Vector3d(0, 0, 0.8));


		//push the parents of this link into the parents vector (as long as its not the first link).
		if (i != 0) {
			parents.push_back(data_list.size() - 2);
			data().parentId = data_list.size() - 2;
		}
		else {
			parents.push_back(-1);
			data().parentId = -1;
			Eigen::MatrixXd V_box(1, 3);
			V_box <<
				center.x(), center.y(), center.z();

			data().add_points(V_box, Eigen::RowVector3d(1, 0, 0));
		}

		tipPositions.push_back(Eigen::Vector4d(0, 0, (i + 1) * 1.6f,1));
	}

	//After we translate everything to its place (links and the destination sphere) we update the positions of the joints and destination
	update_destination();
	update_tips();

}




SandBox::~SandBox()
{

}



/// <summary>
/// The FABRIK implementation, from the papers
/// </summary>
void SandBox::fabrik_impl() {
	double delta = 0.1;
	Eigen::Vector4d tipToDest = tipPositions[linksNum] - destPos;
	Eigen::Vector4d baseToTip;
	Eigen::Vector4d baseToDest;
	Eigen::Vector4d rotvec;
	double R, lambda;
	std::vector<Eigen::Vector4d> ftipPos(tipPositions);


	//checking destination is reachable
	baseToDest = tipPositions[0] - destPos; 
	if (baseToDest.norm() > linksNum * 1.6) {
		isActive = false;
		printf("couldn't reach destination position.\n");
		return;
	}

	ftipPos[linksNum] = destPos;

	for (int i = linksNum-1; i >= 0; i--)
	{
		R=(ftipPos[i + 1] - ftipPos[i]).norm();
		lambda = 1.6 / R;
		ftipPos[i] = (1 - lambda) * ftipPos[i + 1] + lambda * ftipPos[i];
	}

	ftipPos[0] = tipPositions[0];

	for (int i =0; i < linksNum; i++)
	{
		R = (ftipPos[i + 1] - ftipPos[i]).norm();
		lambda = 1.6 / R;
		ftipPos[i+1] = (1 - lambda) * ftipPos[i] + lambda * ftipPos[i + 1];
	}

	updateLinksToTips(ftipPos);

	tipToDest = tipPositions[linksNum] - destPos;
	if (tipToDest.norm() < delta)
		isActive = false;

}



/// <summary>
/// The cyclic coordinate descent implementation , as studyied in class
/// </summary>
void SandBox::ccd_impl() {
	Eigen::Vector4d tipToDest = tipPositions[linksNum] - destPos;
	Eigen::Vector4d baseToTip;
	Eigen::Vector4d baseToDest;
	Eigen::Vector4d rotationVector;
	double cosine,angle;

	//checking destination is reachable
	baseToDest = tipPositions[0] - destPos; 
	if (baseToDest.norm() > linksNum * 1.6) {
		isActive = false;
		printf("couldn't reach destination position.\n");
		return;
	}


	for (int i = linksNum; i > 0; i--)
	{
		baseToDest = destPos - tipPositions[i - 1];
		baseToTip = tipPositions[linksNum] - tipPositions[i - 1];
		cosine = baseToTip.normalized().dot(baseToDest.normalized());
		if (cosine > 1) {
			cosine = 1;
		}
		if (cosine < -1) {
			cosine = -1;
		}

		//get angle from cross product.
		angle = acos(cosine);

		rotationVector = baseToTip.cross3(baseToDest);
		//Rotate the current link according to its parents translation, its own translation and the rotation calculated in the CCD.
		// Then we update the tip positions.
		data_list[i].MyRotate(((CalcParentsTrans(i) * data_list[i].MakeTransd()).inverse() * rotationVector).head(3), angle / 10);
		update_tips();
	}

	tipToDest = tipPositions[linksNum] - destPos;

	//here we check if we reached the destination (aka distance between tip of last link 
	// and the destination is smaller than EPSILON
	if (tipToDest.norm() < EPSILON)
	{
		isActive = false;
		printf("distance reached : %f \n", tipToDest.norm());
	}

}

void SandBox::Animate()
{
	if (isActive)
	{
		ccd_impl();
	}
}





