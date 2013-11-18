#include "RenderSkeletonization.h"

RenderSkeletonization::RenderSkeletonization() :
			display_merged(true) {
	joint_color = osg::Vec4(0.5f, 0.5f, 0.5f, 1.0); //Grey
	bone_color = osg::Vec4(0.0f, 0.0f, 1.0f, 1.0); //Blue
	selection_color = osg::Vec4(1.0f, 1.0f, 1.0f, 1.0); //White
	skel_created = false;
	text_created = false;
}

RenderSkeletonization::~RenderSkeletonization() {
	//dtor
}

void RenderSkeletonization::set_data(
		std::vector<boost::shared_ptr<RGBD_Camera> > camera_arr_,
		osg::ref_ptr<osg::Switch> skel_vis_switch_,
		osg::ref_ptr<osg::Switch> skel_fitting_switch_) {
	//Save arguments
	camera_arr = camera_arr_;
	skel_vis_switch = skel_vis_switch_;
	skel_fitting_switch = skel_fitting_switch_;
	skel_fitting_switch->setNewChildDefaultValue(true);

	//In case this is not first call, do a clean up
	skel_group_array.clear();
	skel_group2D_array.clear();
	skel_group3D_array.clear();
	skel_vis_switch->removeChildren(0, skel_vis_switch->getNumChildren());

	//Set up the basics nodes
	for (camVecIte i = camera_arr.begin(); i != camera_arr.end(); ++i) {
		//Create main skeleton group for this camera and save it in an vector
		osg::ref_ptr<osg::Group> skel_group = new osg::Group;
		skel_vis_switch->addChild(skel_group.get(), true);
		skel_group_array.push_back(skel_group.get());

		//Create 2D skeleton group for this camera, put it under skeleton group
		//and save it in an vector
		osg::ref_ptr<osg::Group> aux_group = new osg::Group;
		skel_group->addChild(aux_group.get());
		skel_group2D_array.push_back(aux_group.get());

		//Get this camera transformation
		osg::ref_ptr<osg::MatrixTransform> cam_transform =
				new osg::MatrixTransform;
		//The original camera node is not used to keep the scene graph simpler
		cam_transform->setMatrix((*i)->cam_pose_xform->getMatrix());

		//Add it as child of main skeleton group
		skel_group->addChild(cam_transform.get());

		//Create 3D skeleton group under the camera transformation
		aux_group = new osg::Group;
		cam_transform->addChild(aux_group.get());
		skel_group3D_array.push_back(aux_group.get());
	}

	merged_group = new osg::Group;
	skel_vis_switch->addChild(merged_group.get(), true);
}

void RenderSkeletonization::clean_scene() {
	clean_2d_skeletons();
	clean_3d_skeleon_cloud();
	clean_3d_merged_skeleon_cloud();
	clean_skeleton();
}

void RenderSkeletonization::clean_2d_skeletons() {
	for (unsigned int i = 0; i < skel_group2D_array.size(); i++) {
		skel_group2D_array[i]->removeChildren(0,
				skel_group2D_array[i]->getNumChildren());
	}
}

void RenderSkeletonization::clean_3d_skeleon_cloud() {
	for (unsigned int i = 0; i < skel_group3D_array.size(); i++) {
		skel_group3D_array[i]->removeChildren(0,
				skel_group3D_array[i]->getNumChildren());
	}
}

void RenderSkeletonization::clean_3d_merged_skeleon_cloud() {
	merged_group->removeChildren(0, merged_group->getNumChildren());
}

void RenderSkeletonization::clean_skeleton() {
	skel_fitting_switch->removeChildren(0,
			skel_fitting_switch->getNumChildren());

	skel_created = false;
	text_created = false;
}

void RenderSkeletonization::clean_text() {
	skel_fitting_switch->removeChildren(1, 1);
	text_created = false;
}

void RenderSkeletonization::display_3d_skeleon_cloud(int disp_frame_no,
		Skeletonization3D& skeleton) {
	osg::ref_ptr<osg::Geode> skel_geode;
	osg::ref_ptr<osg::Geometry> skel_geometry;
	osg::ref_ptr<osg::Vec3Array> vertices;

	//Draw a red cloud of boxes, where each box represents a small part of a bone
	for (unsigned int i = 0; i < camera_arr.size(); i++) {
		skel_geode = new osg::Geode;
		skel_geometry = new osg::Geometry;

		vertices = skeleton.get_simple_3d_projection(i, disp_frame_no);

		for (unsigned int j = 0; j < vertices->size(); j++) {
			osg::ref_ptr<osg::ShapeDrawable> shape1 = new osg::ShapeDrawable;
			shape1->setShape(
					new osg::Box((*vertices)[j], 0.005f, 0.005f, 0.005f));
			shape1->setColor(osg::Vec4(camera_arr[i]->getVisColour(), 1.0f));
			skel_geode->addDrawable(shape1.get());
		}

		skel_group3D_array[i]->addChild(skel_geode.get());
	}
}

void RenderSkeletonization::display_3d_merged_skeleon_cloud(int disp_frame_no,
		Skeletonization3D& skeleton) {

	osg::ref_ptr<osg::Vec3Array> vertices = skeleton.get_merged_3d_projection(
			disp_frame_no);

	osg::ref_ptr<osg::Geode> skel2d_geode;
	if (merged_group->getNumChildren()) {
		skel2d_geode = static_cast<osg::Geode*>(merged_group->getChild(0));
	} else {
		skel2d_geode = new osg::Geode;
		merged_group->addChild(skel2d_geode.get());
	}
	//Draw a blue cloud of squares, where each square represents a small part of a bone
	unsigned int useful_nodes, i = 0;
	unsigned int n_drawables = skel2d_geode->getNumDrawables();
	unsigned int n_vertices = vertices->size();
	useful_nodes = ((n_drawables > n_vertices) ? n_vertices : n_drawables);
	//If the geometry is created, only change its position
	for (; i < useful_nodes; i++) {
		osg::ShapeDrawable* box_shape =
				static_cast<osg::ShapeDrawable*>(skel2d_geode->getDrawable(i));
		osg::Box* box = static_cast<osg::Box*>(box_shape->getShape());
		box->setCenter(vertices->at(i));
	}

	//If the geometry is not created, then create a new one
	for (unsigned int j = i; j < n_vertices; j++) {
		osg::ref_ptr<osg::ShapeDrawable> box_shape = new osg::ShapeDrawable;
		//box_shape->setDataVariance( osg::Object::DYNAMIC );
		box_shape->setShape(
				new osg::Box(vertices->at(j), 0.005f, 0.005f, 0.005f));
		box_shape->setColor(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0)); //Red
		//This lines are used to tell OSG that the data in the Geometry is
		//changing every frame, so it has to update it
		box_shape->setUseDisplayList(false);
		box_shape->setUseVertexBufferObjects(true);
		skel2d_geode->addDrawable(box_shape.get());
	}

	//If the points to draw in this frame are less than in the previous remove
	//all the extra geometries.
	skel2d_geode->removeDrawables(n_vertices,
			skel2d_geode->getNumDrawables() - n_vertices);
}

void RenderSkeletonization::display_2d_skeletons(int disp_frame_no,
		Skeletonization3D& skeleton) {
	int rows = camera_arr[0]->get_d_rows();
	int cols = camera_arr[0]->get_d_cols();
	float ratio_x = cols / (float) rows;

	//Define a quad
	osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
	vertices->push_back(osg::Vec3(0.f, 0.f, 2.f));
	vertices->push_back(osg::Vec3(ratio_x, 0.f, 2.f));
	vertices->push_back(osg::Vec3(ratio_x, 1.f, 2.f));
	vertices->push_back(osg::Vec3(0.f, 1.f, 2.f));

	//Understanding the texture mapping
	//In a standard Y right Y up axis, quad was defined as
	// 3, 2
	// 0, 1
	//While texture mapping world for a quad in the same axis is defined as
	// [0,1] [1,1]
	// [0,0] [1,0]
	//Then the coordinates of the texture has to follow the order of the
	//definition of the quad
	osg::ref_ptr<osg::Vec2Array> tc = new osg::Vec2Array;
	tc->push_back(osg::Vec2(0.f, 0.f));
	tc->push_back(osg::Vec2(ratio_x, 0.f));
	tc->push_back(osg::Vec2(ratio_x, 1.f));
	tc->push_back(osg::Vec2(0.f, 1.f));

	osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
	normals->push_back(osg::Vec3(0.0f, 0.0f, -1.0f));
	osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
	colors->push_back(osg::Vec4(0.0f, 1.0f, 0.0f, 1.0)); //green

	osg::ref_ptr<osg::Geometry> quad = new osg::Geometry;
	quad->setVertexArray(vertices.get());
	quad->setNormalArray(normals.get());
	quad->setNormalBinding(osg::Geometry::BIND_OVERALL);
	quad->setColorArray(colors, osg::Array::BIND_OVERALL);
	quad->setTexCoordArray(0, tc.get());
	quad->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, 4));

	for (unsigned int i = 0; i < camera_arr.size(); i++) {
		const cv::Mat* cvImg = skeleton.get_2D_frame(i, disp_frame_no);

		osg::ref_ptr<osg::Image> osgImage = new osg::Image;
		osgImage->setImage(cvImg->cols, cvImg->rows, 3,
		GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_BYTE, cvImg->data,
				osg::Image::NO_DELETE);

		osg::ref_ptr<osg::Texture2D> tex = new osg::Texture2D;
		tex->setImage(osgImage.get());

		osg::ref_ptr<osg::Geode> skel2d_geode = new osg::Geode;
		skel2d_geode->addDrawable(quad.get());
		skel2d_geode->getOrCreateStateSet()->setTextureAttributeAndModes(0,
				tex.get());

		osg::ref_ptr<osg::MatrixTransform> trans_matrix =
				new osg::MatrixTransform;
		trans_matrix->setMatrix(
				osg::Matrix::translate(osg::Vec3(1.4f * i - 2.f, -0.5f, 0.f)));
		trans_matrix->addChild(skel2d_geode.get());

		skel_group2D_array[i]->addChild(trans_matrix.get());
	}
}

osg::ref_ptr<osg::MatrixTransform> RenderSkeletonization::create_sphere(
		osg::Vec4 color) {
	osg::ref_ptr<osg::Geode> geode = new osg::Geode;
	osg::ref_ptr<osg::ShapeDrawable> sphere_shape;
	sphere_shape = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(), 1.1f));
	sphere_shape->setColor(color);

	geode->getOrCreateStateSet()->setMode( GL_LIGHTING,
			osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);

	geode->addDrawable(sphere_shape);
	osg::ref_ptr<osg::MatrixTransform> sphere_trans = new osg::MatrixTransform;

	sphere_trans->addChild(geode.get());

	return sphere_trans;
}

void RenderSkeletonization::display_skeleton(Node* node, MocapHeader& header,
		int current_frame) {
	if (skel_created) {
		update_skeleton(node, header, current_frame);
	} else {
		create_skeleton(node, header, skel_fitting_switch, current_frame);
		skel_created = true;
	}
}

void RenderSkeletonization::create_skeleton(Node* node, MocapHeader& header,
		osg::Group *pAddToThisGroup, int current_frame) {

	osg::ref_ptr<osg::MatrixTransform> skel_transform = new osg::MatrixTransform;

	skel_transform->setMatrix(
			osg::Matrix::rotate(node->freuler->at(current_frame)[0],
					header.euler->at(0), node->freuler->at(current_frame)[1],
					header.euler->at(1), node->freuler->at(current_frame)[2],
					header.euler->at(2))

					* osg::Matrix::translate(
							node->offset + node->froset->at(current_frame)));

	osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
	colors->push_back(
			osg::Vec4(node->color[0], node->color[1], node->color[2], 1.0));

	osg::Vec3 bone_pos = osg::Vec3(node->length[0] * node->scale[current_frame],
			node->length[1] * node->scale[current_frame],
			node->length[2] * node->scale[current_frame]);

	create_cylinder(osg::Vec3(), bone_pos, 0.01f, bone_color,
			static_cast<osg::Group*>(skel_transform.get()));

	osg::ref_ptr<osg::MatrixTransform> selectionBox = create_sphere(
			node->color);
	selectionBox->setMatrix(osg::Matrix::scale(0.02, 0.02, 0.02));

	skel_transform->addChild(selectionBox.get());
	if (node->noofchildren() == 0) {
		selectionBox = create_sphere(node->color);
		selectionBox->setMatrix(
				osg::Matrix::scale(0.02, 0.02, 0.02)
						* osg::Matrix::translate(bone_pos));

		skel_transform->addChild(selectionBox.get());
	}

	pAddToThisGroup->addChild(skel_transform.get());

	for (unsigned int i = 0; i < node->noofchildren(); i++)
		create_skeleton(node->children[i].get(), header, skel_transform.get(),
				current_frame);

	node->osg_node = skel_transform.get();
}

void RenderSkeletonization::update_skeleton(Node* node, MocapHeader& header,
		int current_frame) {
	osg::ref_ptr<osg::MatrixTransform> skel_transform = node->osg_node;

	skel_transform->setMatrix(
			osg::Matrix::rotate(node->freuler->at(current_frame)[0],
					header.euler->at(0), node->freuler->at(current_frame)[1],
					header.euler->at(1), node->freuler->at(current_frame)[2],
					header.euler->at(2))

					* osg::Matrix::translate(
							node->offset + node->froset->at(current_frame)));

	osg::ref_ptr<osg::MatrixTransform> selectionBox =
			static_cast<osg::MatrixTransform*>(skel_transform->getChild(1));
	static_cast<osg::ShapeDrawable*>(static_cast<osg::Geode*>(selectionBox->getChild(
			0))->getDrawable(0))->setColor(node->color);

	skel_transform->addChild(selectionBox.get());
	if (node->noofchildren() == 0) {
		selectionBox =
				static_cast<osg::MatrixTransform*>(skel_transform->getChild(2));
		static_cast<osg::ShapeDrawable*>(static_cast<osg::Geode*>(selectionBox->getChild(
				0))->getDrawable(0))->setColor(node->color);
	}

	for (unsigned int i = 0; i < node->noofchildren(); i++)
		update_skeleton(node->children[i].get(), header, current_frame);
}

osg::MatrixTransform* RenderSkeletonization::obj_belong_skel(
		osg::MatrixTransform* selected_obj) {
	if (skel_fitting_switch->getNumChildren() > 0) {
		return obj_belong_skel(selected_obj,
				static_cast<osg::MatrixTransform*>(skel_fitting_switch->getChild(
						0)));
	} else {
		return NULL;
	}
}

osg::MatrixTransform* RenderSkeletonization::obj_belong_skel(
		osg::MatrixTransform* selected_obj,
		osg::MatrixTransform* current_node) {
	if (selected_obj == current_node) {
		return current_node;
	}

	for (unsigned int i = 0; i < current_node->getNumChildren(); i++) {
		osg::MatrixTransform* aux =
				dynamic_cast<osg::MatrixTransform*>(current_node->getChild(i));
		if (aux) {
			aux = obj_belong_skel(selected_obj, aux);
			if (aux) {
				return aux;
			}
		}
	}
	return NULL;
}

osgText::Text* RenderSkeletonization::create_text(const osg::Vec3& pos,
		const std::string& content, float size) {
	osg::ref_ptr<osgText::Font> g_font = osgText::readFontFile("FreeSans.ttf");

	osg::ref_ptr<osgText::Text> text = new osgText::Text;
	text->setFont(g_font.get());
	text->setCharacterSize(size);
	text->setAxisAlignment(osgText::TextBase::XY_PLANE);
	text->setPosition(pos);
	text->setColor(osg::Vec4(1.0, 0.0, 0.0, 1));
	text->setText(content);
	return text.release();
}

osg::Camera* RenderSkeletonization::create_hud_camera(double left, double right,
		double bottom, double top) {
	osg::ref_ptr<osg::Camera> camera = new osg::Camera;
	camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	camera->setClearMask( GL_DEPTH_BUFFER_BIT);
	camera->setRenderOrder(osg::Camera::POST_RENDER);
	camera->setAllowEventFocus(false);
	camera->setProjectionMatrix(osg::Matrix::ortho2D(left, right, bottom, top));
	camera->getOrCreateStateSet()->setMode(GL_LIGHTING,
			osg::StateAttribute::OFF);
	return camera.release();
}

void RenderSkeletonization::display_text(std::string text, osg::Vec3 pos) {
	if (text_created) {
		skel_edit_text->setText(text);
		skel_edit_text->setPosition(pos);
	} else {
		osg::ref_ptr<osg::Geode> text_geode = new osg::Geode;
		skel_edit_text = create_text(pos, text, 18.0f);
		skel_edit_text->setDataVariance(osg::Object::DYNAMIC);
		text_geode->addDrawable(skel_edit_text);
		osg::ref_ptr<osg::Camera> text_cam = create_hud_camera(0, 1280, 0, 720);
		text_cam->addChild(text_geode.get());
		skel_fitting_switch->addChild(text_cam);
		text_created = true;
	}
}

void RenderSkeletonization::create_cylinder(osg::Vec3 StartPoint,
		osg::Vec3 EndPoint, float radius, osg::Vec4 CylinderColor,
		osg::Group *pAddToThisGroup) {
	osg::Vec3 center;
	float height;

	osg::ref_ptr<osg::Cylinder> cylinder;
	osg::ref_ptr<osg::ShapeDrawable> cylinderDrawable;
	osg::ref_ptr<osg::Material> pMaterial;
	osg::ref_ptr<osg::Geode> geode;

	height = (StartPoint - EndPoint).length();
	center = osg::Vec3((StartPoint.x() + EndPoint.x()) / 2,
			(StartPoint.y() + EndPoint.y()) / 2,
			(StartPoint.z() + EndPoint.z()) / 2);

	// This is the default direction for the cylinders to face in OpenGL
	osg::Vec3 z = osg::Vec3(0, 0, 1);

	// Get diff between two points you want cylinder along
	osg::Vec3 p = (StartPoint - EndPoint);

	// Get CROSS product (the axis of rotation)
	osg::Vec3 t = z ^ p;

	// Get angle. length is magnitude of the vector
	double angle = acos((z * p) / p.length());

	//   Create a cylinder between the two points with the given radius
	cylinder = new osg::Cylinder(center, radius, height);
	cylinder->setRotation(osg::Quat(angle, osg::Vec3(t.x(), t.y(), t.z())));

	//   A geode to hold our cylinder
	geode = new osg::Geode;
	cylinderDrawable = new osg::ShapeDrawable(cylinder);
	geode->addDrawable(cylinderDrawable);

	//   Set the color of the cylinder that extends between the two points.
	pMaterial = new osg::Material;
	pMaterial->setDiffuse(osg::Material::FRONT, CylinderColor);
	geode->getOrCreateStateSet()->setAttribute(pMaterial,
			osg::StateAttribute::OVERRIDE);

	//   Add the cylinder between the two points to an existing group
	pAddToThisGroup->addChild(geode);
}
