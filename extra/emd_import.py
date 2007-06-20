#!BPY

"""
Name: 'Resident Evil EMD...'
Blender: 241
Group: 'Import'
Tip: 'Import a Resident Evil EMD file'
"""

__author__ = "Patrice Mandin"
__version__ = "0.1"
__bpydoc__ = """\
This script imports Resident Evil EMD files into Blender.
No normals support.
No texture support.

Usage:

Run this script from "File->Import" and select the desired EMD file.
"""

# Copyright (C) 2007: Patrice Mandin, pmandin@caramail.com
# based on Stanford PLY import script
# Copyright (C) 2004, 2005: Bruce Merry, bmerry@cs.uct.ac.za
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


# Portions of this code are taken from mod_meshtools.py in Blender
# 2.32.

import Blender, meshtools
import struct, StringIO
from Blender import NMesh

gRelPos = []

gTexturePalettes = []
gTextureData = []
gTextureBpp = 0
gTextureWidth = 0
gTextureHeight = 0

def readMeshRelativePosition(file, start_offset):
	global gRelPos

	offset1 = struct.unpack("<H",file.read(2))[0]
	offset2 = struct.unpack("<H",file.read(2))[0]
	count = struct.unpack("<H",file.read(2))[0]
	size = struct.unpack("<H",file.read(2))[0]

	if offset2>8:
		# Read relative positions
		rel1_pos = []
		for i in range(count):
			relx = float(struct.unpack("<h",file.read(2))[0]) / 100.0
			rely = float(struct.unpack("<h",file.read(2))[0]) / 100.0
			relz = float(struct.unpack("<h",file.read(2))[0]) / 100.0
			rel1_pos.append([relx,rely,relz])

		gRelPos = rel1_pos

def add_mesh(file, start_offset, num_mesh):
	global gRelPos

	rx = (num_mesh-len(gRelPos)) * 10.0
	ry = 0
	rz = 0
	if num_mesh<len(gRelPos):
		rx = gRelPos[num_mesh][0]
		ry = gRelPos[num_mesh][1]
		rz = gRelPos[num_mesh][2]

	tri_vtx_offset = start_offset + struct.unpack("<L",file.read(4))[0]
	tri_vtx_count = struct.unpack("<L",file.read(4))[0]
	tri_nor_offset = start_offset + struct.unpack("<L",file.read(4))[0]
	tri_nor_count = struct.unpack("<L",file.read(4))[0]
	tri_offset = start_offset + struct.unpack("<L",file.read(4))[0]
	tri_count = struct.unpack("<L",file.read(4))[0]
	tri_tex_offset = start_offset + struct.unpack("<L",file.read(4))[0]

	quad_vtx_offset = start_offset + struct.unpack("<L",file.read(4))[0]
	quad_vtx_count = struct.unpack("<L",file.read(4))[0]
	quad_nor_offset = start_offset + struct.unpack("<L",file.read(4))[0]
	quad_nor_count = struct.unpack("<L",file.read(4))[0]
	quad_offset = start_offset + struct.unpack("<L",file.read(4))[0]
	quad_count = struct.unpack("<L",file.read(4))[0]
	quad_tex_offset = start_offset + struct.unpack("<L",file.read(4))[0]

        mesh = Blender.NMesh.GetRaw()

	if tri_count>0:
		file.seek(tri_vtx_offset)
		for i in range(tri_vtx_count):
			#print "Reading triangle vertex %d" % i
			x = float(struct.unpack("<h",file.read(2))[0]) / 100.0
			x += rx
			y = float(struct.unpack("<h",file.read(2))[0]) / 100.0
			y += ry
			z = float(struct.unpack("<h",file.read(2))[0]) / 100.0
			z += rz
			w = float(struct.unpack("<h",file.read(2))[0]) / 100.0
			mesh.verts.append(NMesh.Vert(x,y,z))

		# nor_list = []
		# file.seek(tri_nor_offset)
		# for i in range(tri_nor_count):
		# 	x = float(struct.unpack("<H",file.read(2))[0]) / 4096.0
		# 	y = float(struct.unpack("<H",file.read(2))[0]) / 4096.0
		# 	z = float(struct.unpack("<H",file.read(2))[0]) / 4096.0
		# 	w = float(struct.unpack("<H",file.read(2))[0]) / 4096.0
		# 	nor_list.append(x,y,z)

		# Read uv info
		uv_list = []
		file.seek(tri_tex_offset)
		for i in range(tri_count):
			# often, num_palette = offset, so don't bother
			u0 = struct.unpack("B",file.read(1))[0]
			v0 = struct.unpack("B",file.read(1))[0]
			num_palette = struct.unpack("<H",file.read(2))[0] & 0x1f
			u1 = struct.unpack("B",file.read(1))[0]
			v1 = struct.unpack("B",file.read(1))[0]
			offset = struct.unpack("<H",file.read(2))[0] & 0x3f
			offset <<= 6
			u2 = struct.unpack("B",file.read(1))[0]
			v2 = struct.unpack("B",file.read(1))[0]
			unknown = struct.unpack("<H",file.read(2))[0]

			u0 = float(u0+offset) / float(gTextureWidth)
			u1 = float(u1+offset) / float(gTextureWidth)
			u2 = float(u2+offset) / float(gTextureWidth)
			v0 = float(v0) / float(gTextureHeight)
			v1 = float(v1) / float(gTextureHeight)
			v2 = float(v2) / float(gTextureHeight)

			uv_list.append([u0,v0])
			uv_list.append([u1,v1])
			uv_list.append([u2,v2])

		file.seek(tri_offset)
		for i in range(tri_count):
			#print "Reading triangle face %d" % i
			n0 = struct.unpack("<H",file.read(2))[0]
			v0 = struct.unpack("<H",file.read(2))[0]
			n1 = struct.unpack("<H",file.read(2))[0]
			v1 = struct.unpack("<H",file.read(2))[0]
			n2 = struct.unpack("<H",file.read(2))[0]
			v2 = struct.unpack("<H",file.read(2))[0]
			f=NMesh.Face()
			f.uv.append(uv_list[i*3])
			f.uv.append(uv_list[i*3+1])
			f.uv.append(uv_list[i*3+2])
			f.v.append(mesh.verts[v0])
			f.v.append(mesh.verts[v1])
			f.v.append(mesh.verts[v2])
			mesh.faces.append(f)

	if quad_count>0:
		start_vtx_count = 0
		if (quad_vtx_offset != tri_vtx_offset) or (quad_vtx_count != tri_vtx_count):
			start_vtx_count = tri_vtx_count
			file.seek(quad_vtx_offset)
			for i in range(quad_vtx_count):
				#print "Reading quad vertex %d" % i
				x = float(struct.unpack("<h",file.read(2))[0]) / 100.0
				x += rx
				y = float(struct.unpack("<h",file.read(2))[0]) / 100.0
				y += ry
				z = float(struct.unpack("<h",file.read(2))[0]) / 100.0
				z += rz
				w = float(struct.unpack("<h",file.read(2))[0]) / 100.0
				mesh.verts.append(NMesh.Vert(x,y,z))

		# nor_list = []
		# if (quad_nor_offset != tri_nor_offset) or (quad_nor_count != tri_nor_count):
		# 	file.seek(quad_nor_offset)
		# 		for i in range(tri_quad_count):
		# 			x = float(struct.unpack("<H",file.read(2))[0]) / 4096.0
		# 			y = float(struct.unpack("<H",file.read(2))[0]) / 4096.0
		# 			z = float(struct.unpack("<H",file.read(2))[0]) / 4096.0
		# 			w = float(struct.unpack("<H",file.read(2))[0]) / 4096.0
		# 			nor_list.append(x,y,z)

		# Read uv info
		uv_list = []
		file.seek(quad_tex_offset)
		for i in range(quad_count):
			# often, num_palette = offset, so don't bother
			u0 = struct.unpack("B",file.read(1))[0]
			v0 = struct.unpack("B",file.read(1))[0]
			num_palette = struct.unpack("<H",file.read(2))[0] & 0x1f
			u1 = struct.unpack("B",file.read(1))[0]
			v1 = struct.unpack("B",file.read(1))[0]
			offset = struct.unpack("<H",file.read(2))[0] & 0x3f
			offset <<= 6
			u2 = struct.unpack("B",file.read(1))[0]
			v2 = struct.unpack("B",file.read(1))[0]
			unknown = struct.unpack("<H",file.read(2))[0]
			u3 = struct.unpack("B",file.read(1))[0]
			v3 = struct.unpack("B",file.read(1))[0]
			unknown = struct.unpack("<H",file.read(2))[0]

			u0 = float(u0+offset) / float(gTextureWidth)
			u1 = float(u1+offset) / float(gTextureWidth)
			u2 = float(u2+offset) / float(gTextureWidth)
			u3 = float(u3+offset) / float(gTextureWidth)
			v0 = float(v0) / float(gTextureHeight)
			v1 = float(v1) / float(gTextureHeight)
			v2 = float(v2) / float(gTextureHeight)
			v3 = float(v3) / float(gTextureHeight)

			uv_list.append([u0,v0])
			uv_list.append([u1,v1])
			uv_list.append([u2,v2])
			uv_list.append([u3,v3])

		file.seek(quad_offset)
		for i in range(quad_count):
			#print "Reading quad face %d" % i
			n0 = struct.unpack("<H",file.read(2))[0]
			v0 = struct.unpack("<H",file.read(2))[0]
			n1 = struct.unpack("<H",file.read(2))[0]
			v1 = struct.unpack("<H",file.read(2))[0]
			n2 = struct.unpack("<H",file.read(2))[0]
			v2 = struct.unpack("<H",file.read(2))[0]
			n3 = struct.unpack("<H",file.read(2))[0]
			v3 = struct.unpack("<H",file.read(2))[0]
			f=NMesh.Face()
			f.uv.append(uv_list[i*4])
			f.uv.append(uv_list[i*4+1])
			f.uv.append(uv_list[i*4+3])
			f.uv.append(uv_list[i*4+2])
			f.v.append(mesh.verts[start_vtx_count+v0])
			f.v.append(mesh.verts[start_vtx_count+v1])
			f.v.append(mesh.verts[start_vtx_count+v3])
			f.v.append(mesh.verts[start_vtx_count+v2])
			mesh.faces.append(f)

	return mesh

def texture_read(filename):
	global gTexturePalettes
	global gTextureData
	global gTextureBpp
	global gTextureWidth
	global gTextureHeight

	try:
		file = open(filename, "rb")

		tim_id = struct.unpack("<L",file.read(4))[0]
		if (tim_id != 0x10):
			print "%s not a TIM file" % filename
			file.close()
			return 0

		tim_type = struct.unpack("<L",file.read(4))[0]
		tim_offset = struct.unpack("<L",file.read(4))[0]
		file.seek(4, 1)
		num_colors = struct.unpack("<H",file.read(2))[0]
		num_palettes = struct.unpack("<H",file.read(2))[0]

		gTextureBpp = 16
		if tim_type == 0x08:
			gTextureBpp = 4
		if tim_type == 0x09:
			gTextureBpp = 8

		if gTextureBpp<16:
			for i in range(num_palettes):
				palette = []
				for j in range(num_colors):
					value = struct.unpack("<H",file.read(2))[0]
					r = (value<<3) & 0xf8
					g = (value>>2) & 0xf8
					b = (value>>7) & 0xf8
					r |= r>>5
					g |= g>>5
					b |= b>>5
					r = float(r) / 255.0
					g = float(g) / 255.0
					b = float(b) / 255.0
					palette.append([r,g,b])
				gTexturePalettes.append(palette)

		file.seek(tim_offset+16)
		gTextureWidth = struct.unpack("<H",file.read(2))[0]
		if gTextureBpp==4:
			gTextureWidth <<= 2
		if gTextureBpp==8:
			gTextureWidth <<= 1
		gTextureHeight = struct.unpack("<H",file.read(2))[0]
		#print "Texture: %dx" % gTextureWidth + "%d" % gTextureHeight

        except IOError, (errno, strerror):
                file.close()
                return 0

        file.close()
        return 1

def read(filename):
	texture_read(filename[:-4] + ".tim")

	try:
		file = open(filename, "rb")

 		dir_offset = struct.unpack("<L",file.read(4))[0]
		# dir_size = struct.unpack("<L",file.read(4))[0]

		# Seek to dir object 2
 		file.seek(dir_offset + 4*2)
		file_offset = struct.unpack("<L", file.read(4))[0]
		file.seek(file_offset)
		readMeshRelativePosition(file, file_offset)

		# Seek to dir object 7
 		file.seek(dir_offset + 4*7)
		file_offset = struct.unpack("<L", file.read(4))[0]
 
		# Seek to object 7, mesh count
		file.seek(file_offset + 4*2)

		mesh_count = struct.unpack("<L",file.read(4))[0] / 2
		mesh_offset = file.tell()

		for i in range(mesh_count):
			Blender.Window.DrawProgressBar(float(i) / mesh_count, "Loading mesh %d " % i)
			file.seek(mesh_offset + i*7*4*2)
 			mesh = add_mesh(file, mesh_offset, i)
		        Blender.NMesh.PutRaw(mesh, "emd_mesh%d" % i, 1)
 
        except IOError, (errno, strerror):
                file.close()
                return 0

        file.close()
        return 1

def filesel_callback(filename):
        if read(filename) == 0:
                print "Invalid file"
                return

	objname = Blender.sys.splitext(Blender.sys.basename(filename))[0]
	if not meshtools.overwrite_mesh_name:
		objname = meshtools.versioned_name(objname)

        #Blender.NMesh.PutRaw(mesh, objname, 1)
        Blender.Object.GetSelected()[0].name = objname
        Blender.Redraw()
        Blender.Window.DrawProgressBar(1.0, '')
        message = "Successfully imported " + filename
        meshtools.print_boxed(message)

Blender.Window.FileSelector(filesel_callback, "Import EMD")
