#---------------------------------------------------------------------------------------------------
# translations

message("PRE-BUILD: building translation files")
system(lrelease $$PWD/translations/*.ts)
