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

#include "cpplexer_decl.h"
#include "version.h"

#if defined(_WIN32)
#define stricmp _stricmp
#define chdir _chdir
#define getcwd _getcwd
#else
#define stricmp strcasecmp
#define _MAX_PATH 4096
#endif

//
// Platform-specific directory name separator
//
#if defined(_WIN32)
#define DIRSEPARATORSTRING "\\"
#else
#define DIRSEPARATORSTRING "/"
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

// storage for the current directory if none was specified on the command line
static char cur_dir[_MAX_PATH];

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
bool ProcessDirectory(const char *dirname);
bool EnumCurrentDir(std::set<std::string>& files, std::set<std::string>& subdirs);
std::string& GetFullPath(const std::list<std::string>& pathlist, std::string& path);
void ProcessFileList(const std::string& dirname, const std::set<std::string>& files);

//
//
//

bool ParseSourceFile(const char *filename)
{
   int linecnt = 0, cmntcnt = 0, cppcnt = 0, ccnt = 0, codecnt = 0, bracecnt = 0, emptycnt = 0;
   int token1;
   FILE *srcfile;

   if(filename == NULL)
      return false;

   srcfile = fopen(filename, "r");

   if(srcfile == NULL) {
      printf("Cannot open file %s (%s)\n", filename, _sys_errlist[errno]);
      return false;
   }

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
      printf("   %5d  %5d      %5d  %10s  %5d  %5d  %s\n", linecnt, codecnt, cmntcnt, cpp_c_cnt, emptycnt, bracecnt, filename);
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

   return true;
}

void ProcessFileList(const std::string& dirname, const std::set<std::string>& files)
{
   bool header = false;
   const char *filename, *cptr;
   std::set<std::string>::const_iterator iter;
   int filecnt = 0;

   if(files.size() == 0)
      return;

   for(iter = files.begin(); iter != files.end(); iter++) {
      if((filename = (*iter).c_str()) == NULL)
         continue;

      if((cptr = strrchr(filename, '.')) == NULL)
         continue;

      if(ExtList.find(++cptr) != ExtList.end()) {
         if(VerboseOutput) {
            if(!header) {
               printf("Directory: %s\n\n", dirname.c_str());
               printf("   Lines   Code  Commented     (C++/C)  Empty  Brace\n");
               printf("  ------ ------ ---------- ----------- ------ ------\n");
               header = true;
            }

            filecnt++;
         }
         ParseSourceFile(filename);
         FileCount++;
      }
   }

   if(VerboseOutput && filecnt)
      printf("\n");
}

bool ProcessDirList(std::set<std::string>& dirs)
{
   std::string dirname, temp;
   std::stack<std::set<std::string>*> stack;
   std::set<std::string>::iterator iter;
   std::set<std::string> files;
   std::list<std::string> pathlist;

   std::set<std::string> *subdirs = &dirs;

   iter = subdirs->begin();

   while(iter != subdirs->end()) {

      DirCount++;

      dirname = *iter;
      if(chdir(dirname.c_str()) != 0) {
         printf("Can't change directory to %s\n", dirname.c_str());
         return false;
      }
      pathlist.push_back(dirname);

      //
      // When saving the current directory list's state, instead of saving
      // both, the iterator and the list, remove the current item and save
      // only the list. 
      //
      subdirs->erase(iter);
      stack.push(subdirs);
      subdirs = new std::set<std::string>;

      //
      // Populate the list with directories and process the files in
      // the current directory. 
      //
      EnumCurrentDir(files, *subdirs);
      ProcessFileList(GetFullPath(pathlist, temp), files);
      files.clear();

      //
      // If the new directory list is empty, delete the list and then
      // delete every empty list on top of the stack, moving one directory
      // every time. Note that the top directory wasn't allocated by this
      // function and shouldn't be deleted. 
      //
      while(subdirs->empty()) {
         if(stack.empty())
            return true;

         delete subdirs;

         subdirs = stack.top();
         stack.pop();
         chdir("..");
         pathlist.pop_back();
      }

      iter = subdirs->begin();
   }

   return true;
}

#if defined(_WIN32)
bool EnumCurrentDir(std::set<std::string>& files, std::set<std::string>& subdirs)
{
   struct _finddata_t fileinfo;
   long fhandle;

   files.clear();
   subdirs.clear();

   if((fhandle = _findfirst("*.*", &fileinfo)) == -1)
      return false;

   do {
      if(fileinfo.attrib & _A_SUBDIR) {
         if(!strcmp(fileinfo.name, ".") || !strcmp(fileinfo.name, "..")) 
            continue;

         if(*fileinfo.name)
            subdirs.insert(fileinfo.name);
         continue;
      }

      if(*fileinfo.name)
         files.insert(fileinfo.name);

   } while(_findnext(fhandle, &fileinfo) == 0);

   _findclose(fhandle);

   return true;
}
#else
bool EnumCurrentDir(std::set<std::string>& files, std::set<std::string>& subdirs)
{
   DIR *dir;
   struct dirent *entry;
   struct stat statinfo;

   files.clear();
   subdirs.clear();

   if((dir = opendir(".")) == NULL) 
      return false;

   while ((entry = readdir(dir)) != NULL) {

      if(stat(entry->d_name, &statinfo) == -1)
         return false;

      if(S_ISDIR(statinfo.st_mode)) {
         if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) 
            continue;

         if(*entry->d_name)
            subdirs.insert(entry->d_name);
         continue;
      }

      if(*entry->d_name)
         files.insert(entry->d_name);

   }

   closedir(dir);

   return true;
}
#endif

bool ProcessDirectory(const char *dirname)
{
   std::set<std::string> files;
   std::set<std::string> subdirs;

   if(!dirname || !*dirname)
      throw std::runtime_error("Directory name cannot be empty");

   EnumCurrentDir(files, subdirs);

   ProcessFileList(dirname, files);

   if(WalkTree) {
      if(ProcessDirList(subdirs) == false)
         return false;
   }

   return true;
}

std::string& GetFullPath(const std::list<std::string>& pathlist, std::string& path)
{
   std::list<std::string>::const_iterator iter;

   path.erase();

   for(iter = pathlist.begin(); iter != pathlist.end(); iter++) {
      path += *iter;
      path += DIRSEPARATORSTRING;
   }

   return path;
}

void PrintCopyrightLine(void)
{
   printf("Copyright 2003-2015, Stone Steps Inc. http://www.stonesteps.ca\n\n");
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

void PrintFileExtensions(const char *fmt, const std::set<std::string, less_stricmp>& extlist)
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

   printf(fmt, extstr.c_str());

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
                     ExtList.insert("hpp");
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
      PrintFileExtensions("Processing files with extensions %s\n\n", ExtList);

      // use the current directory if none was provided on the command line
      if(!dirname || !*dirname)   {
         if(!getcwd(cur_dir, sizeof(cur_dir))) {
            printf("Cannot obtain the current working directory\n");
            exit(2);
         }
         dirname = cur_dir;
      }

      if(ProcessDirectory(dirname) == false) 
         exit(2);

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

