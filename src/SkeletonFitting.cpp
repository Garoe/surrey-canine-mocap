/*
 * SkeletonFitting.cpp
 *
 *  Created on: 5 Nov 2013
 *      Author: m04701
 */

#include "SkeletonFitting.h"
#include "DebugUtil.h"

SkeletonFitting::SkeletonFitting() :
			move_joint_max_dist(0) {
}

SkeletonFitting::~SkeletonFitting() {
}

//void SkeletonFitting::fit_skeleton_into_cloud(Skeleton& skeleton,
//		osg::ref_ptr<osg::Vec3Array> cloud) {

//TODO
//Select all points that are closer than a threshold to the line formed by
//a pair of joints, care with points far away from the joints, case the line
//cuts other points in the skeleton.

//How to get the points???
//Segment in boxes first???

//Call fitLine with those points and the line formed by the joints
//New joints shall be the closest points of the new line to the previous
//joints

//fitLine(InputArray points, OutputArray line, int distType, double param, double reps, double aeps)
//points – Input vector of 2D or 3D points, stored in std::vector<> or Mat.
//line – Output line parameters. In case of 2D fitting, it should be a vector of 4 elements (like Vec4f) - (vx, vy, x0, y0), where (vx, vy) is a normalized vector collinear to the line and (x0, y0) is a point on the line. In case of 3D fitting, it should be a vector of 6 elements (like Vec6f) - (vx, vy, vz, x0, y0, z0), where (vx, vy, vz) is a normalized vector collinear to the line and (x0, y0, z0) is a point on the line.
//distType – Distance used by the M-estimator (see the discussion below).
//param – Numerical parameter ( C ) for some types of distances. If it is 0, an optimal value is chosen.
//reps – Sufficient accuracy for the radius (distance between the coordinate origin and the line).
//aeps – Sufficient accuracy for the angle. 0.01 would be a good default value for reps and aeps.

//}

//void SkeletonFitting::fit_skeleton_with_prev_nex_frame(Skeleton& skeleton,
//	int frame) {
//TODO
//Calculate distance to joint in next and previous frame
//Calculate vectors of movement to positions in next and previous frame
//Add them
//If the are not 0 in any dimension, it means that if we move in that dimension
//a certain amount, then distance to prev and next should be reduced.
//Iterative??? How much to move???
//}

int SkeletonFitting::find_head(osg::ref_ptr<osg::Vec3Array> cloud) {
	divide_four_sections(cloud);

	float max_x = cloud->front().x();
	int index = 0;
	for (unsigned int i = 0; i < cloud->size(); i++) {
		if (labels[i] == Not_Limbs && max_x < cloud->at(i).x()) {
			max_x = cloud->at(i).x();
			index = i;
		}
	}
	return index;
}

int SkeletonFitting::find_front_right_paw(osg::ref_ptr<osg::Vec3Array> cloud) {
	std::vector<int> front_right;

	for (unsigned int i = 0; i < cloud->size(); i++) {
		if (labels[i] == Front_Right) {
			front_right.push_back(i);
		}
	}

	float max_y = cloud->at(front_right.front()).y();
	int index = front_right.front();
	for (unsigned int i = 0; i < front_right.size(); i++) {
		if (max_y < cloud->at(front_right[i]).y()) {
			max_y = cloud->at(front_right[i]).y();
			index = front_right[i];
		}
	}
	return index;
}

void SkeletonFitting::divide_four_sections(osg::ref_ptr<osg::Vec3Array> cloud,
		bool use_median) {
	labels.clear();
	labels.resize(cloud->size(), Front_Left);

	float mean_y;
	if (use_median) {
		mean_y = get_median(cloud, Front_Left, Y);
	} else {
		mean_y = get_mean(cloud, Front_Left, Y);
	}
	int num_invalid = 0;
	//Divide in half vertically, discard all values above
	for (unsigned int i = 0; i < cloud->size(); i++) {
		if (cloud->at(i).y() < mean_y) {
			labels[i] = Not_Limbs;
			num_invalid++;
		}
	}

	//Divide the remaining values in front/back part along x
	float mean_x;
	if (use_median) {
		mean_x = get_median(cloud, Front_Left, X);
	} else {
		mean_x = get_mean(cloud, Front_Left, X);
	}
	for (unsigned int i = 0; i < cloud->size(); i++) {
		if (labels[i] == Front_Left && cloud->at(i).x() <= mean_x) {
			labels[i] = Back_Left;
		}
	}

	//Divide the two groups into left and right
	float mean_z_front;
	float mean_z_back;
	if (use_median) {
		mean_z_front = get_median(cloud, Front_Left, Z);
		mean_z_back = get_median(cloud, Back_Left, Z);
	} else {
		mean_z_front = get_mean(cloud, Front_Left, Z);
		mean_z_back = get_mean(cloud, Back_Left, Z);
	}
	for (unsigned int i = 0; i < cloud->size(); i++) {
		if (labels[i] == Front_Left && cloud->at(i).z() < mean_z_front) {
			labels[i] = Front_Right;
		} else if (labels[i] == Back_Left && cloud->at(i).z() < mean_z_back) {
			labels[i] = Back_Right;
		}
	}
	/*//Kmeans does not gives good results, but leave code here in case it would
	 // be used later
	 int num_valid = cloud->size() - num_invalid;
	 int max_clusters = 4;
	 cv::Mat labels;

	 cv::Mat data2(num_valid, 1, CV_32FC3);
	 for(int i = 0; i < num_valid; i++) {
	 if(result[i] != Not_Use){
	 cv::Point3f ipt(cloud->at(i).x(),
	 cloud->at(i).y(), cloud->at(i).z() );
	 data2.at<cv::Point3f>(i) =ipt;
	 }
	 }

	 cv::kmeans(data2, max_clusters, labels,
	 cv::TermCriteria( CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 10, 1.0), 3,
	 cv::KMEANS_PP_CENTERS);

	 int j = 0;
	 for(int i = 0; i < num_valid; i++) {
	 if(result[i] != Not_Limbs){
	 result[i] = (Skel_Leg)labels.at<int>(j);
	 j++;
	 }
	 }*/
}

bool comp_x(const osg::Vec3& i, const osg::Vec3& j) {
	return (i.x() < j.x());
}

bool comp_y(const osg::Vec3& i, const osg::Vec3& j) {
	return (i.y() < j.y());
}

bool comp_z(const osg::Vec3& i, const osg::Vec3& j) {
	return (i.z() < j.z());
}

float SkeletonFitting::get_median(osg::ref_ptr<osg::Vec3Array> points,
		Skel_Leg use_label, Axis axis) {
	osg::ref_ptr<osg::Vec3Array> aux_vec = new osg::Vec3Array();

	for (unsigned int i = 0; i < points->size(); i++) {
		if (labels[i] == use_label) {
			aux_vec->push_back(points->at(i));
		}
	}

	osg::Vec3Array::iterator first = aux_vec->begin();
	osg::Vec3Array::iterator last = aux_vec->end();
	osg::Vec3Array::iterator middle = first + (last - first) / 2;

	switch (axis) {
	case X:
		std::nth_element(first, middle, last, comp_x);
		return middle->x();
	case Y:
		std::nth_element(first, middle, last, comp_y);
		return middle->y();
	case Z:
		std::nth_element(first, middle, last, comp_z);
		return middle->z();
	}
	return 0.0;
}

float SkeletonFitting::get_mean(osg::ref_ptr<osg::Vec3Array> points,
		Skel_Leg use_label, Axis axis) {

	float mean = 0.0;
	int num_valid = 0;
	for (unsigned int i = 0; i < points->size(); i++) {
		if (labels[i] == use_label) {
			mean += points->at(i)[axis];
			num_valid++;
		}
	}

	return mean / num_valid;
}

const std::vector<Skel_Leg>& SkeletonFitting::getLabels() const {
	return labels;
}
