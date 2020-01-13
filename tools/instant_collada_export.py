bl_info = {
    "name": "Instant Collada Export",
    "blender": (2, 80, 0),
    "category": "Import-Export",
}

import bpy
import os


class InstantColladaExport(bpy.types.Operator):
    """Instantly export the current scene as collada"""
    bl_idname = "object.instant_collada_export"
    bl_label = "Instant Collada Export"

    def execute(self, context):
        filepath = os.path.splitext(bpy.data.filepath)[0]
        if not filepath:
            self.report({'ERROR'}, "Save the file!")
            return {'CANCELLED'}

        bpy.ops.wm.collada_export(filepath=filepath + '.dae',
                                  apply_modifiers=True,
                                  export_global_forward_selection='-Z',
                                  export_global_up_selection='Y',
                                  apply_global_orientation=True)

        return {'FINISHED'}


def menu_func(self, context):
    self.layout.operator(InstantColladaExport.bl_idname)


def register():
    bpy.utils.register_class(InstantColladaExport)
    bpy.types.TOPBAR_MT_file_export.append(menu_func)


def unregister():
    bpy.utils.unregister_class(InstantColladaExport)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func)


if __name__ == "__main__":
    register()
