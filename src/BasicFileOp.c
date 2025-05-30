
#include "BasicFileOp.h"
#include "FileOp.h"
#include "Message.h"

#include <fcntl.h>
#include <io.h>
#include <ntdef.h>
#include <shellapi.h>
#include <stdlib.h>
#include <winioctl.h>

#include <sys/stat.h>

//! The prefix for a DOS device path
LPCTSTR DosDevicePathPrefix = _T("\\\\?\\");
// The length of the DOS device path prefix
const int DOS_DEVICE_PREFIX_LENGTH = sizeof(DosDevicePathPrefix) / sizeof(TCHAR);
//! The prefix for a DOS device path prefix for UNC paths
LPCTSTR DosDevicePathPrefixUnc = _T("\\\\?\\UNC\\");
// The length of the DOS device path prefix for UNC paths
const int DOS_DEVICE_PREFIX_UNC_LENGTH = sizeof(DosDevicePathPrefixUnc) / sizeof(TCHAR);

//! The buffer for the source path if needed.
TCHAR DosDevicePath[DOS_DEVICE_BUFFER_SIZE] = _T("");
//! The buffer for the target path.
TCHAR TargetDosDevicePath[DOS_DEVICE_BUFFER_SIZE] = _T("");
//! The buffer for the current response file.
TCHAR DosDevicePathResponseFile[DOS_DEVICE_BUFFER_SIZE] = _T("");

TCHAR **KnownFilenames = NULL;

//! Size ot the read buffer.
#define SIZE_IO_BUFFER (0x1FFFF)
//! The buffer to read a file.
char IoBuffer[SIZE_IO_BUFFER];
//! The buffer to read a file.
TCHAR IoBufferWideCharacter[SIZE_IO_BUFFER];

//! The time format for touch
static const TCHAR TIME_FORMAT[] = _T("%4d-%02d-%02dT%02d:%02d:%02d");
static LPFILETIME ptr_file_time = NULL;

/*!
 * Create the used DOS device path (starting with \\?\). Path is normalized and
 * changed to an absolute path..
 *
 * @param currentSourcePath The path to normalize.
 * @param currentPath       The target buffer for the DOS device path.
 */
void createDosDevicePath(LPCTSTR currentSourcePath, LPTSTR currentPath) {
   static TCHAR TempSourcePath[DOS_DEVICE_BUFFER_SIZE];

   DWORD Size = GetFullPathName(currentSourcePath, DOS_DEVICE_BUFFER_SIZE, TempSourcePath, NULL);
   // GCOVR_EXCL_START
   if (Size == 0L) {
      printLastError(_T("Could not get full name of %s"), currentSourcePath);
      flushOutputAndExit(EXIT_FAILURE);
   } else if (Size > DOS_DEVICE_BUFFER_SIZE) {
      printLastError(_T("Could not get full name of %s. Buffer is to small, needed are %d characters."), currentSourcePath, Size);
      flushOutputAndExit(EXIT_FAILURE);
   }
   // GCOVR_EXCL_STOP
   LPTSTR StartOfPath = TempSourcePath;
   // Create the used DOS device path (starting with \\?\).
   if (_tcsncmp(StartOfPath, DosDevicePathPrefix, DOS_DEVICE_PREFIX_LENGTH) == 0) {
      currentPath[0] = _T('\0');
   } else {
      if (_tcsncmp(StartOfPath, _T("\\\\"), 2) == 0) {
         StartOfPath += 2;
         _tcscpy(currentPath, DosDevicePathPrefixUnc);
         // \\?\UNC\server\share
      } else {
         _tcscpy(currentPath, DosDevicePathPrefix);
      }
   }

   _tcscat(currentPath, StartOfPath);

   return;
}

/*!
 * Get a readable file without DOS device prefix.
 *
 * @param currentPath The path to get the prefix from
 * @return The path after the DOS device prefix
 */
LPCTSTR getReadableFilename(LPCTSTR currentPath) { return &currentPath[DOS_DEVICE_PREFIX_LENGTH]; }

/*!
 * Check if the attributes are valid
 *
 * @param dwAttrs The attributes to check.
 * @return eOk if file attributes are valid
 */
tResult isValidFileAttributes(const DWORD dwAttrs) { return (dwAttrs == INVALID_FILE_ATTRIBUTES) ? eError : eOk; }

/*!
 * Check if readonly attribute is set.
 *
 * @param dwAttrs The attributes to check.
 * @return eOk if readonly attribute is set, else eError.
 */
tResult isReadonly(const DWORD dwAttrs) { return (isValidFileAttributes(dwAttrs) && ((dwAttrs & FILE_ATTRIBUTE_READONLY) != 0)) ? eOk : eError; }

/*!
 * Check if it is a reparse point
 *
 * @param dwAttrs The attributes to check.
 * @return eOk if element is a reparse point, else eError.
 */
tResult isReparsePoint(const DWORD dwAttrs) {
   return (isValidFileAttributes(dwAttrs) && ((dwAttrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0)) ? eOk : eError;
}

/*!
 * Check if directory attribute is set.
 *
 * @param dwAttrs The attributes to check.
 * @return eOk if directory attribute is set, else eError.
 */
tResult isDirectory(const DWORD dwAttrs) { return (isValidFileAttributes(dwAttrs) && ((dwAttrs & FILE_ATTRIBUTE_DIRECTORY) != 0)) ? eOk : eError; }

/*!
 * Clear the readonly attribute.
 *
 * @param currentPath The file for which the attribute are changed.
 * @param dwAttrs     The current attribute value.
 * @return eOk on success, else eError.
 */
tResult clearReadonly(LPCTSTR currentPath, const DWORD dwAttrs) {
   if (Debug) {
      printOut(_T("Clear readonly flag of element %s\n"), getReadableFilename(currentPath));
   }
   if (SetFileAttributes(currentPath, dwAttrs & (~FILE_ATTRIBUTE_READONLY)) == 0) {
      // GCOVR_EXCL_START
      return printLastError(_T("Can't clear readonly flag of element %s"), getReadableFilename(currentPath));
      // GCOVR_EXCL_STOP
   }
   return eOk;
}

/*!
 * Create a single directory.
 *
 * @param currentPath The directory to create.
 * @return eOk on success, else eError.
 */
tResult createSingleDirectory(LPCTSTR currentPath) {
   if (Debug) {
      printOut(_T("Create directory %s\n"), getReadableFilename(currentPath));
   }
   if (CreateDirectory(currentPath, NULL) == 0) {
      return printLastError(_T("Can't create directory %s"), getReadableFilename(currentPath));
   }

   return eOk;
}

/*!
 * Delete a empty directory directory.
 *
 * @param currentPath The directory to remove.
 * @return eOk on success, else eError.
 */
tResult removeEmptyDirectory(LPCTSTR currentPath) {
   if (Debug) {
      printOut(_T("Remove directory %s\n"), getReadableFilename(currentPath));
   }
   if (RemoveDirectory(currentPath) == 0) {
      return printLastError(_T("Can't remove directory %s"), getReadableFilename(currentPath));
   }

   return eOk;
}

/*!
 * Delete a reparse point.
 *
 * @param currentPath The reparse point to remove.
 * @return eOk on success, else eError.
 */
tResult removeReparsePoint(LPCTSTR currentPath) {
   if (Debug) {
      printOut(_T("Remove reparse point %s\n"), getReadableFilename(currentPath));
   }

   HANDLE Handle =
       CreateFile(currentPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
   if (Handle == INVALID_HANDLE_VALUE) {                                                     // GCOVR_EXCL_BR_LINE
      return printLastError(_T("Can't get handle to %s"), getReadableFilename(currentPath)); // GCOVR_EXCL_LINE
   }

   REPARSE_DATA_BUFFER ReparseDataBuffer = {0};
   ReparseDataBuffer.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;

   DWORD BytesReturned = 0;
   if (DeviceIoControl(Handle, FSCTL_DELETE_REPARSE_POINT, &ReparseDataBuffer, REPARSE_DATA_BUFFER_HEADER_SIZE, NULL, 0, &BytesReturned, 0) == 0) {
      // GCOVR_EXCL_START
      printLastError(_T("Can't remove reparse point %s"), getReadableFilename(currentPath));
      if (CloseHandle(Handle) == 0) {
         printLastError(_T("Can't close handle to %s"), getReadableFilename(currentPath));
      }
      return eError;
      // GCOVR_EXCL_STOP
   }
   if (CloseHandle(Handle) == 0) {                                                             // GCOVR_EXCL_BR_LINE
      return printLastError(_T("Can't close handle to %s"), getReadableFilename(currentPath)); // GCOVR_EXCL_LINE
   }
   return removeEmptyDirectory(currentPath);
}

/*!
 * Delete a single file.
 *
 * @param currentPath The file to remove.
 * @return eOk on success, else eError.
 */
tResult removeSingleFile(LPCTSTR currentPath) {
   if (Debug) {
      printOut(_T("Remove file %s\n"), getReadableFilename(currentPath));
   }
   if (DeleteFile(currentPath) == 0) {
      return printLastError(_T("Can't remove file %s"), getReadableFilename(currentPath));
   }

   return eOk;
}

/*!
 * Copy a single file.
 *
 * @param currentSourcePath The file to copy.
 * @param currentTargetPath The target file.
 * @param force             Overwrite existing file.
 *
 * @return eOk on success, else eError.
 */
tResult copySingleFile(LPCTSTR currentSourcePath, LPCTSTR currentTargetPath, tBool force) {
   if (Debug) {
      printOut(_T("Copy file %s to %s\n"), getReadableFilename(currentSourcePath), getReadableFilename(currentTargetPath));
   }
   if (CopyFile(currentSourcePath, currentTargetPath, force ? 0 : 1) == 0) {
      return printLastError(_T("Can't copy file %s to %s"), getReadableFilename(currentSourcePath), getReadableFilename(currentTargetPath));
   }

   return eOk;
}

/*!
 * Move a single file.
 *
 * @param currentSourcePath The file to copy.
 * @param currentTargetPath The target file.
 * @param force             Overwrite existing file.
 *
 * @return eOk on success, else eError.
 */
tResult moveSingleFile(LPCTSTR currentSourcePath, LPCTSTR currentTargetPath, tBool force) {
   if (Debug) {
      printOut(_T("Move file %s to %s\n"), getReadableFilename(currentSourcePath), getReadableFilename(currentTargetPath));
   }
   if (MoveFileEx(currentSourcePath, currentTargetPath, MOVEFILE_COPY_ALLOWED | (force ? MOVEFILE_REPLACE_EXISTING : 0)) == 0) {
      return printLastError(_T("Can't move file %s to %s"), getReadableFilename(currentSourcePath), getReadableFilename(currentTargetPath));
   }

   return eOk;
}

/*!
 * Touch a file.
 *
 * @param currentPath     The file to touch.
 * @param localTime       The time to use, NULL to use current time.
 * @param createIfMissing If eOk, the file is created if it doesn't exist.
 *
 * @return eOk if the file was successfully touched, else eError.
 */
tResult touchSingleFile(LPCTSTR currentPath, tBool createIfMissing) {
   HANDLE hFile;

   if (Debug) {
      printOut(_T("Touch file %s\n"), getReadableFilename(currentPath));
   }
   hFile = CreateFile(currentPath, FILE_WRITE_ATTRIBUTES, 0, NULL, createIfMissing ? OPEN_ALWAYS : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
   if (hFile == INVALID_HANDLE_VALUE) {                                                      // GCOVR_EXCL_BR_LINE
      return printLastError(_T("Can't get handle to %s"), getReadableFilename(currentPath)); // GCOVR_EXCL_LINE
   } else {
      LPFILETIME pft = ptr_file_time;
      // Use current system time if not defined in command line
      if (pft == NULL) {
         static FILETIME ft;
         static SYSTEMTIME st;
         pft = &ft;
         GetSystemTime(&st);
         // Converts the current system time to file time format
         if (SystemTimeToFileTime(&st, &ft) == 0) {                              // GCOVR_EXCL_BR_LINE
            return printLastError(_T("Can't convert system time to file time")); // GCOVR_EXCL_LINE
         }
      }
      if (SetFileTime(hFile, (LPFILETIME)NULL, pft, pft) == 0) {                                               // GCOVR_EXCL_BR_LINE
         return printLastError(_T("Can't set access and write time of %s"), getReadableFilename(currentPath)); // GCOVR_EXCL_LINE
      }
      if (CloseHandle(hFile) == 0) {                                                              // GCOVR_EXCL_BR_LINE
         return printLastError(_T("Can't close handle to %s"), getReadableFilename(currentPath)); // GCOVR_EXCL_LINE
      }
   }

   return eOk;
}

/*!
 * Open the given path for reading.
 *
 * @param currentPath The file to open.
 * @param handle      A pointer to write the opened handle to.
 *
 * @return eOk if the file was opened successfully, also setting argument
 * handle, else eError.
 */
tResult openFile(LPCTSTR currentPath, HANDLE *handle) {
   IoBuffer[0] = '\0';

   *handle = CreateFile(currentPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
   if (*handle == INVALID_HANDLE_VALUE) {
      // GCOVR_EXCL_START
      return printLastError(_T("Can't get handle to %s"), getReadableFilename(currentPath));
      // GCOVR_EXCL_STOP
   }

   return eOk;
}

/*!
 * Close the handle to the file.
 *
 * @param currentPath The file to close (needed for error message).
 * @param handle      The handle to close.
 *
 * @return The handle or INVALID_HANDLE_VALUE if open wasn't successful.
 */
tResult closeFile(LPCTSTR currentPath, HANDLE *handle) {
   if (CloseHandle(handle) == 0) {                                                             // GCOVR_EXCL_BR_LINE
      return printLastError(_T("Can't close handle to %s"), getReadableFilename(currentPath)); // GCOVR_EXCL_LINE
   }

   return eOk;
}

/*!
 * Check if the option is --time.
 *
 * @param argv The current argv pointer.
 *
 * @return eTrue if --time option else, else eError
 */
tBool isTimeOption(wchar_t *argv) { return _tcsncmp(argv, _T("--time"), 6) == 0; }

/*!
 * Check if the option is --time.
 *
 * @param argv The current argv pointer.
 *
 * @return eOk if the file was opened successfully, also setting argument
 * handle, else eError.
 */
tResult storeTimeValue(int *argc, wchar_t **argv[]) {
   LPCTSTR localTime = NULL;
   if (_tcsncmp(**argv, _T("--time="), 7) == 0) {
      localTime = &(**argv)[7];
   } else {
      if (*argc <= 1) {
         return printErr(_T("Option %s needs an argument.\n"), **argv);
      }
      localTime = *(++*argv);
      --*argc;
   }

   if (Debug) {
      printOut(_T("Converting time string '%s'\n"), localTime);
   }

   SYSTEMTIME lt, st;
   int count = swscanf(localTime, TIME_FORMAT, &lt.wYear, &lt.wMonth, &lt.wDay, &lt.wHour, &lt.wMinute, &lt.wSecond);
   if (count < 3) {
      return printErr(_T("Wrong format for time %s, expected yyyy-mm-dd[Thh:mm[:ss]].\n"), localTime);
   }
   if (count == 3) {
      lt.wHour = 12;
      lt.wMinute = 0;
      lt.wSecond = 0;
   } else if (count == 5) {
      lt.wSecond = 0;
   } else if (count != 6) {
      return printErr(_T("Wrong format for time %s, expected yyyy-mm-dd[Thh:mm[:ss]].\n"), localTime);
   }
   lt.wMilliseconds = 0;
   if (TzSpecificLocalTimeToSystemTime((TIME_ZONE_INFORMATION *)NULL, &lt, &st) == 0) {   // GCOVR_EXCL_BR_LINE
      return printLastError(_T("Can't convert local time %s to system time"), localTime); // GCOVR_EXCL_LINE
   }

   static FILETIME ft;
   // Converts the current system time to file time format
   if (SystemTimeToFileTime(&st, &ft) == 0) {                              // GCOVR_EXCL_BR_LINE
      return printLastError(_T("Can't convert system time to file time")); // GCOVR_EXCL_LINE
   }

   ptr_file_time = &ft;

   return eOk;
}

/*!
 * Run a command for each argument. If the argument starts with a @ the
 * file is opened and read line by line, running the given command for each
 * line.
 *
 * @param argc    The number of arguments.
 * @param argv    The arguments.
 * @param command The pointer to the function which shall be executed for each
 * file.
 *
 * @return eOk on success, else eError
 */
tResult runCommandForEachInputLine(int argc, wchar_t *argv[], tResult (*command)(void)) {
   tResult result = eOk;
   while ((result == eOk) && (argc-- != 0)) {
      TCHAR *filename = *(argv++);
      if (filename[0] == _T('@')) {
         createDosDevicePath(&filename[1], DosDevicePathResponseFile);
         HANDLE InHandle;
         result &= openFile(DosDevicePathResponseFile, &InHandle);
         if (result == eOk) {
            DWORD BytesRead = 0;
            *IoBuffer = '\0';
            *IoBufferWideCharacter = _T('\0');
            do {
               // Start behind the current content
               DWORD Offset = _tcslen(IoBufferWideCharacter);
               if (FALSE == ReadFile(InHandle, IoBuffer, SIZE_IO_BUFFER - Offset - 1, &BytesRead, NULL)) { // GCOVR_EXCL_BR_LINE
                  result &= printLastError(_T("Error reading file %s"), DosDevicePathResponseFile);        // GCOVR_EXCL_LINE
               }
               if (BytesRead != 0) {
                  // Add a \0 after the read content
                  IoBuffer[BytesRead] = '\0';
                  // ...and convert it.
                  if (MultiByteToWideChar(CP_ACP, 0, IoBuffer, -1, &IoBufferWideCharacter[Offset], SIZE_IO_BUFFER - Offset) // GCOVR_EXCL_BR_LINE
                      == 0) {
                     result &= printLastError(_T("Error converting file content of %s"), DosDevicePathResponseFile); // GCOVR_EXCL_LINE
                  } else {
                     LPTSTR PtrStart = IoBufferWideCharacter;
                     LPTSTR PtrEnd = &IoBufferWideCharacter[BytesRead + Offset];
                     do {
                        LPTSTR PtrEndOfLine = _tcschr(PtrStart, _T('\n'));
                        if (PtrEndOfLine != NULL) {
                           LPTSTR PtrStartNextLine = &PtrEndOfLine[1];
                           *PtrEndOfLine = _T('\0');
                           do {
                              --PtrEndOfLine;
                              if ((*PtrEndOfLine == _T('\r')) || (*PtrEndOfLine == _T(' '))) {
                                 *PtrEndOfLine = _T('\0');
                              }
                           } while ((*PtrEndOfLine == _T('\0')) && (PtrEndOfLine > PtrStart));
                           // If line isn't empty
                           if (PtrEndOfLine > PtrStart) {
                              createDosDevicePath(PtrStart, DosDevicePath);
                              result &= command();
                           }
                           // Clear this line (needed if last char in buffer is a
                           // linebreak
                           *PtrStart = _T('\0');
                           PtrStart = PtrStartNextLine;
                        } else {
                           // Move the rest of the string to the start of the buffer
                           DWORD Index = _tcslen(PtrStart) + 1;
                           for (; Index > 0; --Index) {
                              IoBufferWideCharacter[Index] = PtrStart[Index];
                           }
                           IoBufferWideCharacter[0] = PtrStart[0];
                           PtrStart = PtrEnd;
                        }
                     } while ((result == eOk) && (PtrStart < PtrEnd));
                  }
               }
            } while ((result == eOk) && (BytesRead != 0));
            result &= closeFile(DosDevicePathResponseFile, InHandle);
         }
      } else {
         createDosDevicePath(filename, DosDevicePath);
         result &= command();
      }
   }

   return result;
}

/*!
 * Check if the last part of the input is unique.
 *
 * @return eOk if everything is unique, else eError
 */
tResult checkUniqueName(void) {
   tResult result = eOk;

   // Get the filename and make it lowercase
   LPTSTR PtrFilenameStart = _tcsrchr(DosDevicePath, _T('\\')) + 1;
   (void)_tcslwr(PtrFilenameStart);
   TCHAR **CurrentFilename = KnownFilenames;
   while (*CurrentFilename != NULL) {
      if (_tcscmp(PtrFilenameStart, *CurrentFilename) == 0) {
         result &= printErr(_T("File in source list will overwrite each other: %s"), PtrFilenameStart);
         break;
      }
      ++CurrentFilename;
   }
   if (result == eOk) {
      *CurrentFilename = malloc(_tcslen(PtrFilenameStart) * sizeof(TCHAR *));
      if (*CurrentFilename == NULL) {                                                 // GCOVR_EXCL_BR_LINE
         result &= printLastError(_T("Can't allocate memory for current filenames")); // GCOVR_EXCL_LINE
      }
      _tcscpy(*CurrentFilename, PtrFilenameStart);
   }

   return result;
}

tResult checkUniqueNames(int argc, wchar_t *argv[]) {
   tResult result = eOk;
   size_t ListSize = (argc + 1) * sizeof(TCHAR *); // +1 to have a trailing NULL pointer
                                                   // (needed for freeing the space)

   KnownFilenames = malloc(ListSize);
   if (KnownFilenames == NULL) {                                                   // GCOVR_EXCL_BR_LINE
      result &= printLastError(_T("Can't allocate memory for list of filenames")); // GCOVR_EXCL_LINE
   } else {
      memset(KnownFilenames, 0, ListSize);

      result &= runCommandForEachInputLine(argc, argv, checkUniqueName);

      TCHAR **CurrentFilename = KnownFilenames;
      while (*CurrentFilename != NULL) {
         free(*CurrentFilename);
         if (errno != 0) {                                                 // GCOVR_EXCL_BR_LINE
            result &= printLastError(_T("Can't free memory of filename")); // GCOVR_EXCL_LINE
         }
         ++CurrentFilename;
      }
      free(KnownFilenames);
      if (errno != 0) {                                                          // GCOVR_EXCL_BR_LINE
         result &= printLastError(_T("Can't free memory of list of filenames")); // GCOVR_EXCL_LINE
      }
   }

   return result;
}

/*!
 * Print a file to a given handle.
 *
 * @param outHandle The handle to which the file is printed.
 *
 * @return eOk on success, else eError.
 */
tResult printFileToHandle(LPCTSTR currentPath, HANDLE *outHandle) {
   tResult result = eOk;

   HANDLE InHandle;
   result &= openFile(currentPath, &InHandle);
   if (result == eOk) {
      DWORD BytesRead = 0;
      do {
         if (FALSE == ReadFile(InHandle, IoBuffer, SIZE_IO_BUFFER, &BytesRead, NULL)) {              // GCOVR_EXCL_BR_LINE
            result &= printLastError(_T("Error reading file %s"), getReadableFilename(currentPath)); // GCOVR_EXCL_LINE
         }
         if (BytesRead != 0) {
            DWORD BytesWritten = 0;
            if (WriteFile(outHandle, IoBuffer, BytesRead, &BytesWritten, NULL) == 0) { // GCOVR_EXCL_BR_LINE
               result &= printLastError(_T("Error writing to output handle"));         // GCOVR_EXCL_LINE
            }
         }
      } while ((result == eOk) && (BytesRead != 0));
      result &= closeFile(currentPath, InHandle);
   }

   return result;
}
