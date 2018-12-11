#!/bin/bash -e

# that is the SOURCE of the data and depends where you have cloned the oprecomp-data repository.
# if it was cloned into the same level as the oprecomp main repository, the following relative path works.
SOURCE="data/source/mb/blstm/fraktur_dataset.zip"
SOURCE_GT="data/source/mb/blstm/blstm.ctc.ocr.baseline.zip" # somehow the Gt is stored in that archive.

# TARGET FOLDER
# this is local within the oprecomp project, relative path from the source of that script.
TARGET_ROOT="data/prepared"
TARGET_FOLDER_mb_main="/mb"
TARGET_FOLDER_mb_name="/blstm"

mbdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ -e ${SOURCE} ]
then
  	echo "Found raw data: ${SOURCE}"
else
	echo "Can not find raw data at: ${SOURCE}"
	exit 1 # terminate and indicate error
fi


if [ -e ${TARGET_ROOT} ]
then
  	echo "Found Target dir: ${TARGET_ROOT}"
else
	echo "Can not find Target dir: ${TARGET_ROOT}"
	exit 1 # terminate and indicate error
fi

if [ -e ${TARGET_ROOT}${TARGET_FOLDER_mb_main} ]
then
  	echo "Found dir: ${TARGET_ROOT}${TARGET_FOLDER_mb_main}"
else
	mkdir ${TARGET_ROOT}${TARGET_FOLDER_mb_main}
fi

if [ -e ${TARGET_ROOT}${TARGET_FOLDER_mb_main}${TARGET_FOLDER_mb_name} ]
then
  	echo "Found dir: ${TARGET_ROOT}${TARGET_FOLDER_mb_main}${TARGET_FOLDER_mb_name}"
else
	mkdir ${TARGET_ROOT}${TARGET_FOLDER_mb_main}${TARGET_FOLDER_mb_name}
fi

### FILE TO EXTRACT DATA ###
TRGT="${TARGET_ROOT}${TARGET_FOLDER_mb_main}${TARGET_FOLDER_mb_name}"

echo "Extracting data into ${TRGT}"
echo "This might take a while..."

# WORKS ON MAC, NOT ON LINUX
# tar -C ${TRGT} -xzf ${SOURCE}
# tar -C ${TRGT} -xzf ${SOURCE_GT}

unzip -od ${TRGT} ${SOURCE}
unzip -od ${TRGT} ${SOURCE_GT}

echo "Cleaning files that are not used here ..."

# clean up the mess
if [ -e ${TRGT}/gt/ ]
then
  	rm -rf ${TRGT}/gt/
fi

mv ${TRGT}/blstm.ctc.ocr.baseline/gt ${TRGT}/gt/

rm -rf ${TRGT}/blstm.ctc.ocr.baseline/
echo "[Data preparation finished.]"



