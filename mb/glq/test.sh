#!/bin/bash -e

mbdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# plain C-Version
testsuite/MB000

# openMP Version
testsuite/MB001