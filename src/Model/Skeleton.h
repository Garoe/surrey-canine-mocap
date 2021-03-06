/*
 * SkeletonFitting.h
 *
 *  Created on: 1 Nov 2013
 *      Author: m04701
 */

#ifndef SKELETON_H_
#define SKELETON_H_

#include <osg/Vec3>
#include <osg/Group>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include <fstream>
#include <iostream>
#include <string>
#include "BVHFormat.h"

using std::cout;
using std::endl;

//TODO Skeleton should hide what internal structure it uses for the nodes
//should have methods to access children/parent and other attributes

class Skeleton: public BVHFormat {
public:
	Skeleton();
	virtual ~Skeleton();
	void rotate_joint(unsigned int index, const osg::Vec3& angle);
	void rotate_root_all_frames(const osg::Vec3& angle);
	void translate_root(const osg::Vec3& translation);
	void translate_root_all_frames(const osg::Vec3& translation);
	void change_bone_length(unsigned int index, const osg::Vec3& translation);
	void change_bone_length_all_frames(unsigned int index,
			const osg::Vec3& translation);

	//Rotate index and its parent bone, so that index end position
	//remains in the same place
	void rotate_two_bones_keep_end_pos(unsigned int index, float angle);

	//Do the rotation but also maintain index children positions
	void rotate_two_bones_keep_end_pos_aim(unsigned int index, float angle);

	//Toggle bone colour
	void toggle_color(int index);

	unsigned int get_num_bones();
	void save_to_file(std::string file_name);
	void load_from_file(std::string file_name);
	void set_current_frame(int frame_no);
	MocapHeader& get_header();

	//Get the index of the bone whose render transform matrix is node_transform
	int get_node_index(osg::MatrixTransform* node_transform);

	//True if a skeleton have been loaded
	bool isSkelLoaded() const;
	void reset_state();

	void get_matrices_for_rotate_keep_end_pos(unsigned int index,
			osg::Matrix& first_bone_old_rot, osg::Matrix& first_bone_old_trans,
			osg::Vec3& dir_vec);

	//Values correspond to paw Node indices in skeleton
	enum Skel_Leg {
		Front_Left = 5,
		Front_Right = 9,
		Back_Left = 14,
		Back_Right = 18,
		Not_Limbs = 0
	};

	enum Axis {
		X, Y, Z
	};
private:
	bool skel_loaded;
};

typedef boost::shared_ptr<Skeleton> SkeletonPtr;

#endif /* SKELETON_H_ */
