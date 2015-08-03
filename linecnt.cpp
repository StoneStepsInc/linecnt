#if defined(_WIN32)
#define STRICT
#include <windows.h>
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

#include "cpplexer_decl.h"

#if defined(_WIN32)
#define stricmp _stricmp
#define chdir _chdir
#define getcwd _getcwd
#else
#define stricmp strcasecmp
#define _MAX_PATH 4096
#endif

//
//
//

#define LINECNT_VERSION_TEXT      "2.0"

#if defined(_WIN32)
#define DIRSEPARATORSTRING "\\"
#else
#define DIRSEPARATORSTRING "/"
#endif

//
//
//

struct less_stricmp {
   bool operator () (const std::string& str1, const std::string& str2) const
   {
      return stricmp(str1.c_str(), str2.c_str()) < 0 ? true : false;
   }
};

//
//
//

static int FileCount = 0;
static int DirCount = 1;
static int LineCount = 0;
static int CppLineCount = 0;
static int CLineCount = 0;
static int CommentCount = 0;
static int EmptyLineCount = 0;
static int BraceLineCount = 0;
static int CodeLineCount = 0;

static bool VerboseOutput = false;
static bool WalkTree = false;

static std::set<std::string, less_stricmp>   ExtList;

//
//
//

bool ProcessDirectory(const char *dirname);
bool EnumCurrentDir(std::set<std::string, less_stricmp>& files, std::set<std::string, less_stricmp>& subdirs);
std::string& GetFullPath(const std::list<std::string>& pathlist, std::string& path);
void ProcessFileList(const std::string& dirname, const std::set<std::string, less_stricmp>& files);

//
//
//

bool ParseSourceFile(const char *filename)
{
   int linecnt = 0, cppcnt = 0, ccnt = 0, codecnt = 0, bracecnt = 0, emptycnt = 0;
   int token1;
   FILE *srcfile;
   char temp;

   if(filename == NULL)
      return false;

   srcfile = fopen(filename, "r");

   if(srcfile == NULL) {
      printf("Cannot open file %s (%s)\n", filename, _sys_errlist[errno]);
      return false;
   }

   //
   //   Initialize Flex's input
   //
   CppFlexLexer lexer1(srcfile);

   //
   //   Check if the file's empty
   //
   if(fread(&temp, 1, 1, srcfile) == -1)
      return false;

   if(feof(srcfile) == false) {
      rewind(srcfile);

      //
      //   Parse the input
      //
      linecnt = 0; 
      if((token1 = lexer1.yylex()) != TOKEN_EOF) {
         do {
            switch (token1) {
               case TOKEN_EMPTY_LINE:
                  linecnt++;
                  emptycnt++;
                  break;
               case TOKEN_BRACE_LINE:
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
                  break;
               case TOKEN_CPP_COMMENT_EOL:
               case TOKEN_CPP_COMMENT_EOL + TOKEN_EOF:
                  linecnt++;
                  cppcnt++;
                  break;
               case TOKEN_C_CPP_COMMENT_EOL:
               case TOKEN_C_CPP_COMMENT_EOL + TOKEN_EOF:
                  linecnt++;
                  cppcnt++;
                  ccnt++;
                  break;
               case TOKEN_CODE_C_COMMENT_EOL:
               case TOKEN_CODE_C_COMMENT_EOL + TOKEN_EOF:
                  linecnt++;
                  ccnt++;
                  codecnt++;
                  break;
               case TOKEN_CODE_CPP_COMMENT_EOL:
               case TOKEN_CODE_CPP_COMMENT_EOL + TOKEN_EOF:
                  linecnt++;
                  cppcnt++;
                  codecnt++;
                  break;
               case TOKEN_CODE_C_CPP_COMMENT_EOL:
               case TOKEN_CODE_C_CPP_COMMENT_EOL + TOKEN_EOF:
                  linecnt++;
                  ccnt++;
                  cppcnt++;
                  codecnt++;
                  break;
               default:
                  printf("Unknown token: %s at %d\n", lexer1.YYText(), LineCount+linecnt);
                  break;
            }
         } while(token1 < TOKEN_EOF && ((token1 = lexer1.yylex()) != TOKEN_EOF));
      }
   }

   if(VerboseOutput) {
      char cpp_c_cnt[32];
      // make a shared column for C and C++ commented line counts
      sprintf(cpp_c_cnt, "%d/%d", cppcnt, ccnt); 
      printf("   %5d  %5d      %5d  %10s  %5d  %5d  %s\n", linecnt, codecnt, cppcnt+ccnt, cpp_c_cnt, emptycnt, bracecnt, filename);
   }

   EmptyLineCount += emptycnt;
   BraceLineCount += bracecnt;
   LineCount += linecnt;
   CodeLineCount += codecnt;
   CppLineCount += cppcnt;
   CLineCount += ccnt;

   CommentCount += cppcnt;
   CommentCount += ccnt;

   //
   //   Clean up
   //
   fclose(srcfile);

   return true;
}

void ProcessFileList(const std::string& dirname, const std::set<std::string, less_stricmp>& files)
{
   bool header = false;
   const char *filename, *cptr;
   std::set<std::string, less_stricmp>::const_iterator iter;
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

bool ProcessDirList(std::set<std::string, less_stricmp>& dirs)
{
   std::string dirname, temp;
   std::stack<std::set<std::string, less_stricmp>*> stack;
   std::set<std::string, less_stricmp>::iterator iter;
   std::set<std::string, less_stricmp> files;
   std::list<std::string> pathlist;

   std::set<std::string, less_stricmp> *subdirs = &dirs;

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
      //   When saving the current directory list's state, instead of saving
      // both, the iterator and the list, remove the current item and save
      // only the list. 
      //
      subdirs->erase(iter);
      stack.push(subdirs);
      subdirs = new std::set<std::string, less_stricmp>;

      //
      //   Populate the list with directories and process the files in
      // the current directory. 
      //
      EnumCurrentDir(files, *subdirs);
      ProcessFileList(GetFullPath(pathlist, temp), files);
      files.clear();

      //
      //   If the new directory list is empty, delete the list and then
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
bool EnumCurrentDir(std::set<std::string, less_stricmp>& files, std::set<std::string, less_stricmp>& subdirs)
{
   struct _finddata_t fileinfo;
   long fhandle;

   files.clear();
   subdirs.clear();

   if((fhandle = _findfirst("*.*", &fileinfo)) == -1)
      return false;

   do {
      if(fileinfo.attrib & _A_SUBDIR) {
         if(!stricmp(fileinfo.name, ".") || !stricmp(fileinfo.name, "..")) 
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
bool EnumCurrentDir(std::set<std::string, less_stricmp>& files, std::set<std::string, less_stricmp>& subdirs)
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
         if(!stricmp(entry->d_name, ".") || !stricmp(entry->d_name, "..")) 
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
   std::set<std::string, less_stricmp> files;
   std::set<std::string, less_stricmp> subdirs;
   char buffer[_MAX_PATH];

   if(dirname && *dirname)   {
      if(chdir(dirname) != 0) {
         printf("Can't change directory to %s\n", dirname);
         return false;
      }
   }
   else {
      getcwd(buffer, sizeof(buffer));
      dirname = buffer;
   }

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
   printf("Copyright 2015, Stone Steps Inc. http://www.stonesteps.ca\n\n");
}

void PrintUsage(void)
{
   printf("Syntax: linecnt [-s] [-v] [-d dir-name] [-c] [-j] [ext [ext [ ...]]]\n\n");

   printf("  -s    Process files in the current directory and all subdirectories\n");
   printf("  -v    Produce verbose output\n");
   printf("  -d    Start in the specified directory\n");
   printf("  -c    Add standard C/C++ extensions to the list\n");
   printf("  -j    Add standard Java extensions to the list\n");
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
   printf("Version: %s\n", LINECNT_VERSION_TEXT);
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
int main(int argc, char *argv[])
{
   const char *dirname = NULL;
   char **argptr = &argv[0];
   int comments = 0;

   PrintCopyrightLine();

   if(argc > 1) {
      //
      //   Skip the executable's name
      //
      argptr++;

      while(*argptr) {
         if(strchr("-/", **argptr)) {
            switch((*argptr)[1]) {
               case 'd':
                  dirname = *(++argptr);
                  break;
               case 's':
                  WalkTree = true;
                  break;
               case 'c':
                  ExtList.insert("cpp");
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
                  exit(-1);
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
      exit(-1);
   }

   //
   //
   //
   PrintFileExtensions("Processing files with extensions %s\n\n", ExtList);

   if(ProcessDirectory(dirname) == false) 
      exit(-2);

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

