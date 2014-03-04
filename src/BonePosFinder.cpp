/*
 * BonePosFinder.cpp
 *
 *  Created on: 12 Feb 2014
 *      Author: m04701
 */

#include "BonePosFinder.h"

BonePosFinder::BonePosFinder() {
	// TODO Auto-generated constructor stub

}

int BonePosFinder::find_head(const osg::ref_ptr<osg::Vec3Array>& cloud,
		const std::vector<Skeleton::Skel_Leg>& labels) {
	int index = -1;

	if (cloud->size() > 4) {
		float max_x = -FLT_MAX;
		for (unsigned int i = 0; i < cloud->size(); i++) {
			if (labels[i] == Skeleton::Not_Limbs && max_x < cloud->at(i).x()) {
				max_x = cloud->at(i).x();
				index = i;
			}
		}
	}
	return index;
}

int BonePosFinder::find_paw(const osg::ref_ptr<osg::Vec3Array>& cloud,
		const std::vector<Skeleton::Skel_Leg>& labels, Skeleton::Skel_Leg leg,
		std::vector<int>& leg_points_index) {
	leg_points_index.clear();

	for (unsigned int i = 0; i < cloud->size(); i++) {
		if (labels[i] == leg) {
			leg_points_index.push_back(i);
		}
	}

	if (leg_points_index.size() > 0) {
		float max_y = cloud->at(leg_points_index.front()).y();
		int index = leg_points_index.front();
		for (unsigned int i = 0; i < leg_points_index.size(); i++) {
			if (max_y < cloud->at(leg_points_index[i]).y()) {
				max_y = cloud->at(leg_points_index[i]).y();
				index = leg_points_index[i];
			}
		}
		return index;
	} else {
		return -1;
	}
}

int BonePosFinder::find_paw_fast(const std::vector<int>& leg_points_index) {

	if (leg_points_index.size() > 0) {
		return 0;
	} else {
		return -1;
	}
}

int BonePosFinder::find_leg_upper_end(const osg::ref_ptr<osg::Vec3Array>& cloud,
		const std::vector<Skeleton::Skel_Leg>& labels, Skeleton::Skel_Leg leg,
		std::vector<int>& leg_points_index) {
	leg_points_index.clear();

	for (unsigned int i = 0; i < cloud->size(); i++) {
		if (labels[i] == leg) {
			leg_points_index.push_back(i);
		}
	}

	if (leg_points_index.size() > 0) {
		float min_y = cloud->at(leg_points_index.front()).y();
		int index = leg_points_index.front();
		for (unsigned int i = 0; i < leg_points_index.size(); i++) {
			if (min_y > cloud->at(leg_points_index[i]).y()) {
				min_y = cloud->at(leg_points_index[i]).y();
				index = leg_points_index[i];
			}
		}
		return index;
	} else {
		return -1;
	}
}

int BonePosFinder::find_leg_upper_end_fast(
		const std::vector<int>& leg_points_index) {

	if (leg_points_index.size() > 0) {
		return leg_points_index.size() - 1;
	} else {
		return -1;
	}
}

bool BonePosFinder::find_first_bone_end_pos(const cv::Mat& cam2_bin_img,
		float bone_length, constCamVecIte& cam, int current_frame,
		const osg::Vec3& root_pos, osg::Vec3& head_pos) {
	//TODO Could also do this by having a complete cloud point
	//of the dog and going up and left (-y, -x) until distance was
	//reached, in any case a complete cloud could be useful

	float3 root_pos3 = make_float3(root_pos.x(), root_pos.y(), root_pos.z());

	//TODO SUPER IMPORTANT DO NOT DO THIS ON CAMERA BASED, DO A SUPER
	//CAMERA A PROJECT ALL POINTS TO AXES, on find second bone too
	//To be able to do this, I have to take the binary images from each camera
	//project them to 3D, and them project the resulting cloud to 2D using the
	//projections methods in this class

	int row = 0, col = 0;

	float3 start_point = Projections::get_2d_projection(root_pos, cam);
	row = start_point.y;
	col = start_point.x;

	//The point could be projected on a position this camera does not have
	//information on. So find the closest point and continue from there
	if (cam2_bin_img.at<uchar>(row, col) != 255) {
		if (!PixelSearch::get_nearest_white_pixel(cam2_bin_img, row, col, row,
				col)) {
			cout << "Head error not recoverable" << endl;
			return false;
		}
	}

	bool not_bone_length = true;
	float3 aux_point;
	while (not_bone_length) {
		if (PixelSearch::get_top_left_white_pixel(cam2_bin_img, row, col, row,
				col)) {
			aux_point = Projections::get_3d_projection(row, col, cam,
					current_frame);
			float current_length = length(root_pos3 - aux_point);
			if (current_length >= bone_length) {
				not_bone_length = false;
			}
		} else {
			cout << "Error calculating first bone position" << endl;
			return false;
		}
	}
	head_pos.set(aux_point.x, aux_point.y, aux_point.z);

	refine_goal_position(head_pos, root_pos, bone_length);
	return true;
}

bool BonePosFinder::find_second_bone_end_pos(const cv::Mat& cam1_bin_img,
		float bone_length, constCamVecIte& cam, int current_frame,
		const osg::Vec3& head_pos, osg::Vec3& shoulder_pos) {

	float3 start_point = Projections::get_2d_projection(head_pos, cam);

	int row = start_point.y;
	int col = start_point.x;

	if (cam1_bin_img.at<uchar>(row, col) != 255) {
		if (!PixelSearch::get_nearest_white_pixel(cam1_bin_img, row, col, row,
				col)) {
			cout << "Could not locate head end position on second camera"
					<< endl;
			return false;
		}
	}

	bool not_bone_length = true;
	float3 aux_point, head3 = make_float3(head_pos.x(), head_pos.y(),
			head_pos.z());
	while (not_bone_length) {
		if (PixelSearch::get_top_left_white_pixel(cam1_bin_img, row, col, row,
				col)) {
			aux_point = Projections::get_3d_projection(row, col, cam,
					current_frame);
			float current_length = length(head3 - aux_point);
			if (current_length >= bone_length) {
				not_bone_length = false;
			}
		} else {
			if (!unstuck_go_down(cam1_bin_img, row, col, row, col)) {
				cout << "Error calculating second bone position" << endl;
				return false;
			}
		}
	}
	shoulder_pos.set(aux_point.x, aux_point.y, aux_point.z);

	refine_goal_position(shoulder_pos, head_pos, bone_length);
	return true;
}

bool BonePosFinder::find_vertebral_end_pos(const cv::Mat& cam1_bin_img,
		float bone_length, constCamVecIte& cam, int current_frame,
		const osg::Vec3& shoulder_pos, osg::Vec3& vertebral_end_pos) {

	float3 start_point = Projections::get_2d_projection(shoulder_pos, cam);

	int row = start_point.y;
	int col = start_point.x;

	if (cam1_bin_img.at<uchar>(row, col) != 255) {
		if (!PixelSearch::get_nearest_white_pixel(cam1_bin_img, row, col, row,
				col)) {
			cout << "Could not locate shoulder end position on second camera"
					<< endl;
			return false;
		}
	}

	bool not_bone_length = true;
	float3 aux_point, head3 = make_float3(shoulder_pos.x(), shoulder_pos.y(),
			shoulder_pos.z());
	while (not_bone_length) {
		if (PixelSearch::get_top_left_white_pixel(cam1_bin_img, row, col, row,
				col)) {
			aux_point = Projections::get_3d_projection(row, col, cam,
					current_frame);
			float current_length = length(head3 - aux_point);
			if (current_length >= bone_length) {
				not_bone_length = false;
			}
		} else {
			if (!unstuck_go_down(cam1_bin_img, row, col, row, col)) {
				cout << "Error calculating third bone position" << endl;
				return false;
			}
		}
	}
	vertebral_end_pos.set(aux_point.x, aux_point.y, aux_point.z);

	refine_goal_position(vertebral_end_pos, shoulder_pos, bone_length);
	return true;
}

bool BonePosFinder::unstuck_go_down(const cv::Mat& img, int i_row, int i_col,
		int &res_row, int &res_col) {
	i_col--;
	if (i_col < 0) {
		return false;
	}

	while (i_row < img.rows && (int) img.at<uchar>(i_row, i_col) != 255) {
		i_row++;
	}

	if (i_row < img.rows && (int) img.at<uchar>(i_row, i_col) == 255) {
		res_row = i_row;
		res_col = i_col;
		return true;
	}

	return false;
}

void BonePosFinder::get_y_z_front_projection(
		const osg::ref_ptr<osg::Vec3Array>& cloud,
		const std::vector<Skeleton::Skel_Leg>& labels, Skeleton::Skel_Leg leg,
		cv::Mat& out_img, const osg::Vec3& trans) {
	//We want a front view so in world axis is vectors
	//x = [0,0,1]
	//y = [0,1,0]
	//z = [-1,0,0]
	//For the projection position the head position is used,
	//but for the legs to be the centre of the projection
	//an offset is needed
	float4x4 invT(0.0);
	invT[0 * 4 + 2] = -1;
	invT[1 * 4 + 1] = 1;
	invT[2 * 4 + 0] = 1;
	invT[3 * 4 + 0] = trans.z() + 0.05;
	invT[3 * 4 + 1] = trans.y() + 0.0;
	invT[3 * 4 + 2] = -(trans.x() - 0.3);
	invT[3 * 4 + 3] = 1;

	for (unsigned int i = 0; i < cloud->size(); i++) {
		if (labels[i] == leg) {
			float3 point2d = Projections::get_2d_projection(cloud->at(i), invT);
			if (point2d.y >= 0 && point2d.y < out_img.rows && point2d.x >= 0
					&& point2d.x < out_img.cols)
				out_img.at<uchar>(point2d.y, point2d.x) = 255;
		}
	}
}

void BonePosFinder::get_x_y_side_projection(
		const osg::ref_ptr<osg::Vec3Array>& cloud,
		const std::vector<Skeleton::Skel_Leg>& labels, Skeleton::Skel_Leg leg,
		cv::Mat& out_img, const osg::Vec3& trans) {
	//We want a side view, so no rotation is needed
	//x = [1,0,0]
	//y = [0,1,0]
	//z = [0,0,1]
	//For the projection position the head position is used,
	//but for the legs to be the centre of the projection
	//an offset is needed
	float4x4 invT(0.0);
	invT[0 * 4 + 0] = 1;
	invT[1 * 4 + 1] = 1;
	invT[2 * 4 + 2] = 1;
	invT[3 * 4 + 0] = trans.x() + 0.5;
	invT[3 * 4 + 1] = trans.y() - 0.05;
	invT[3 * 4 + 2] = trans.z() + 1.5;
	invT[3 * 4 + 3] = 1;

	for (unsigned int i = 0; i < cloud->size(); i++) {
		if (labels[i] == leg) {
			float3 point2d = Projections::get_2d_projection(cloud->at(i), invT);
			if (point2d.y >= 0 && point2d.y < out_img.rows && point2d.x >= 0
					&& point2d.x < out_img.cols)
				out_img.at<uchar>(point2d.y, point2d.x) = 255;
		}
	}
}

int BonePosFinder::find_leg_lower_3_joints_simple(
		const osg::ref_ptr<osg::Vec3Array>& cloud,
		const std::vector<int>& leg_points_index, const float bone_lengths[3],
		osg::Vec3 bone_positions[2]) {

	if (leg_points_index.size() == 0) {
		return 0;
	}

	unsigned int num_joints = 3;
	//Start at paw position
	bone_positions[0] = cloud->at(leg_points_index[0]);

	unsigned int j = 0;
	unsigned int i = 1, valid_pos = 1;
	bool continue_shearch = true;
	osg::Vec3 bone_start_pos = bone_positions[0];
	int bone_pos_index[3] = { 0, 0, 0 };
	//For each bone
	while (i < num_joints && continue_shearch) {

		bool not_bone_length = true;
		int index;
		//Go up in the cloud until we reach bone length
		//or there are no more points in the cloud
		while (not_bone_length && j < leg_points_index.size()) {
			index = leg_points_index[j];

			float current_length = (bone_start_pos - cloud->at(index)).length();
			if (current_length >= bone_lengths[i - 1]) {
				not_bone_length = false;
			} else {
				j++;
			}
		}

		if (not_bone_length == false) {
			if (i < 3) {
				bone_start_pos = cloud->at(index);
				//Make sure position is exactly at bone length
				refine_start_position(bone_start_pos, bone_positions[i - 1],
						bone_lengths[i - 1]);

				bone_positions[i] = bone_start_pos;
				bone_pos_index[i] = j;
				valid_pos++;
			} else {
				//All bones positions have been found
				continue_shearch = false;
			}
		} else {
			//We run out of points
			continue_shearch = false;
		}
		i++;
	}

	if (valid_pos == 2) {
		//Set temp position as the mean of all the points above the bone
		//end position
		int num_points = 0;
		for (unsigned int i = bone_pos_index[1]; i < leg_points_index.size();
				i++) {
			bone_positions[2] += cloud->at(leg_points_index[i]);
			num_points++;
		}
		bone_positions[2] = bone_positions[2] / num_points;

		//Use the vector from the mean to next bone start position
		//to project a plausible position for this bone
		refine_start_position(bone_positions[2], bone_positions[1],
				bone_lengths[1]);
		valid_pos++;
	}

	return valid_pos;
}

void BonePosFinder::refine_goal_position(osg::Vec3& end_position,
		const osg::Vec3& base_position, float length) {
	//Recalculate bone goal position using its length so we are sure it can
	//be reached
	osg::Vec3 pos_direction = (end_position - base_position);
	pos_direction.normalize();
	end_position = base_position + pos_direction * length;
}

void BonePosFinder::refine_start_position(osg::Vec3& start_position,
		const osg::Vec3& end_position, float length) {
	//Recalculate bone start position using its length so we are sure it can
	//be reached
	osg::Vec3 pos_direction = (start_position - end_position);
	pos_direction.normalize();
	start_position = end_position + pos_direction * length;
}
