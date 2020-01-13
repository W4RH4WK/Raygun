import bpy
import os
import sys
import string

from math import pi

DIR = os.path.dirname(os.path.realpath(__file__))

argv = sys.argv
argv = argv[argv.index('--') + 1:]

font = bpy.data.fonts.load(os.path.join(DIR, argv[0]))
fontname = os.path.splitext(os.path.basename(argv[0]))[0]

bpy.ops.scene.new()

for i in range(128):
    bpy.ops.object.text_add(location=(0, 0, 0), rotation=(pi / 2, 0, 0))
    txt = bpy.context.active_object
    txt.name = str(i)
    txt.data.name = str(i)
    txt.data.body = chr(i)
    txt.data.font = font
    txt.data.extrude = 0.1
    txt.data.resolution_u = 8

    bpy.context.view_layer.update()

    if txt.dimensions.x < 0.01:
        txt.select_set(True)
        bpy.ops.object.delete()

bpy.ops.export_scene.obj(filepath=os.path.join(DIR, fontname + '.obj'),
                         use_materials=False,
                         use_triangles=True)
