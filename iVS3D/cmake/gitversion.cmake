####################################################################
# Extraction of version string from git tag and commit list
#
# This will set the following variables
# VERSION_STR:      complete version string, e.g. 1.0.1.3
# VERSION_MAJOR:    major version number, e.g. 1
# VERSION_MINOR:    minor version number, e.g. 0
# VERSION_PATCH:    patch version number, e.g. 1
#
####################################################################

# Need to call git with manually specified paths to repository
set(GIT_BASE_COMMAND git --git-dir "${CMAKE_CURRENT_LIST_DIR}/../../.git" --work-tree "${CMAKE_CURRENT_LIST_DIR}/..")

# Trying to get version from git tag / revision
execute_process(COMMAND ${GIT_BASE_COMMAND} describe --always --tags OUTPUT_VARIABLE GIT_TAG)
string(STRIP "${GIT_TAG}" GIT_VERSION) # strip leading and trailing whitespace

# get commit count
execute_process(COMMAND ${GIT_BASE_COMMAND} rev-list HEAD --count OUTPUT_VARIABLE GIT_COMMIT_COUNT)
string(STRIP "${GIT_COMMIT_COUNT}" GIT_COMMIT_COUNT) # strip leading and trailing whitespace
if ("${GIT_COMMIT_COUNT}" STREQUAL "")
    set(GIT_COMMIT_COUNT 0)
endif()

# Check if tag name does not consist of 'v' (optional) followed by three digits
# if not append commit count to tag name
if("${GIT_VERSION}" MATCHES "[v]*[0-9]+[.][0-9]+[.][0-9]+.*")
    
    # if preceeding 'v', remove
    if("${GIT_VERSION}" MATCHES "^[v].*")
        string(SUBSTRING "${GIT_VERSION}" 1 -1 GIT_VERSION)
    endif()

    # Turn describe output like 0.1.5-42-g652c397 into "0.1.5.42.652c397"
    string(REPLACE "-" "." GIT_VERSION "${GIT_VERSION}")

    # remove commit hash
    string(REGEX MATCH "[0-9]+[.][0-9]+[.][0-9]+[.]*[0-9]*" GIT_VERSION "${GIT_VERSION}")

else()

    # append commit count to tag name
    set(GIT_VERSION "0.0.0.${GIT_COMMIT_COUNT}")
endif()

# set return variables
string(REPLACE "." ";" VERSION_LIST "${GIT_VERSION}")
set(VERSION_STR "${GIT_VERSION}")
list(GET VERSION_LIST 0 VERSION_MAJOR)
list(GET VERSION_LIST 1 VERSION_MINOR)
list(GET VERSION_LIST 2 VERSION_PATCH)
