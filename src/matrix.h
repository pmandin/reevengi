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

#ifndef MATRIX_H
#define MATRIX_H 1

void mtx_setIdentity(float m[4][4]);
void mtx_print(float m[4][4]);

void mtx_setProjection(float m[4][4],
	float angle, float aspect, float z_near, float z_far);

void mtx_setLookAt(float m[4][4],
	float x_from, float y_from, float z_from,
	float x_to, float y_to, float z_to,
	float x_up, float y_up, float z_up);

void mtx_mult(float m1[4][4],float m2[4][4], float result[4][4]);

/* Calculate clip planes for view frustum */
void mtx_calcFrustumClip(float frustum[4][4], float clip[6][4]);

#define CLIPPING_OUTSIDE 0
#define CLIPPING_INSIDE 1
#define CLIPPING_NEEDED 2

/* Check if points array are all in view frustum or not */
int mtx_clipCheck(float points[4][4], int num_points, float clip[6][4]);

/* Clip segment against one of view frustum planes */
void mtx_clipSegment(float points[4][4], int num_points, float clip[6][4]);

#endif /* MATRIX_H */
