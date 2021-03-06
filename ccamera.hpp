#ifndef CCAMERA_HPP
#define CCAMERA_HPP
#include "geometry.hpp"

class CCamera {
	private:
		CPoint3D m_position;
		CVector3D m_view;
	public:
		CCamera() : m_position(CPoint3D()), m_view(CVector3D()) {}
		CCamera(const CPoint3D& p, const CVector3D& v) : m_position(p), m_view(v) {
			m_view.normalize();
		}
		CCamera(const CPoint3D& p, const CPoint3D& v) : m_position(p), m_view(CVector3D(p, v)) {
			m_view.normalize();
		}

		CPoint3D get_position() const { return m_position; }
		CVector3D get_view() const { return m_view; }
		void set_position(const CPoint3D& p) { m_position = p; }
		void set_view(const CVector3D& v) { 
			m_view = v; 
			m_view.normalize();
		}
};
#endif //CCAMERA_HPP
