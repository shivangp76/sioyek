# rebuild the *.frag and *.vert shaders using qsb into the prebuilt folder
ls *.frag *.vert | xargs -I {} qsb --qt6 -o prebuilt/{}.qsb {}

# equivalent windows batch script:
# @echo off
# for %%f in (*.frag *.vert) do qsb --qt6 -o prebuilt/%%f.qsb %%f

