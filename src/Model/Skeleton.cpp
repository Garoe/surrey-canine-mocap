/*
 * SkeletonFitting.cpp
 *
 *  Created on: 1 Nov 2013
 *      Author: m04701
 */

#include "Skeleton.h"

const osg::Vec3 Skeleton::x_axis(1, 0, 0);
const osg::Vec3 Skeleton::y_axis(0, 1, 0);
const osg::Vec3 Skeleton::z_axis(0, 0, 1);

Skeleton::Skeleton() :
		skel_loaded(false) {
}

Skeleton::~Skeleton() {
}

void Skeleton::rotate_joint(unsigned int index, const osg::Vec3& angle) {

	//Rotation axis are x, y, z but after the current rotation
	osg::Vec3 c_x_axis = nodelist[index]->quat_arr.at(header.currentframe)
			* x_axis;
	osg::Vec3 c_y_axis = nodelist[index]->quat_arr.at(header.currentframe)
			* y_axis;
	osg::Vec3 c_z_axis = nodelist[index]->quat_arr.at(header.currentframe)
			* z_axis;

	osg::Quat new_rot(angle[0], c_x_axis, angle[1], c_y_axis, angle[2],
			c_z_axis);

	nodelist[index]->quat_arr.at(header.currentframe) =
			nodelist[index]->quat_arr.at(header.currentframe) * new_rot;
}

void Skeleton::rotate_root_all_frames(const osg::Vec3& angle) {
	std::vector<osg::Quat>::iterator i;
	for (i = root->quat_arr.begin(); i != root->quat_arr.end(); ++i) {
		osg::Vec3 x_axis(1, 0, 0), y_axis(0, 1, 0), z_axis(0, 0, 1);
		x_axis = (*i) * x_axis;
		y_axis = (*i) * y_axis;
		z_axis = (*i) * z_axis;

		osg::Quat new_rot(angle[0], x_axis, angle[1], y_axis, angle[2], z_axis);
		(*i) = (*i) * new_rot;
	}
}

void Skeleton::translate_root(const osg::Vec3& translation) {
	root->froset->at(header.currentframe) += translation;
}

void Skeleton::translate_root_all_frames(const osg::Vec3& translation) {
	root->offset += translation;
}

void Skeleton::change_bone_length(unsigned int index,
		const osg::Vec3& translation) {
	nodelist[index]->length += translation;
	for (unsigned int i = 0; i < nodelist[index]->get_num_children(); i++) {
		nodelist[index]->children[i]->froset->at(header.currentframe) +=
				translation;
	}
}

void Skeleton::change_bone_length_all_frames(unsigned int index,
		const osg::Vec3& translation) {

	//Bone length is easier along global axis
	osg::Quat inv_glob_rot = nodelist[index]->get_inv_global_rot(
			header.currentframe);

	osg::Vec3 trans_local_coor = inv_glob_rot * translation;
	//Inverse bone rotation and translate in world coordinates
	nodelist[index]->length += trans_local_coor;

	//Translate also all brothers to maintain skeleton connectivity
	for (unsigned int i = 0; i < nodelist[index]->get_num_children(); i++) {
		nodelist[index]->children[i]->offset += trans_local_coor;
	}
}

void Skeleton::rotate_two_bones_keep_end_pos(unsigned int index, float angle) {
	unsigned int prev_index = index - 1;

	if (prev_index < 0) {
		return;
	}

	Node* first_bone = nodelist.at(prev_index);
	Node* second_bone = nodelist.at(index);

	osg::Matrix m;
	second_bone->get_parent_to_bone_end_matrix(header.currentframe, m);

	//Calculate the end position relative to the first bone
	osg::Vec3 dir_vec = osg::Vec3() * m;

	dir_vec.normalize();

	//The rotation that maintains the end position is along the
	//calculated vector
	osg::Quat new_rot(angle, dir_vec);

	//Rotate first bone along the previous vector
	first_bone->quat_arr.at(header.currentframe) *= new_rot;
	return;
}

void Skeleton::rotate_two_bones_keep_end_pos_aim(unsigned int index,
		float angle) {
	unsigned int prev_index = index - 1;

	if (prev_index < 0) {
		return;
	}

	Node* first_bone = nodelist.at(prev_index);
	Node* second_bone = nodelist.at(index);

	osg::Matrix m;

	//TODO Axes calculation could be done only once and saved for next
	//bone rotations
	second_bone->get_parent_to_bone_end_matrix(header.currentframe, m);

	//Calculate the end position relative to the first bone
	osg::Vec3 dir_vec = osg::Vec3() * m;

	dir_vec.normalize();

	//The rotation that maintains the end position is along the
	//calculated vector
	osg::Quat new_rot(angle, dir_vec);

	//Rotate first bone along the previous vector
	first_bone->quat_arr.at(header.currentframe) *= new_rot;

	//The rotation vector has to be in child coordinate system
	osg::Vec3 new_vec = dir_vec * osg::Matrix::inverse(m);

	//Rotate all children in the opposite direction so they remain
	//in the same position
	new_rot = osg::Quat(-angle, new_vec);
	for (unsigned int i = 0; i < second_bone->get_num_children(); i++) {
		second_bone->children.at(i)->quat_arr.at(header.currentframe) *=
				new_rot;
	}

	return;
}

void Skeleton::save_to_file(std::string file_name) {
	NodeIte i;
	for (i = nodelist.begin(); i != nodelist.end(); ++i) {
		(*i)->update_euler_angles();
	}
	export_data(file_name.c_str());
}

void Skeleton::load_from_file(std::string file_name) {
	reset_state();
	import_data(file_name.c_str());

	//Instead of euler angles use quaternions for the rotations
	NodeIte i;
	for (i = nodelist.begin(); i != nodelist.end(); ++i) {
		(*i)->calculate_quats(header.euler);
	}

	//Make all bones length to be align with the x axis
	for (i = nodelist.begin(); i != nodelist.end(); ++i) {
		(*i)->set_x_rotation_along_bone_length();
	}

	//Make all bones y axis to be align with normal between current and
	//next bone plane
	for (i = nodelist.begin(); i != nodelist.end(); ++i) {
		(*i)->set_y_rotation_perpendicular_to_next_bone();
	}
	skel_loaded = true;
}

unsigned int Skeleton::get_num_bones() {
	return header.noofsegments;
}

void Skeleton::set_current_frame(int frame_no) {
	header.currentframe = frame_no;

	//Frame beyond vector
	if (frame_no >= (long) header.noofframes) {
		header.noofframes = frame_no + 1;
		for (NodeIte j = nodelist.begin(); j != nodelist.end(); ++j) {
			(*j)->resize_frame_no(header.noofframes);
		}
	}
}

MocapHeader& Skeleton::get_header() {
	return header;
}

void Skeleton::toggle_color(int index) {
	nodelist[index]->toggle_color();
}

int Skeleton::get_node_index(osg::MatrixTransform* node_transform) {
	for (unsigned int i = 0; i < nodelist.size(); i++) {
		if (nodelist[i]->osg_node == node_transform) {
			return i;
		}
	}
	return -1;
}

bool Skeleton::isSkelLoaded() const {
	return skel_loaded;
}

void Skeleton::reset_state() {
	BVHFormat::reset_state();
	skel_loaded = false;
}

void Skeleton::get_matrices_for_rotate_keep_end_pos(unsigned int index,
		osg::Matrix& first_bone_old_rot, osg::Matrix& first_bone_old_trans,
		osg::Vec3& dir_vec) {

	int prev_index = index - 1;
	Node* first_bone = get_node(prev_index);
	Node* second_bone = get_node(index);

	osg::Matrix m;
	second_bone->get_parent_to_bone_end_matrix(header.currentframe, m);

	//Calculate the end position relative to the first bone
	dir_vec = osg::Vec3() * m;

	dir_vec.normalize();

	first_bone_old_rot = osg::Matrix::rotate(
			first_bone->quat_arr.at(header.currentframe));
	first_bone_old_trans = osg::Matrix::translate(
			first_bone->offset + first_bone->froset->at(header.currentframe));
}