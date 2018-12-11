#!/bin/bash -e

mbdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# testsuite/test000
# testsuite/MB000	${mbdir}/testsuite/data
testsuite/MB001 ${mbdir}/testsuite/data data/prepared/mb/pagerank
