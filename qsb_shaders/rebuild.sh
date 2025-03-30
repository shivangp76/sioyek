# rebuild the *.frag and *.vert shaders using qsb into the prebuilt folder
ls *.frag *.vert | xargs -I {} qsb --qt6 -o prebuilt/{}.qsb {}
