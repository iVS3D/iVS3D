# This script computes and set the rpath according to included libraries

# initialize rpath with origin
RPATH='\$$ORIGIN:\$$ORIGIN/lib'

# split $$LIBS (seperated by whitespace) into list
LIBS_LIST=$$split(LIBS, ' ')

# loop through entry list
for(ENTRY, LIBS_LIST) {

 # get tag by substringing the first two charactes from ENTRY
 TAG=$$str_member($$ENTRY, 0, 1)

 #if entry begins with -L, replace Tag with :
 equals(TAG,-L) {
  ENTRY=$$replace(ENTRY, $$TAG, :)

  # add entry to RPATH except if entry equals ':../../Base/lib/unix'
  !equals(ENTRY, :../../Base/lib/unix) {
   RPATH=$$RPATH$$ENTRY
  }
 }
}
message($$RPATH)
# set rpath
QMAKE_LFLAGS += '-Wl,--rpath=\'$$RPATH\''
