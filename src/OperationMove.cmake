test_fileop_command_common(
   COMMAND move
   HELP_REGULAR_EXPRESSION "Move files or directories\\." "Available options are:"
   TEST_TIME_OPTION
)

test_fileop_check_filesystem(
   NAME MoveMissingTarget
   ARGS move ${CMAKE_CURRENT_LIST_FILE}
   WILL_FAIL
   FAIL_REGULAR_EXPRESSION "FileOp\\.exe: error: Too view arguments given\\."
)

test_fileop_check_filesystem(
   NAME MoveMissingSource
   ARGS move file1 file2
   WILL_FAIL
   FAIL_REGULAR_EXPRESSION "FileOp\\.exe: error: Can't move file .+\\\\file to .+\\\\file2: The system cannot find the file specified\\."
)

test_fileop_check_filesystem(
   NAME MoveMissingTargetDirectoryI
   ARGS move --target-directory= CopyMissingTargetDirectoryI
   WILL_FAIL
   FAIL_REGULAR_EXPRESSION "FileOp\\.exe: error: Target directory must not be empty\\."
)

test_fileop_check_filesystem(
   NAME MoveMissingTargetDirectoryII
   ARGS move --target-directory
   WILL_FAIL
   FAIL_REGULAR_EXPRESSION "FileOp\\.exe: error: Option --target-directory needs an argument\\."
)

test_fileop_check_filesystem(
   NAME MoveNonExistingTargetDirectory
   PREPARE_COMMAND "cp -f ${CMAKE_CURRENT_LIST_FILE} MoveNonExistingTargetDirectory.in"
   ARGS move MoveNonExistingTargetDirectory.in MoveNonExistingTargetDirectory/
   WILL_FAIL
   FAIL_REGULAR_EXPRESSION "FileOp\\.exe: error: Directory [A-Z]:\\\\.+\\\\MoveNonExistingTargetDirectory doesn't exist\\."
   MUST_NOT_EXIST MoveNonExistingTargetDirectory/MoveNonExistingTargetDirectory.in
)

test_fileop_check_filesystem(
   NAME MoveFileWithTargetDirectory
   PREPARE_COMMAND "mkdir SubDir && cp -f ${CMAKE_CURRENT_LIST_FILE} ."
   ARGS move --target-directory=SubDir OperationMove.cmake
   MUST_NOT_EXIST OperationMove.cmake
   MUST_EXIST SubDir/OperationMove.cmake
)

test_fileop_check_filesystem(
   NAME MoveFileWithTargetDirectoryForce
   PREPARE_COMMAND "mkdir SubDir && cp -f ${CMAKE_CURRENT_LIST_FILE} ."
   ARGS move --force --target-directory SubDir OperationMove.cmake
   MUST_NOT_EXIST OperationMove.cmake
   MUST_EXIST SubDir/OperationMove.cmake
)

test_fileop_check_filesystem(
   NAME MoveFileWithTargetDirectoryUniqueNames
   PREPARE_COMMAND "mkdir SubDir additional_file && cp -f ${CMAKE_CURRENT_LIST_FILE} . && cp -f ${CMAKE_CURRENT_LIST_FILE} additional_file"
   ARGS move --check-unique-names --target-directory SubDir ../../FileOp.exe OperationMove.cmake additional_file/OperationMove.cmake
   WILL_FAIL
   FAIL_REGULAR_EXPRESSION "FileOp\\.exe: error: File in source list will overwrite each other: OperationMove.cmake"
   MUST_EXIST OperationMove.cmake additional_file/OperationMove.cmake
   MUST_NOT_EXIST SubDir/OperationMove.cmake
)

test_fileop_check_filesystem(
   NAME MoveFileWithPattern
   PREPARE_COMMAND "mkdir SubDir && cp -f ${CMAKE_CURRENT_LIST_DIR}/*.* ."
   ARGS move --target-directory SubDir OperationMove.*
   MUST_EXIST SubDir/OperationMove.c SubDir/OperationMove.cmake SubDir/OperationMove.h
   MUST_NOT_EXIST OperationMove.c OperationMove.cmake OperationMove.h
)

test_fileop_check_filesystem(
   NAME MoveFileListWithPattern
   PREPARE_COMMAND "mkdir -p Source Target && cp -f ${CMAKE_CURRENT_LIST_DIR}/OperationMove.* Source/"
   ARGS move --touch --time=2001-01-01T00:30 Source/OperationMove.* Target/
   MUST_EXIST Target/OperationMove.c Target/OperationMove.cmake Target/OperationMove.h
   MUST_NOT_EXIST Source/OperationMove.c Source/OperationMove.cmake Source/OperationMove.h
   FILE_TIMESTAMP_REGEX "2001-01-01 00:30:00\\.000000000 \\+0000"
)

test_fileop_check_filesystem(
   NAME MoveFileListWithPatternExisting
   PREPARE_COMMAND "mkdir -p Source Target && cp -f ${CMAKE_CURRENT_LIST_DIR}/OperationMove.* Source/ && cp -f Source/*.* Target/"
   ARGS move --touch --time=2002-01-01T00:30 Target/OperationMove.* MoveFileListWithPatternTarget/
   WILL_FAIL
   MUST_EXIST Source/OperationMove.c Source/OperationMove.cmake Source/OperationMove.h
)

test_fileop_check_filesystem(
   NAME MoveFileListWithPatternExistingForced
   PREPARE_COMMAND "mkdir -p Source Target && cp -f ${CMAKE_CURRENT_LIST_DIR}/OperationMove.* Source/ && cp -f Source/*.* Target/ && chmod -R oga-w ."
   ARGS move --touch --force --time=2002-01-01T00:30 Source/*.* Target/
   MUST_EXIST Target/OperationMove.c Target/OperationMove.cmake Target/OperationMove.h
   MUST_NOT_EXIST Source/OperationMove.c Source/OperationMove.cmake Source/OperationMove.h
   FILE_TIMESTAMP_REGEX "2002-01-01 00:30:00\\.000000000 \\+0000"
)

test_fileop_check_filesystem(
   NAME MoveFileWithDashDash
   PREPARE_COMMAND "cp -f -- ${CMAKE_CURRENT_LIST_FILE} --source"
   ARGS move -- --source --target
   MUST_NOT_EXIST --source
   MUST_EXIST --target
)

test_fileop_check_filesystem(
   NAME MoveFileToDirectory
   PREPARE_COMMAND "cp -f -- ${CMAKE_CURRENT_LIST_FILE} --source && mkdir -p SubDir"
   ARGS move -- --source SubDir
   MUST_NOT_EXIST --source
   MUST_EXIST SubDir/--source
)

test_fileop_check_filesystem(
   NAME MoveDirectoryToExistingFile
   PREPARE_COMMAND "mkdir SubDir && touch target SubDir/file"
   ARGS --debug move --target-directory target SubDir
   WILL_FAIL
   MUST_EXIST target
   MUST_NOT_EXIST target/SubDir/file
   FAIL_REGULAR_EXPRESSION "FileOp\\.exe: error: Target .+\\\\target must be a directory\\."
)

test_fileop_check_filesystem(
   NAME MoveFileExistingTarget
   PREPARE_COMMAND "touch source target"
   ARGS move -- source target
   WILL_FAIL
   MUST_EXIST source target
   FAIL_REGULAR_EXPRESSION "FileOp\\.exe: error: Can't move file .+\\\\source to .+\\\\target: The file exists\\."
)

test_fileop_check_filesystem(
   NAME MoveFileExistingTargetForce
   PREPARE_COMMAND "touch source target && chmod -R oga-w ."
   ARGS move --force -- source target
   MUST_NOT_EXIST source
   MUST_EXIST target
)

test_fileop_check_filesystem(
   NAME MoveDirectoryRecursive
   PREPARE_COMMAND "mkdir -p source target/source && cp -rf ${CMAKE_CURRENT_LIST_DIR}/OperationMove.* source/ && chmod -R oga-w ."
   ARGS --debug move --force --touch --time 2000-01-01 source target
   MUST_EXIST target/source/OperationMove.c target/source/OperationMove.cmake target/source/OperationMove.h
   MUST_NOT_EXIST source/OperationMove.c source/OperationMove.cmake source/OperationMove.h
   FILE_TIMESTAMP_REGEX "2000-01-01 12:00:00\\.000000000 \\+0000"
)
