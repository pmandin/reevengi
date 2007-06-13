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

def add_mesh(file, start_offset, num_mesh):
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
			x = float(struct.unpack("<h",file.read(2))[0]) / 256.0
			y = float(struct.unpack("<h",file.read(2))[0]) / 256.0
			y += num_mesh * 4.0
			z = float(struct.unpack("<h",file.read(2))[0]) / 256.0
			w = float(struct.unpack("<h",file.read(2))[0]) / 256.0
			mesh.verts.append(NMesh.Vert(x,y,z))

		# nor_list = []
		# file.seek(tri_nor_offset)
		# for i in range(tri_nor_count):
		# 	x = struct.unpack("<H",file.read(2))[0]
		# 	y = struct.unpack("<H",file.read(2))[0]
		# 	z = struct.unpack("<H",file.read(2))[0]
		# 	w = struct.unpack("<H",file.read(2))[0]
		# 	nor_list.append(x,y,z)

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
			f.v.append(mesh.verts[v0])
			f.v.append(mesh.verts[v1])
			f.v.append(mesh.verts[v2])
			mesh.faces.append(f)

	if quad_count>0:
		file.seek(quad_vtx_offset)
		for i in range(quad_vtx_count):
			#print "Reading quad vertex %d" % i
			x = float(struct.unpack("<h",file.read(2))[0]) / 256.0
			y = float(struct.unpack("<h",file.read(2))[0]) / 256.0
			y += num_mesh * 4.0
			z = float(struct.unpack("<h",file.read(2))[0]) / 256.0
			w = float(struct.unpack("<h",file.read(2))[0]) / 256.0
			mesh.verts.append(NMesh.Vert(x,y,z))

		# nor_list = []
		# file.seek(quad_nor_offset)
		# for i in range(tri_quad_count):
		# 	x = struct.unpack("<H",file.read(2))[0]
		# 	y = struct.unpack("<H",file.read(2))[0]
		# 	z = struct.unpack("<H",file.read(2))[0]
		# 	w = struct.unpack("<H",file.read(2))[0]
		# 	nor_list.append(x,y,z)

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
			f.v.append(mesh.verts[tri_vtx_count+v0])
			f.v.append(mesh.verts[tri_vtx_count+v1])
			f.v.append(mesh.verts[tri_vtx_count+v3])
			f.v.append(mesh.verts[tri_vtx_count+v2])
			mesh.faces.append(f)

	return mesh

def read(filename):
	try:
		file = open(filename, "rb")

 		dir_offset = struct.unpack("<L",file.read(4))[0]
		# dir_size = struct.unpack("<L",file.read(4))[0]

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
