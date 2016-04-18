#include "ckdtreecpu.h"

CVoxel::CVoxel(const std::vector<IObject3D*>& objects) {
	if(objects.size() == 0) {
		m_bottom = CPoint3D(-1.0, -1.0, -1.0);
		m_top = CPoint3D(1.0, 1.0, 1.0);
	}

	for(IObject3D* obj : objects) {
		CPoint3D tmin_p = obj->get_min_boundary_point();
		CPoint3D tmax_p = obj->get_max_boundary_point();

		m_bottom.set_x(std::min(m_bottom.get_x(), tmin_p.get_x())); 
		m_bottom.set_y(std::min(m_bottom.get_y(), tmin_p.get_y())); 
		m_bottom.set_z(std::min(m_bottom.get_z(), tmin_p.get_z())); 
		m_top.set_x(std::max(m_top.get_x(), tmax_p.get_x())); 
		m_top.set_y(std::max(m_top.get_y(), tmax_p.get_y())); 
		m_top.set_z(std::max(m_top.get_z(), tmax_p.get_z())); 
	}

	m_bottom -= 1.0;
	m_top += 1.0;
}

inline void CVoxel::split(EPlane plane, const CPoint3D& plane_coord, CVoxel& left_vox, CVoxel& right_vox) const {
	CPoint3D left_top, left_bottom, right_top, right_bottom;
	left_top = m_top;
	right_top = m_top;
	left_bottom = m_bottom;
	right_bottom = m_bottom;

	switch(plane) {
		case EPlane::XY:
			left_top.set_z(plane_coord.get_z());
			right_bottom.set_z(plane_coord.get_z());
			break;
		case EPlane::XZ:
			left_top.set_y(plane_coord.get_y());
			right_bottom.set_y(plane_coord.get_y());
		case EPlane::YZ:
			left_top.set_x(plane_coord.get_x());
			right_bottom.set_x(plane_coord.get_x());
		case EPlane::NONE:
			std::cerr << "[EE]: Unable to split voxel: no line provided." << std::endl;
			exit(1);
			break;
	}

	left_vox = CVoxel(left_bottom, left_top);
	right_vox = CVoxel(right_bottom, right_top);
}

inline bool CVoxel::contains(IObject3D* object) const {
	CPoint3D min_p = object->get_min_boundary_point();
	CPoint3D max_p = object->get_max_boundary_point();

	return !((max_p.get_x() < m_bottom.get_x()) 
			|| (max_p.get_y() < m_bottom.get_y()) 
			|| (max_p.get_z() < m_bottom.get_z()) 
			|| (min_p.get_x() > m_top.get_x()) 
			|| (min_p.get_y() > m_top.get_y()) 
			|| (min_p.get_z() > m_top.get_z())); 
			
}

inline int CVoxel::contained_elements(const std::vector<IObject3D*>& objects) const {
	int result = 0;
	for(IObject3D* obj : objects) 
		if(contains(obj)) ++result;
	return result;
}

inline bool CVoxel::contains_point(const CPoint3D& point) const {
	return ((m_bottom.get_x() < point.get_x() && m_top.get_x() > point.get_x()
				&& m_bottom.get_y() < point.get_y() && m_top.get_y() > point.get_y()
				&& m_bottom.get_z() < point.get_z() && m_top.get_z() > point.get_z()));
}

inline bool CVoxel::intersects_with_vector(const CVector3D& vector) const {
	if(contains_point(vector.get_begin())) return true;

	CPoint3D intersection;
	CPoint3D plane_coord;

	plane_coord.set_z(m_bottom.get_z());
	if(vector.intersects_with_plane(EPlane::XY, plane_coord, intersection) 
			&& (intersection.get_x() > m_bottom.get_x() && intersection.get_x() < m_top.get_x())
			&& (intersection.get_y() > m_bottom.get_y() && intersection.get_y() < m_top.get_y()))
		return true;
	plane_coord.set_z(m_top.get_z());
	if(vector.intersects_with_plane(EPlane::XY, plane_coord, intersection) 
			&& (intersection.get_x() > m_bottom.get_x() && intersection.get_x() < m_top.get_x())
			&& (intersection.get_y() > m_bottom.get_y() && intersection.get_y() < m_top.get_y()))
		return true;
	plane_coord.set_y(m_bottom.get_y());
	if(vector.intersects_with_plane(EPlane::XZ, plane_coord, intersection) 
			&& (intersection.get_x() > m_bottom.get_x() && intersection.get_x() < m_top.get_x())
			&& (intersection.get_z() > m_bottom.get_z() && intersection.get_z() < m_top.get_z()))
		return true;
	plane_coord.set_y(m_top.get_y());
	if(vector.intersects_with_plane(EPlane::XZ, plane_coord, intersection) 
			&& (intersection.get_x() > m_bottom.get_x() && intersection.get_x() < m_top.get_x())
			&& (intersection.get_z() > m_bottom.get_z() && intersection.get_z() < m_top.get_z()))
	plane_coord.set_z(m_bottom.get_x());
	if(vector.intersects_with_plane(EPlane::YZ, plane_coord, intersection) 
			&& (intersection.get_z() > m_bottom.get_z() && intersection.get_z() < m_top.get_z())
			&& (intersection.get_y() > m_bottom.get_y() && intersection.get_y() < m_top.get_y()))
		return true;
	plane_coord.set_z(m_top.get_x());
	if(vector.intersects_with_plane(EPlane::YZ, plane_coord, intersection) 
			&& (intersection.get_z() > m_bottom.get_z() && intersection.get_z() < m_top.get_z())
			&& (intersection.get_y() > m_bottom.get_y() && intersection.get_y() < m_top.get_y()))
		return true;
	return false;
}


double CKDNode::MinimizeSAH(const std::vector<IObject3D*>& obj, EPlane plane, double bestSAH, const CVoxel& voxel, 
		double Ssplit, double Snot_split, CPoint3D& plane_coord, EPlane& res_plane) const {
	const double hx = voxel.get_top().get_x() - voxel.get_bottom().get_x();
	const double hy = voxel.get_top().get_y() - voxel.get_bottom().get_y();
	const double hz = voxel.get_top().get_z() - voxel.get_bottom().get_z();

	CVoxel left_vox;
	CVoxel right_vox;
	double curSAH;
	double resSAH = bestSAH;

	for(int i = 1; i <= MAX_SPLITS; ++i) {
		double l = i / MAX_SPLITS;
		double r = 1 - l;
		
		CPoint3D cur_plane_coord(voxel.get_bottom().get_x() + l * hx,
				voxel.get_bottom().get_y() + l * hy,
				voxel.get_bottom().get_z() + l * hz);
		voxel.split(plane, cur_plane_coord, left_vox, right_vox);
		curSAH = (Ssplit + l * Snot_split) * left_vox.contained_elements(obj) 
			+ (Ssplit + r * Snot_split) * right_vox.contained_elements(obj)
			+ SPLIT_COST;

		if(curSAH < bestSAH) {
			resSAH = curSAH;
			res_plane = plane;
			plane_coord = cur_plane_coord;
		}
	}

	return resSAH;
}

inline void CKDNode::FindPlane(const std::vector<IObject3D*>& objects, const CVoxel& voxel,
		int depth, EPlane& plane, CPoint3D& plane_coord) {
	if((depth >= MAX_DEPTH) || (objects.size() <= OBJECTS_IN_LEAF)) {
		plane = EPlane::NONE;
		return;
	}

	const double hx = voxel.get_top().get_x() - voxel.get_bottom().get_x();
	const double hy = voxel.get_top().get_y() - voxel.get_bottom().get_y();
	const double hz = voxel.get_top().get_z() - voxel.get_bottom().get_z();
	double Sxy = hx * hy;
	double Sxz = hx * hz;
	double Syz = hy * hz;
	double Ssum = Sxy + Sxz + Syz;

	//normalization
	Sxy /= Ssum;
	Sxz /= Ssum;
	Syz /= Ssum;

	double bestSAH = objects.size();
	plane = EPlane::NONE;
	bestSAH = MinimizeSAH(objects, EPlane::XY, bestSAH, voxel, Sxy, Sxz + Syz, plane_coord, plane);
	bestSAH = MinimizeSAH(objects, EPlane::XZ, bestSAH, voxel, Sxz, Sxy + Syz, plane_coord, plane);
	bestSAH = MinimizeSAH(objects, EPlane::YZ, bestSAH, voxel, Syz, Sxz + Sxy, plane_coord, plane);
}

inline CKDNode* CKDNode::build(std::vector<IObject3D*>& objects, const CVoxel& voxel, int depth) {
	EPlane plane;
	CPoint3D plane_coord;
	FindPlane(objects, voxel, depth, plane, plane_coord);
	
	if(plane == EPlane::NONE)
		return MakeLeaf(objects);

	CVoxel left_vox;
	CVoxel right_vox;
	voxel.split(plane, plane_coord, left_vox, right_vox);
	CKDNode* left_node = build(objects, left_vox, depth + 1);
	CKDNode* right_node = build(objects, right_vox, depth + 1);
	CKDNode* node = new CKDNode(plane, plane_coord, left_node, right_node); 

	return node;
}

inline CKDNode* CKDNode::MakeLeaf(const std::vector<IObject3D*>& objects) {
	CKDNode* node = new CKDNode();
	if(objects.size()) m_objects = objects;
	return node;
}

bool CKDNode::find_intersection(const CVoxel& voxel, const CVector3D& vector,
		IObject3D* nearest_object, CPoint3D& nearest_intersect) {
	if(m_plane == EPlane::NONE) {
		if(m_objects.size()) {
			CPoint3D intersection;
			double min_dist = -1.0, cur_dist;
			for(IObject3D* obj : m_objects) {
				if(obj->intersect(vector, intersection) 
						&& voxel.contains_point(intersection)){
					CVector3D v(vector.get_begin(), intersection);
					cur_dist = v.length();
					if((min_dist - cur_dist < EPS && min_dist - cur_dist > 0) 
							|| (min_dist < 0)) {
						nearest_object = obj;
						nearest_intersect = intersection;
						min_dist = cur_dist;
					}
				}
			}

			if(min_dist >= 0) return true;
		}
		return false;
	}

	CVoxel front_vox;
	CVoxel back_vox;

	CKDNode* front_node;
	CKDNode* back_node;

	switch(m_plane) {
		case EPlane::XY:
			if(((m_coordinate.get_z() > voxel.get_bottom().get_z()) 
					&& (m_coordinate.get_z() > vector.get_begin().get_z()))
					|| ((m_coordinate.get_z() < voxel.get_bottom().get_z())
					&& (m_coordinate.get_z() < vector.get_begin().get_z()))) {
				front_node = m_left;
				back_node = m_right;
				voxel.split(m_plane, m_coordinate, front_vox, back_vox);
			}
			else {
				front_node = m_right;
				back_node = m_left;
				voxel.split(m_plane, m_coordinate, back_vox, front_vox);
			}
			break;

		case EPlane::XZ:
			if(((m_coordinate.get_y() > voxel.get_bottom().get_y()) 
					&& (m_coordinate.get_y() > vector.get_begin().get_y()))
					|| ((m_coordinate.get_y() < voxel.get_bottom().get_y())
					&& (m_coordinate.get_y() < vector.get_begin().get_y()))) {
				front_node = m_left;
				back_node = m_right;
				voxel.split(m_plane, m_coordinate, front_vox, back_vox);
			}
			else {
				front_node = m_right;
				back_node = m_left;
				voxel.split(m_plane, m_coordinate, back_vox, front_vox);
			}
			break;

		case EPlane::YZ:
			if(((m_coordinate.get_x() > voxel.get_bottom().get_x()) 
					&& (m_coordinate.get_x() > vector.get_begin().get_x()))
					|| ((m_coordinate.get_x() < voxel.get_bottom().get_x())
					&& (m_coordinate.get_x() < vector.get_begin().get_x()))) {
				front_node = m_left;
				back_node = m_right;
				voxel.split(m_plane, m_coordinate, front_vox, back_vox);
			}
			else {
				front_node = m_right;
				back_node = m_left;
				voxel.split(m_plane, m_coordinate, back_vox, front_vox);
			}
			break;

		case EPlane::NONE:
			std::cerr << "[EE]: No plane" << std::endl;
			exit(1);
			break;
	}

	if(front_vox.intersects_with_vector(vector)
		&& front_node->find_intersection(front_vox, vector, nearest_object, nearest_intersect))
		return true;

	return (back_vox.intersects_with_vector(vector) 
			&& back_node->find_intersection(back_vox, vector, nearest_object, nearest_intersect));
}

CKDTreeCPU::CKDTreeCPU(std::vector<IObject3D*>& objects) {
	m_bounding_box = CVoxel(objects);
	m_root->build(objects, m_bounding_box, 0);
}

bool CKDTreeCPU::find_intersection(const CVector3D& vector, IObject3D* nearest_object, CPoint3D& nearest_intersect) {
	return (m_bounding_box.intersects_with_vector(vector)
			&& m_root->find_intersection(m_bounding_box, vector, nearest_object, nearest_intersect));
}
