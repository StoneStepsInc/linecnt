/*
    linecnt - a source line counting utility

    Copyright (c) 2003-2020, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information
*/
#if defined(_WIN32)
#include <io.h>
#include <direct.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <list>
#include <stack>
#include <set>
#include <string>
#include <stdexcept>
#include <system_error>

#include "cpplexer_decl.h"
#include "version.h"

#if defined(_WIN32)
#define stricmp _stricmp
#define getcwd _getcwd
#else
#define stricmp strcasecmp
#define _MAX_PATH 4096
#endif

//
// Platform-specific directory name separator
//
#if defined(_WIN32)
#define DIRSEP "\\"
#else
#define DIRSEP "/"
#endif

//
// Case-insensitive string comparison function
//
struct less_stricmp {
   bool operator () (const std::string& str1, const std::string& str2) const
   {
      return stricmp(str1.c_str(), str2.c_str()) < 0 ? true : false;
   }
};

//
// Various counters
//
static int FileCount = 0;                 // total file count
static int DirCount = 1;                  // total directory count
static int LineCount = 0;                 // total source line count
static int CppLineCount = 0;              // C++ -commented line count
static int CLineCount = 0;                // C-commented line count
static int CommentCount = 0;              // commented line count
static int EmptyLineCount = 0;            // empty line count
static int BraceLineCount = 0;            // brace-only line count
static int CodeLineCount = 0;             // code line count

//
// Run flags
//
static bool VerboseOutput = false;
static bool WalkTree = false;

// a set of case-insensitive file extensions to process
static std::set<std::string, less_stricmp>   ExtList;

//
//
//
bool ProcessDirectory(const std::string& dirname);
void EnumDirectory(const std::string& dirname, std::list<std::string>& files, std::list<std::string>& subdirs);
void ProcessFileList(const std::string& dirname, const std::list<std::string>& files);

//
//
//

void ParseSourceFile(const std::string& dirname, const std::string& filename)
{
   int linecnt = 0, cmntcnt = 0, cppcnt = 0, ccnt = 0, codecnt = 0, bracecnt = 0, emptycnt = 0;
   int token1;
   FILE *srcfile;

   srcfile = fopen((dirname + DIRSEP + filename).c_str(), "r");

   if(srcfile == NULL)
      throw std::system_error(errno, std::system_category(), filename);

   //
   // Initialize Flex's input
   //
   CppFlexLexer lexer1(srcfile);

   //
   // Run the source file through Flex
   //
   linecnt = 0; 
   if((token1 = lexer1.yylex()) != TOKEN_EOF) {
      do {
         switch (token1) {
            case TOKEN_EMPTY_LINE:
            case TOKEN_EMPTY_LINE + TOKEN_EOF:
               linecnt++;
               emptycnt++;
               break;
            case TOKEN_BRACE_LINE:
            case TOKEN_BRACE_LINE + TOKEN_EOF:
               linecnt++;
               bracecnt++;
               break;
            case TOKEN_CODE_EOL:
            case TOKEN_CODE_EOL + TOKEN_EOF:
               linecnt++;
               codecnt++;
               break;
            case TOKEN_C_COMMENT_EOL:
            case TOKEN_C_COMMENT_EOL + TOKEN_EOF:
               linecnt++;
               ccnt++;
               cmntcnt++;
               break;
            case TOKEN_CPP_COMMENT_EOL:
            case TOKEN_CPP_COMMENT_EOL + TOKEN_EOF:
               linecnt++;
               cppcnt++;
               cmntcnt++;
               break;
            case TOKEN_C_CPP_COMMENT_EOL:
            case TOKEN_C_CPP_COMMENT_EOL + TOKEN_EOF:
               linecnt++;
               cppcnt++;
               ccnt++;
               cmntcnt++;
               break;
            case TOKEN_CODE_C_COMMENT_EOL:
            case TOKEN_CODE_C_COMMENT_EOL + TOKEN_EOF:
               linecnt++;
               ccnt++;
               codecnt++;
               cmntcnt++;
               break;
            case TOKEN_CODE_CPP_COMMENT_EOL:
            case TOKEN_CODE_CPP_COMMENT_EOL + TOKEN_EOF:
               linecnt++;
               cppcnt++;
               codecnt++;
               cmntcnt++;
               break;
            case TOKEN_CODE_C_CPP_COMMENT_EOL:
            case TOKEN_CODE_C_CPP_COMMENT_EOL + TOKEN_EOF:
               linecnt++;
               ccnt++;
               cppcnt++;
               codecnt++;
               cmntcnt++;
               break;
            default:
               printf("Unknown token: %s at %d\n", lexer1.YYText(), LineCount+linecnt);
               break;
         }
      } while(token1 < TOKEN_EOF && ((token1 = lexer1.yylex()) != TOKEN_EOF));
   }

   if(VerboseOutput) {
      char cpp_c_cnt[32];
      // make a shared column for C and C++ commented line counts
      sprintf(cpp_c_cnt, "%d/%d", cppcnt, ccnt); 
      printf("   %5d  %5d      %5d  %10s  %5d  %5d  %s\n", linecnt, codecnt, cmntcnt, cpp_c_cnt, emptycnt, bracecnt, filename.c_str());
   }

   EmptyLineCount += emptycnt;
   BraceLineCount += bracecnt;
   LineCount += linecnt;
   CodeLineCount += codecnt;
   CppLineCount += cppcnt;
   CLineCount += ccnt;
   CommentCount += cmntcnt;

   //
   //   Clean up
   //
   fclose(srcfile);
}

void ProcessFileList(const std::string& dirname, const std::list<std::string>& files)
{
   bool header = false;
   int filecnt = 0;

   if(files.size() == 0)
      return;

   for(const std::string& filename : files) {
      if(VerboseOutput) {
         if(!header) {
            printf("Directory: %s\n\n", dirname.c_str());
            printf("   Lines   Code  Commented     (C++/C)  Empty  Brace\n");
            printf("  ------ ------ ---------- ----------- ------ ------\n");
            header = true;
         }

         filecnt++;
      }

      ParseSourceFile(dirname, filename);
      FileCount++;
   }

   if(VerboseOutput && filecnt)
      printf("\n");
}

void ProcessDirList(const std::string& basedir, std::list<std::string>&& dirs)
{
   // processing state of a directory
   struct state_t {
      std::list<std::string>  subdirs;    // remaining subdirectories
      std::string             dirname;    // directory that is being processed
   };

   std::string dirpath = basedir;
   std::stack<state_t> stack;
   std::list<std::string>::iterator iter;

   stack.push({std::move(dirs), dirpath});

   std::list<std::string> *subdirs = &stack.top().subdirs;

   iter = subdirs->begin();

   while(iter != subdirs->end()) {

      DirCount++;

      // add the new directory to the current path
      dirpath += DIRSEP + *iter;

      // move the new directory name into the new state
      stack.push({std::list<std::string>(), std::move(*iter)});

      // and remove the empty directory node from the state list
      subdirs->erase(iter);

      // and assign the pointer to the new directory list
      subdirs = &stack.top().subdirs;

      // populate the directory list and the file list for the new directory
      std::list<std::string> files;
      EnumDirectory(dirpath, files, *subdirs);

      // and process all files in the current directory
      ProcessFileList(dirpath, files);
      files.clear();

      // pop all empty directory lists from the stack
      while(subdirs->empty()) {
         // chop off the current directory from the path, along with the separator
         dirpath.erase(dirpath.length() - stack.top().dirname.length() - 1);

         stack.pop();

         if(stack.empty())
            return;

         // check if more directories to process in the parent directory
         subdirs = &stack.top().subdirs;

      }

      iter = subdirs->begin();
   }
}

void EnumDirectory(const std::string& dirname, std::list<std::string>& files, std::list<std::string>& subdirs)
{
   files.clear();
   subdirs.clear();

#if defined(_WIN32)
   struct _finddata_t fileinfo;
   long fhandle;

   // get all files and directories, except current and parent directories
   std::string dirpat(dirname + DIRSEP + "*.*");

   if((fhandle = _findfirst(dirpat.c_str(), &fileinfo)) == -1)
      throw std::runtime_error("Cannot enumerate directory: " + dirname);

   do {
      if(fileinfo.attrib & _A_SUBDIR) {
         // skip any directory that starts with a period (e.g. ".", "..", ".git", ".vs", etc)
         if(*fileinfo.name == '.')
            continue;

         if(*fileinfo.name)
            subdirs.push_back(fileinfo.name);
         continue;
      }

      if(*fileinfo.name) {
         const char *ext = strrchr(fileinfo.name, '.');

         if(ext && ExtList.find(++ext) != ExtList.end())
            files.push_back(fileinfo.name);
      }

   } while(_findnext(fhandle, &fileinfo) == 0);

   _findclose(fhandle);
#else
   DIR *dir;
   struct dirent *entry;
   struct stat statinfo;

   if((dir = opendir(dirname.c_str())) == NULL) 
      throw std::runtime_error("Cannot open directory: " + dirname);

   while ((entry = readdir(dir)) != NULL) {

      if(stat((dirname + DIRSEP + entry->d_name).c_str(), &statinfo) == -1)
         throw std::runtime_error("Cannot stat directory: " + dirname);

      if(S_ISDIR(statinfo.st_mode)) {
         // skip any directory that starts with a period (e.g. ".", "..", ".git", ".vs", etc)
         if(*entry->d_name == '.')
            continue;

         if(*entry->d_name)
            subdirs.push_back(entry->d_name);
         continue;
      }

      if(*entry->d_name) {
         const char *ext = strrchr(entry->d_name, '.');

         if(ext && ExtList.find(++ext) != ExtList.end())
            files.push_back(entry->d_name);
      }

   }

   closedir(dir);
#endif
}

void ProcessDirectory(const std::string& dirname)
{
   std::list<std::string> files;
   std::list<std::string> subdirs;

   EnumDirectory(dirname, files, subdirs);

   ProcessFileList(dirname, files);

   if(WalkTree)
      ProcessDirList(dirname, std::move(subdirs));
}

void PrintCopyrightLine(void)
{
   printf("Copyright 2003-2020, Stone Steps Inc. http://www.stonesteps.ca\n\n");
}

void PrintUsage(void)
{
   printf("Syntax: linecnt [-s] [-v] [-d dir-name] [-c] [-j] [ext [ext [ ...]]]\n\n");

   printf("  -s    Process files in the current directory and all subdirectories\n");
   printf("  -v    Produce verbose output\n");
   printf("  -d    Start in the specified directory\n");
   printf("  -c    Add common C/C++ extensions to the list\n");
   printf("  -j    Add common Java extensions to the list\n");
   printf("  -V    Print version information\n");
   printf("  -W    Print warranty information\n");
   printf("  -h    Print this help\n");
   printf("\n");

   printf("Examples:\n");
   printf("  linecnt cpp c h    ; Count lines in .cpp, .c and .h files\n");
   printf("  linecnt -c -j inc  ; Count lines in C/C++, Java and .inc files\n");
}

std::string GetFileExtensions(const std::set<std::string, less_stricmp>& extlist)
{
   std::string extstr;
   std::set<std::string, less_stricmp>::const_iterator iter;
   int extcnt = 1;

   for(iter = extlist.begin(); iter != extlist.end(); extcnt++, iter++) {
      if(extcnt > 1) 
         extstr += (extcnt == extlist.size()) ? " and " : ", ";

      extstr += ".";
      extstr += (*iter).c_str();
   }

   return extstr;
}

void PrintVersion(void)
{
   printf("Version: %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, EDITION_LEVEL);
}

void PrintWarranty(void)
{
   printf("THIS PROGRAM IS DISTRIBUTED ON AN AS-IS BASIS, WITHOUT WARRANTY\n");
   printf("OF ANY KIND, EITHER EXPRESSED OR IMPLIED. USE THIS PROGRAM AT\n");
   printf("YOUR OWN RISK.\n");
}

//
//
//
int main(int argc, const char *argv[])
{
   const char *dirname = NULL;
   const char * const *argptr = &argv[0];
   int comments = 0;

   try {
      PrintCopyrightLine();

      if(argc > 1) {
         // skip the executable's name
         argptr++;

         while(*argptr) {
            if(strchr("-/", **argptr)) {
               switch((*argptr)[1]) {
                  case 'd':
                     // check if a directory follows -d without a space
                     if(*(*argptr+2))
                        dirname = *argptr+2;
                     else {
                        // otherwise check the next argument
                        dirname = *(++argptr);

                        // make sure we have a directory
                        if(!dirname) {
                           printf("You must supply a directory to start in\n");
                           exit(1);
                        }
                     }
                     break;
                  case 's':
                     WalkTree = true;
                     break;
                  case 'c':
                     ExtList.insert("cpp");
                     ExtList.insert("cxx");
                     ExtList.insert("cc");
                     ExtList.insert("c++");
                     ExtList.insert("hpp");
                     ExtList.insert("hxx");
                     ExtList.insert("h++");
                     ExtList.insert("h");
                     ExtList.insert("c");
                     break;
                  case 'j':
                     ExtList.insert("java");
                     break;
                  case 'v':
                     VerboseOutput = true;
                     break;
                  case 'V':
                     PrintVersion();
                     exit(0);
                  case 'W':
                     PrintWarranty();
                     exit(0);
                  case 'h':
                  case '?':
                     PrintUsage();
                     exit(0);
                  default:
                     printf("Unknown option: %s\n\n", *argptr);
                     PrintUsage();
                     exit(1);
               }
               argptr++;
               continue;
            }

            ExtList.insert(*argptr++);
         }
      }

      if(ExtList.size() == 0) {
         printf("The extension list is empty. At least one extension must be specified.\n\n");
         PrintUsage();
         exit(1);
      }

      //
      //
      //
      printf("Processing files with extensions %s\n\n", GetFileExtensions(ExtList).c_str());

      // use the current directory if none was provided on the command line
      if(!dirname || !*dirname)   {
         char cur_dir[_MAX_PATH];
         if(!getcwd(cur_dir, sizeof(cur_dir))) {
            printf("Cannot obtain the current working directory\n");
            exit(2);
         }
         dirname = cur_dir;
      }

      if(!dirname || !*dirname)
         throw std::runtime_error("Directory name cannot be empty");

      ProcessDirectory(dirname);

      //
      //
      //
      printf("\n");
      printf("Processed %d files in %d directories\n", FileCount, DirCount);

      if(LineCount) {
         printf("\n");
         printf("Total lines            : %d\n", LineCount);
         printf("Code lines             : %d\n", CodeLineCount);
         printf("Commented lines        : %d (C++: %d; C: %d)\n", CommentCount, CppLineCount, CLineCount);
         printf("Empty Lines            : %d\n", EmptyLineCount);
         printf("Brace Lines            : %d\n", BraceLineCount);
      }

      if(CommentCount)
         printf("Code/comments ratio    : %.2f\n", (double) CodeLineCount/CommentCount);

      if(FileCount) {
         printf("Source lines per file  : %.2f\n", (double) LineCount/FileCount);
         printf("Code lines per file    : %.2f\n", (double) CodeLineCount/FileCount);
         printf("Comment lines per file : %.2f\n", (double) CommentCount/FileCount);
      }

      printf("\n");

      return 0;
   }
   catch (const std::exception& ex) {
      printf("Unexpected error (%s)\n", ex.what());
      return 1;
   }
}

