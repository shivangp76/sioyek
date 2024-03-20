# note: you might want to change -ios-x86_64 to something like -ios-arm64 for physical device builds
# also don't forget to set the correct architecture in make_script.sh and change the makerules patch
# to not include iphonesimulator

(cd mupdf && git apply ../makerules_changes_for_iphone_simulator.patch)
./make_script.sh

rm -rf ios_temp_files
mkdir ios_temp_files
cd ios_temp_files

# Create a whitelist of symbols exported from mupdf. We only export these symbols from the final library 
nm ../mupdf/build/-ios-x86_64/libmupdf.a -U | grep -e ' T ' -e ' S ' | awk '{print $3}' > whitelist.txt
mkdir libmupdf_unarchived
mkdir libmupdf_third_unarchived
cd libmupdf_unarchived
ar x ../../mupdf/build/-ios-x86_64/libmupdf.a
cd ../libmupdf_third_unarchived
ar x ../../mupdf/build/-ios-x86_64/libmupdf-third.a
cd ..

ls -l libmupdf_unarchived | awk '{ print $9 }' | grep '.*\.o' > mupdf_object_file_names.txt
ls -l libmupdf_third_unarchived | awk '{ print $9 }' | grep '.*\.o' > mupdf_third_object_file_names.txt
comm -12 mupdf_object_file_names.txt mupdf_third_object_file_names.txt > common_files.txt

# rename the common object file names in libmupdf.a and libmupdf-third.a
while read line; do
    filename=$(basename -- "$line")
    filename="${filename%.*}"

    old_path="libmupdf_third_unarchived/${filename}.o"
    new_path="libmupdf_third_unarchived/${filename}_renamed.o"
    mv ${old_path} ${new_path}

done <common_files.txt

cp libmupdf_unarchived/* .
cp libmupdf_third_unarchived/* .

ld -r -o combined.o *.o -exported_symbols_list whitelist.txt
ar rcs libcombined.a combined.o
cp libcombined.a ../mupdf/build/-ios-x86_64/libcombined.a

