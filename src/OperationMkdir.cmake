test_fileop_command_common(
   COMMAND mkdir
   HELP_REGULAR_EXPRESSION "Create directories\\." "Available options are:"
)

test_fileop_check_filesystem(
   NAME Mkdir
   ARGS mkdir dir
   MUST_EXIST dir
)

test_fileop_check_filesystem(
   NAME MkdirTrailingSlash
   ARGS mkdir dir///
   MUST_EXIST dir///
)

test_fileop_check_filesystem(
   NAME MkdirExistingDir
   PREPARE_COMMAND "mkdir dir"
   ARGS mkdir dir
   WILL_FAIL
   FAIL_REGULAR_EXPRESSION "FileOp\\.exe: error: Directory [A-Z]:\\\\.+\\\\dir already exists\\."
)

test_fileop_check_filesystem(
   NAME MkdirExistingDirParents
   PREPARE_COMMAND "mkdir dir"
   ARGS mkdir --parents mkdir
)

test_fileop_check_filesystem(
   NAME MkdirDirStructure
   ARGS mkdir a/b/c
   WILL_FAIL
   MUST_NOT_EXIST a/b/c
)

test_fileop_check_filesystem(
   NAME MkdirDirStructureParents
   ARGS mkdir -p a/b/c
   MUST_EXIST a/b/c
)

test_fileop_check_filesystem(
   NAME MkdirWithDashDash
   ARGS mkdir -- --dir
   MUST_EXIST --dir
)

test_fileop_check_filesystem(
   NAME MkdirFilename
   ARGS mkdir -- ${CMAKE_CURRENT_LIST_FILE}
   WILL_FAIL
   FAIL_REGULAR_EXPRESSION "FileOp\\.exe: error: Not a directory [A-Z]:\\\\.*\\\\OperationMkdir.cmake\\."
)

test_fileop_check_filesystem(
   NAME MkdirIllegalCharacter
   ARGS mkdir --parents "test/invalid_|_name/test_"
   WILL_FAIL
   FAIL_REGULAR_EXPRESSION "FileOp\\.exe: error: Not a directory [A-Z]:\\\\.*\\\\test\\\\invalid_|_name\\\\test_:"
   MUST_EXIST test
)
