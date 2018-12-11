#!/bin/bash -e

mbdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

./main -m data/prepared/mb/blstm/fraktur_dataset/samples/ data/prepared/mb/blstm/gt/
