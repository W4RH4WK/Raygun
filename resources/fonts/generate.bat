for %%f in (*.otf,*.ttf) do (
  blender.exe -b --python blender_fontgen.py -- "%%f"
)

pause
