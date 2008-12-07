/*
	Matrix calculations

	Copyright (C) 2008	Patrice Mandin

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <string.h>
#include <stdio.h>
#include <math.h>

#include "matrix.h"

/*--- Functions ---*/

/*
	m[col][row]
*/

void mtx_setIdentity(float m[4][4])
{
	memset(m, 0, sizeof(float)*4*4);
	m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0;
}

void mtx_print(float m[4][4])
{
	int i;

	for (i=0; i<4; i++) {
		printf("(%8.3f\t%8.3f\t%8.3f\t%8.3f)\n",
			m[0][i],m[1][i],m[2][i],m[3][i]);
	}
}

/*
	gluPerspective(angle, aspect, z_near, z_far);

	f = cotangent(fovy/2) = 1 / tan(fovy/2)

	   f
	------	0	0	0
	aspect

	0	f	0	0

	0	0	zf+zn	2*zf*zn
			-----   -------
			zn-zf	zn-zf

	0	0	-1	0
*/

void mtx_setProjection(float m[4][4], float angle, float aspect, float z_near, float z_far)
{
	float sine, cotangent, deltaZ;
	float radians = angle / 2 * M_PI / 180;

	deltaZ = z_far - z_near;
	sine = sin(radians);
	if ((deltaZ == 0) || (sine == 0) || (aspect == 0)) {
		return;
	}

	cotangent = cos(radians) / sine;

	memset(m, 0, sizeof(float)*4*4);
	m[0][0] = cotangent / aspect;
	m[1][1] = cotangent;
	m[2][2] = -(z_far + z_near) / deltaZ;
	m[2][3] = -1;
	m[3][2] = -2 * z_near * z_far / deltaZ;
	m[3][3] = 0;
}

/*
    	gluLookAt(x_from, y_from, z_from,
		x_to, y_to, z_to,
		x_up, y_up, z_up);

    Let E be the 3d column vector (eyeX, eyeY, eyeZ).
    Let C be the 3d column vector (centerX, centerY, centerZ).
    Let U be the 3d column vector (upX, upY, upZ).
    Compute L = C - E.
    Normalize L.
    Compute S = L x U.
    Normalize S.
    Compute U' = S x L.

M is the matrix whose columns are, in order:

    (S, 0), (U', 0), (-L, 0), (-E, 1)  (all column vectors)
*/

static void normalize(float v[4])
{
	float r;

	r = sqrt( v[0]*v[0] + v[1]*v[1] + v[2]*v[2] );
	if (r <= 1.0e-5) return;

	v[0] /= r;
	v[1] /= r;
	v[2] /= r;
	v[3] /= r;
}

static void cross(float v1[3], float v2[3], float result[3])
{
	result[0] = v1[1]*v2[2] - v1[2]*v2[1];
	result[1] = v1[2]*v2[0] - v1[0]*v2[2];
	result[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

void mtx_setLookAt(float m[4][4],
	float x_from, float y_from, float z_from,
	float x_to, float y_to, float z_to,
	float x_up, float y_up, float z_up)
{
	float forward[4], side[4], up[3];

	forward[0] = x_to - x_from;
	forward[1] = y_to - y_from;
	forward[2] = z_to - z_from;

	up[0] = x_up;
	up[1] = y_up;
	up[2] = z_up;

	normalize(forward);

	/* Side = forward x up */
	cross(forward, up, side);
	normalize(side);

	/* Recompute up as: up = side x forward */
	cross(side, forward, up);

	memset(m, 0, sizeof(float)*4*4);
	m[0][0] = side[0];
	m[1][0] = side[1];
	m[2][0] = side[2];

	m[0][1] = up[0];
	m[1][1] = up[1];
	m[2][1] = up[2];

	m[0][2] = -forward[0];
	m[1][2] = -forward[1];
	m[2][2] = -forward[2];

	m[3][3] = 1.0;
}

void mtx_setRotation(float m[4][4], float angle,
	float x, float y, float z)
{
	float s,c,l;
	float xx,yy,zz, xy,yz,zx, xs,ys,zs, one_c;

	mtx_setIdentity(m);

	l = sqrt(x*x+y*y+z*z);
	if (l <= 1.0e-5) {
		return;
	}
	x /= l;
	y /= l;
	z /= l;

	c = cos((angle * M_PI) / 180.0);
	s = sin((angle * M_PI) / 180.0);

	xx = x * x;
	yy = y * y;
	zz = z * z;
	xy = x * y;
	yz = y * z;
	zx = z * x;
	xs = x * s;
	ys = y * s;
	zs = z * s;
	one_c = 1.0F - c;

	m[0][0] = (one_c * xx) + c;
	m[1][0] = (one_c * xy) - zs;
	m[2][0] = (one_c * zx) + ys;
	m[0][1] = (one_c * xy) + zs;
	m[1][1] = (one_c * yy) + c;
	m[2][1] = (one_c * yz) - xs;
	m[0][2] = (one_c * zx) - ys;
	m[1][2] = (one_c * yz) + xs;
	m[2][2] = (one_c * zz) + c;
}

void mtx_mult(float m1[4][4],float m2[4][4], float result[4][4])
{
	int row,col;

	for (row=0; row<4; row++) {
		for (col=0; col<4; col++) {
			result[col][row] =
				m1[0][row]*m2[col][0]
				+ m1[1][row]*m2[col][1]
				+ m1[2][row]*m2[col][2]
				+ m1[3][row]*m2[col][3];
		}
	}
}

void mtx_calcFrustumClip(float frustum[4][4], float clip[6][4])
{
	/* right */
	clip[0][0] = frustum[0][3]-frustum[0][0];
	clip[0][1] = frustum[1][3]-frustum[1][0];
	clip[0][2] = frustum[2][3]-frustum[2][0];
	clip[0][3] = frustum[3][3]-frustum[3][0];
	normalize(clip[0]);

	/* left */
	clip[1][0] = frustum[0][3]+frustum[0][0];
	clip[1][1] = frustum[1][3]+frustum[1][0];
	clip[1][2] = frustum[2][3]+frustum[2][0];
	clip[1][3] = frustum[3][3]+frustum[3][0];
	normalize(clip[1]);

	/* top */
	clip[2][0] = frustum[0][3]-frustum[0][1];
	clip[2][1] = frustum[1][3]-frustum[1][1];
	clip[2][2] = frustum[2][3]-frustum[2][1];
	clip[2][3] = frustum[3][3]-frustum[3][1];
	normalize(clip[2]);

	/* bottom */
	clip[3][0] = frustum[0][3]+frustum[0][1];
	clip[3][1] = frustum[1][3]+frustum[1][1];
	clip[3][2] = frustum[2][3]+frustum[2][1];
	clip[3][3] = frustum[3][3]+frustum[3][1];
	normalize(clip[3]);

	/* far */
	clip[4][0] = frustum[0][3]-frustum[0][2];
	clip[4][1] = frustum[1][3]-frustum[1][2];
	clip[4][2] = frustum[2][3]-frustum[2][2];
	clip[4][3] = frustum[3][3]-frustum[3][2];
	normalize(clip[4]);

	/* near */
	clip[5][0] = frustum[0][3]+frustum[0][2];
	clip[5][1] = frustum[1][3]+frustum[1][2];
	clip[5][2] = frustum[2][3]+frustum[2][2];
	clip[5][3] = frustum[3][3]+frustum[3][2];
	normalize(clip[5]);
}

/* For each clip plane, check if all points are on same side, or not */
static float dotProductPlus(float point[4], float plane[4])
{
	return (point[0]*plane[0] + point[1]*plane[1] + point[2]*plane[2] + point[3]*plane[3]);
}

int mtx_clipCheck(float points[][4], int num_points, float clip[6][4])
{
	int i, result = CLIPPING_INSIDE;

	for (i=0; i<6; i++) {
		int j, num_outsides = 0;
		for (j=0; j<num_points; j++) {
			if (dotProductPlus(points[j], clip[i])<0) {
				++num_outsides;
			}
		}

		if (num_outsides==num_points) {
			/* All points outside of current clip plane */
			return CLIPPING_OUTSIDE;
		} else if (num_outsides>0) {
			/* At least one point outside, need clipping */
			result = CLIPPING_NEEDED;
		}
	}

	return result;
}

int mtx_clipSegment(float points[4][4], float clip[6][4])
{
	int i, result = CLIPPING_INSIDE;

	for (i=0; i<6; i++) {
		int j, num_outsides = 0, num_point_outside=-1;
		float num,den, u, x,y,z,w;

		for (j=0; j<2; j++) {
			if (dotProductPlus(points[j], clip[i])<0) {
				++num_outsides;
				if (num_point_outside<0) {
					num_point_outside=j;
				}
			}
		}

		if (num_outsides==2) {
			/* All points outside of current clip plane */
			return CLIPPING_OUTSIDE;
		} else if (num_outsides==0) {
			/* All points inside, check other planes */
			continue;
		}

		/* Ah, clip this segment against clip plane */
		num =	clip[i][0]*points[0][0]+
			clip[i][1]*points[0][1]+
			clip[i][2]*points[0][2]+
			clip[i][3]*points[0][3];
		den =	clip[i][0]*(points[0][0]-points[1][0])+
			clip[i][1]*(points[0][1]-points[1][1])+
			clip[i][2]*(points[0][2]-points[1][2])+
			clip[i][3]*(points[0][3]-points[1][3]);

		u = num/den;

		x = points[0][0]+u*(points[1][0]-points[0][0]);
		y = points[0][1]+u*(points[1][1]-points[0][1]);
		z = points[0][2]+u*(points[1][2]-points[0][2]);
		w = points[0][3]+u*(points[1][3]-points[0][3]);

		/* Replace point outside, with the one which is on the clip plane */
		points[num_point_outside][0] = x;
		points[num_point_outside][1] = y;
		points[num_point_outside][2] = z;
		points[num_point_outside][3] = w;

		result = CLIPPING_NEEDED;
	}

	return result;
}
