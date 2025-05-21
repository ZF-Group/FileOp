test_fileop_command_common(
   COMMAND remove
   HELP_REGULAR_EXPRESSION "Remove files or directories\\." "Available options are:"
)

test_fileop_check_filesystem(
   NAME RemoveNonExisting
   ARGS --debug remove file
   PASS_REGULAR_EXPRESSION "Skip .+\\\\file because it doesn't exist\\."
)

test_fileop_check_filesystem(
   NAME RemoveSingleFile
   PREPARE_COMMAND "touch file"
   ARGS remove file
   MUST_NOT_EXIST file
)

set(REMOVE_PREPARE_COMMAND "mkdir -p test/subdir junction_target && echo 'Junction target file content' > junction_target/test_file && touch test/subdir/test_file_1 test/subdir/test_file_2_readonly test/subdir/test_file_3 && attrib +R test/subdir/test_file_2_readonly && pushd test/subdir && cmd.exe //C \"mklink /J junction ..\\..\\junction_target\" && cat junction/test_file && popd")

test_fileop_check_filesystem(
   NAME RemoveNotEmptyDirectory
   PREPARE_COMMAND ${REMOVE_PREPARE_COMMAND}
   ARGS remove test
   WILL_FAIL
   MUST_EXIST junction_target/test_file test/subdir/junction/test_file test/subdir/test_file_1 test/subdir/test_file_3
   FAIL_REGULAR_EXPRESSION "FileOp.exe: error: Can't remove directory .+\\\\test: The directory is not empty."
)

test_fileop_check_filesystem(
   NAME RemoveNotEmptyDirectoryRecursive
   PREPARE_COMMAND ${REMOVE_PREPARE_COMMAND}
   ARGS --debug remove --recursive test
   WILL_FAIL
   MUST_EXIST junction_target/test_file test/subdir/test_file_3
   MUST_NOT_EXIST test/subdir/junction/test_file  test/subdir/test_file_1
   FAIL_REGULAR_EXPRESSION "FileOp.exe: error: Can't remove file .+\\\\test\\\\subdir\\\\test_file_2_readonly: Access is denied."
)

test_fileop_check_filesystem(
   NAME RemoveNotEmptyDirectoryRecursiveForce
   PREPARE_COMMAND ${REMOVE_PREPARE_COMMAND}
   ARGS remove --recursive --force test
   MUST_EXIST junction_target/test_file
   MUST_NOT_EXIST test/subdir/junction/test_file test/subdir/test_file_1 test/subdir/test_file_3
)

test_fileop_check_filesystem(
   NAME RemoveFileWithDashDash
   PREPARE_COMMAND "touch -- --file"
   ARGS remove -- --file
   MUST_NOT_EXIST --file
)
