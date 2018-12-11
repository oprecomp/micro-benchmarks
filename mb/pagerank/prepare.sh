#!/bin/bash -e

# SOURCE AND FURTHER INFO
# [1]: http://www.cs.toronto.edu/~tsap/experiments/download/download.html
# The following extracts data and generates full date as provided by the scripts form [1]

# that is the SOURCE of the data and depends where you have cloned the oprecomp-data repository.
# if it was cloned into the same level as the oprecomp main repository, the following relative path works.
SOURCE="data/source/mb/pagerank/All.tar"
# TARGET FOLDER
# this is local within the oprecomp project, relative path from the source of that script.
TARGET_ROOT="data/prepared"
TARGET_FOLDER_mb_main="/mb"
TARGET_FOLDER_mb_name="/pagerank"

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

tar -C ${TRGT} -xzf ${SOURCE} 

tar -C ${TRGT} -xzf ${TRGT}/_abortion.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_affirmative_action.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_alcohol.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_amusement_parks.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_architecture.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_armstrong.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_automobile_industries.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_basketball.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_blues.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_cheese.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_classical_guitar.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_complexity.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_computational_complexity.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_computational_geometry.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_death_penalty.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_genetic.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_geometry.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_globalization.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_gun_control.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_iraq_war.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_jaguar.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_jordan.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_moon_landing.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_movies.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_national_parks.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_net_censorship.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_randomized_algorithms.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_recipes.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_roswell.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_search_engines.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_shakespeare.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_table_tennis.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_vintage_cars.tar.Z
tar -C ${TRGT} -xzf ${TRGT}/_weather.tar.Z

# cd ../

echo DONE
echo Compiling to rewrite data ... 

### compile to rewrite data
gcc -Wall -o list2matrix ${mbdir}/list2matrix.c

echo DONE
echo Rewrite data ... 

# ### rewrite the data
echo running: ./list2matrix ${TRGT}/_abortion
./list2matrix ${TRGT}/_abortion
echo running: ./list2matrix ${TRGT}/_affirmative_action
./list2matrix ${TRGT}/_affirmative_action
echo running: ./list2matrix ${TRGT}/_alcohol
./list2matrix ${TRGT}/_alcohol
echo running: ./list2matrix ${TRGT}/_amusement_parks
./list2matrix ${TRGT}/_amusement_parks
echo running: ./list2matrix ${TRGT}/_architecture
./list2matrix ${TRGT}/_architecture
echo running: ./list2matrix ${TRGT}/_armstrong
./list2matrix ${TRGT}/_armstrong
echo running: ./list2matrix ${TRGT}/_automobile_industries
./list2matrix ${TRGT}/_automobile_industries
echo running: ./list2matrix ${TRGT}/_basketball
./list2matrix ${TRGT}/_basketball
echo running: ./list2matrix ${TRGT}/_blues
./list2matrix ${TRGT}/_blues
echo running: ./list2matrix ${TRGT}/_cheese
./list2matrix ${TRGT}/_cheese
echo running: ./list2matrix ${TRGT}/_classical_guitar
./list2matrix ${TRGT}/_classical_guitar
echo running: ./list2matrix ${TRGT}/_complexity
./list2matrix ${TRGT}/_complexity
echo running: ./list2matrix ${TRGT}/_computational_complexity
./list2matrix ${TRGT}/_computational_complexity
echo running: ./list2matrix ${TRGT}/_computational_geometry
./list2matrix ${TRGT}/_computational_geometry
echo running: ./list2matrix ${TRGT}/_death_penalty
./list2matrix ${TRGT}/_death_penalty
echo running: ./list2matrix ${TRGT}/_genetic
./list2matrix ${TRGT}/_genetic
echo running: ./list2matrix ${TRGT}/_geometry
./list2matrix ${TRGT}/_geometry
echo running: ./list2matrix ${TRGT}/_globalization
./list2matrix ${TRGT}/_globalization
echo running: ./list2matrix ${TRGT}/_gun_control
./list2matrix ${TRGT}/_gun_control
echo running: ./list2matrix ${TRGT}/_iraq_war
./list2matrix ${TRGT}/_iraq_war
echo running: ./list2matrix ${TRGT}/_jaguar
./list2matrix ${TRGT}/_jaguar
echo running: ./list2matrix ${TRGT}/_jordan
./list2matrix ${TRGT}/_jordan
echo running: ./list2matrix ${TRGT}/_moon_landing
./list2matrix ${TRGT}/_moon_landing
echo running: ./list2matrix ${TRGT}/_movies
./list2matrix ${TRGT}/_movies
echo running: ./list2matrix ${TRGT}/_national_parks
./list2matrix ${TRGT}/_national_parks
echo running: ./list2matrix ${TRGT}/_net_censorship
./list2matrix ${TRGT}/_net_censorship
echo running: ./list2matrix ${TRGT}/_randomized_algorithms
./list2matrix ${TRGT}/_randomized_algorithms
echo running: ./list2matrix ${TRGT}/_recipes
./list2matrix ${TRGT}/_recipes
echo running: ./list2matrix ${TRGT}/_roswell
./list2matrix ${TRGT}/_roswell
echo running: ./list2matrix ${TRGT}/_search_engines
./list2matrix ${TRGT}/_search_engines
echo running: ./list2matrix ${TRGT}/_shakespeare
./list2matrix ${TRGT}/_shakespeare
echo running: ./list2matrix ${TRGT}/_table_tennis
./list2matrix ${TRGT}/_table_tennis
echo running: ./list2matrix ${TRGT}/_vintage_cars
./list2matrix ${TRGT}/_vintage_cars
echo running: ./list2matrix ${TRGT}/_weather
./list2matrix ${TRGT}/_weather

echo ALL DONE