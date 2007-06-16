#!BPY

"""
Name: 'PS1 TMD...'
Blender: 241
Group: 'Import'
Tip: 'Import a TMD file'
"""

__author__ = "Patrice Mandin"
__version__ = "0.1"
__bpydoc__ = """\
This script imports PS1 TMD files into Blender.

Usage:

Run this script from "File->Import" and select the desired TMD file.
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

def addMesh(file):
	tmd_header_size = 12

	vtx_offset = tmd_header_size + struct.unpack("<L",file.read(4))[0]
	vtx_count = struct.unpack("<L",file.read(4))[0]
	nor_offset = tmd_header_size + struct.unpack("<L",file.read(4))[0]
	nor_count = struct.unpack("<L",file.read(4))[0]
	pri_offset = tmd_header_size + struct.unpack("<L",file.read(4))[0]
	pri_count = struct.unpack("<L",file.read(4))[0]
	unknown = struct.unpack("<L",file.read(4))[0]

        mesh = Blender.NMesh.GetRaw()
	mesh.hasVertexColours(1)

	# Read vertices
	file.seek(vtx_offset)
	for i in range(vtx_count):
		x = float(struct.unpack("<h",file.read(2))[0]) / 100.0
		y = float(struct.unpack("<h",file.read(2))[0]) / 100.0
		z = float(struct.unpack("<h",file.read(2))[0]) / 100.0
		w = float(struct.unpack("<h",file.read(2))[0]) / 100.0
		mesh.verts.append(NMesh.Vert(x,y,z))

	# Read normals
	file.seek(nor_offset)
	normals = []
	for i in range(nor_count):
		x = float(struct.unpack("<h",file.read(2))[0]) / 4096.0
		y = float(struct.unpack("<h",file.read(2))[0]) / 4096.0
		z = float(struct.unpack("<h",file.read(2))[0]) / 4096.0
		w = float(struct.unpack("<h",file.read(2))[0]) / 4096.0
		normals.append([x,y,z])

	# Read primitives
	file.seek(pri_offset)
	for i in range(pri_count):
		unknown1 = struct.unpack("B",file.read(1))[0]
		pri_length = struct.unpack("B",file.read(1))[0] * 4
		unknown2 = struct.unpack("B",file.read(1))[0]
		pri_type = struct.unpack("B",file.read(1))[0]

		cur_file_pos = file.tell()

		if (pri_type == 0x20) or (pri_type == 0x22):
			# Coloured triangle
			r = struct.unpack("B",file.read(1))[0]
			g = struct.unpack("B",file.read(1))[0]
			b = struct.unpack("B",file.read(1))[0]
			a = struct.unpack("B",file.read(1))[0]

			n = struct.unpack("<H",file.read(2))[0]
			v0 = struct.unpack("<H",file.read(2))[0]
			v1 = struct.unpack("<H",file.read(2))[0]
			v2 = struct.unpack("<H",file.read(2))[0]

			f=NMesh.Face()
			f.v.append(mesh.verts[v0])
			f.v.append(mesh.verts[v1])
			f.v.append(mesh.verts[v2])
			f.col.append(Blender.NMesh.Col(r,g,b,255))
			f.col.append(Blender.NMesh.Col(r,g,b,255))
			f.col.append(Blender.NMesh.Col(r,g,b,255))
			mesh.faces.append(f)
		elif (pri_type == 0x24):
			# Gourauded triangle
			r0 = struct.unpack("B",file.read(1))[0]
			g0 = struct.unpack("B",file.read(1))[0]
			b0 = struct.unpack("B",file.read(1))[0]
			a0 = struct.unpack("B",file.read(1))[0]

			r1 = struct.unpack("B",file.read(1))[0]
			g1 = struct.unpack("B",file.read(1))[0]
			b1 = struct.unpack("B",file.read(1))[0]
			a1 = struct.unpack("B",file.read(1))[0]

			r2 = struct.unpack("B",file.read(1))[0]
			g2 = struct.unpack("B",file.read(1))[0]
			b2 = struct.unpack("B",file.read(1))[0]
			a2 = struct.unpack("B",file.read(1))[0]

			n = struct.unpack("<H",file.read(2))[0]
			v0 = struct.unpack("<H",file.read(2))[0]
			v1 = struct.unpack("<H",file.read(2))[0]
			v2 = struct.unpack("<H",file.read(2))[0]

			f=NMesh.Face()
			f.v.append(mesh.verts[v0])
			f.v.append(mesh.verts[v1])
			f.v.append(mesh.verts[v2])
			f.col.append(Blender.NMesh.Col(r0,g0,b0,255))
			f.col.append(Blender.NMesh.Col(r1,g1,b1,255))
			f.col.append(Blender.NMesh.Col(r2,g2,b2,255))
			mesh.faces.append(f)
		elif (pri_type == 0x28):
			# Coloured quad
			r = struct.unpack("B",file.read(1))[0]
			g = struct.unpack("B",file.read(1))[0]
			b = struct.unpack("B",file.read(1))[0]
			a = struct.unpack("B",file.read(1))[0]

			n = struct.unpack("<H",file.read(2))[0]
			v0 = struct.unpack("<H",file.read(2))[0]
			v1 = struct.unpack("<H",file.read(2))[0]
			v2 = struct.unpack("<H",file.read(2))[0]
			v3 = struct.unpack("<H",file.read(2))[0]

			f=NMesh.Face()
			f.v.append(mesh.verts[v0])
			f.v.append(mesh.verts[v1])
			f.v.append(mesh.verts[v3])
			f.v.append(mesh.verts[v2])
			f.col.append(Blender.NMesh.Col(r,g,b,255))
			f.col.append(Blender.NMesh.Col(r,g,b,255))
			f.col.append(Blender.NMesh.Col(r,g,b,255))
			f.col.append(Blender.NMesh.Col(r,g,b,255))
			mesh.faces.append(f)
		elif (pri_type == 0x2c):
			# Gourauded quad
			r0 = struct.unpack("B",file.read(1))[0]
			g0 = struct.unpack("B",file.read(1))[0]
			b0 = struct.unpack("B",file.read(1))[0]
			a0 = struct.unpack("B",file.read(1))[0]

			r1 = struct.unpack("B",file.read(1))[0]
			g1 = struct.unpack("B",file.read(1))[0]
			b1 = struct.unpack("B",file.read(1))[0]
			a1 = struct.unpack("B",file.read(1))[0]

			r2 = struct.unpack("B",file.read(1))[0]
			g2 = struct.unpack("B",file.read(1))[0]
			b2 = struct.unpack("B",file.read(1))[0]
			a2 = struct.unpack("B",file.read(1))[0]

			r3 = struct.unpack("B",file.read(1))[0]
			g3 = struct.unpack("B",file.read(1))[0]
			b3 = struct.unpack("B",file.read(1))[0]
			a3 = struct.unpack("B",file.read(1))[0]

			n = struct.unpack("<H",file.read(2))[0]
			v0 = struct.unpack("<H",file.read(2))[0]
			v1 = struct.unpack("<H",file.read(2))[0]
			v2 = struct.unpack("<H",file.read(2))[0]
			v3 = struct.unpack("<H",file.read(2))[0]

			f=NMesh.Face()
			f.v.append(mesh.verts[v0])
			f.v.append(mesh.verts[v1])
			f.v.append(mesh.verts[v3])
			f.v.append(mesh.verts[v2])
			f.col.append(Blender.NMesh.Col(r0,g0,b0,255))
			f.col.append(Blender.NMesh.Col(r1,g1,b1,255))
			f.col.append(Blender.NMesh.Col(r3,g3,b3,255))
			f.col.append(Blender.NMesh.Col(r2,g2,b2,255))
			mesh.faces.append(f)
		elif (pri_type == 0x30):
			# Coloured triangle with per-vertex normal
			r = struct.unpack("B",file.read(1))[0]
			g = struct.unpack("B",file.read(1))[0]
			b = struct.unpack("B",file.read(1))[0]
			a = struct.unpack("B",file.read(1))[0]

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
			f.col.append(Blender.NMesh.Col(r,g,b,255))
			f.col.append(Blender.NMesh.Col(r,g,b,255))
			f.col.append(Blender.NMesh.Col(r,g,b,255))
			mesh.faces.append(f)
		elif (pri_type == 0x34):
			# Textured triangle with per-vertex normal
			unknown0 = struct.unpack("<H",file.read(2))[0]
			tv0 = struct.unpack("B",file.read(1))[0]
			tu0 = struct.unpack("B",file.read(1))[0]

			unknown1 = struct.unpack("<H",file.read(2))[0]
			tv1 = struct.unpack("B",file.read(1))[0]
			tu1 = struct.unpack("B",file.read(1))[0]

			unknown2 = struct.unpack("<H",file.read(2))[0]
			tv2 = struct.unpack("B",file.read(1))[0]
			tu2 = struct.unpack("B",file.read(1))[0]

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
		elif (pri_type == 0x38):
			# Coloured quad with per-vertex normal
			r = struct.unpack("B",file.read(1))[0]
			g = struct.unpack("B",file.read(1))[0]
			b = struct.unpack("B",file.read(1))[0]
			a = struct.unpack("B",file.read(1))[0]

			n0 = struct.unpack("<H",file.read(2))[0]
			v0 = struct.unpack("<H",file.read(2))[0]
			n1 = struct.unpack("<H",file.read(2))[0]
			v1 = struct.unpack("<H",file.read(2))[0]
			n2 = struct.unpack("<H",file.read(2))[0]
			v2 = struct.unpack("<H",file.read(2))[0]
			n3 = struct.unpack("<H",file.read(2))[0]
			v3 = struct.unpack("<H",file.read(2))[0]

			f=NMesh.Face()
			f.v.append(mesh.verts[v0])
			f.v.append(mesh.verts[v1])
			f.v.append(mesh.verts[v3])
			f.v.append(mesh.verts[v2])
			f.col.append(Blender.NMesh.Col(r,g,b,255))
			f.col.append(Blender.NMesh.Col(r,g,b,255))
			f.col.append(Blender.NMesh.Col(r,g,b,255))
			f.col.append(Blender.NMesh.Col(r,g,b,255))
			mesh.faces.append(f)

		file.seek(cur_file_pos + pri_length)

	return mesh

def read(filename):
	try:
		file = open(filename, "rb")

		id = struct.unpack("<L",file.read(4))[0]
		if (id != 0x41):
			file.close()
			return 0

		file.seek(4, 1)
		mesh_count = struct.unpack("<L",file.read(4))[0]

		mesh_header_offset = file.tell()
		for i in range(mesh_count):
			Blender.Window.DrawProgressBar(float(i) / mesh_count, "Loading mesh %d " % i)
			file.seek(mesh_header_offset + i*7*4)
 			mesh = addMesh(file)
		        Blender.NMesh.PutRaw(mesh, "tmd_mesh%d" % i, 1)
 
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

Blender.Window.FileSelector(filesel_callback, "Import TMD")
