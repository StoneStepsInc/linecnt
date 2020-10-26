## Line Counting Utility

`lincnt` is a source line counting utility for C-like files. It uses a simple
parser to recognize C and C++ style comments to distinguish code and comment
lines and count them separately.

### What's Counted

`linecnt` counts separately comments, empty lines and brace lines, which are
lines with single open and close brace characters. Everything else is considered
code.

For example, this is how the following source file will be counted:

    ///                                         <-- C++ comment
    /// @brief Go to a URL.                     <-- C++ comment
    ///                                         <-- C++ comment
    void go(const std::string& url)             <-- code
    {                                           <-- brace
        if(url.empty())                         <-- code
            fw("http://localhost/");            <-- ignores `//` within a string
        else                                    <-- code
            fw(url); // go to the URL           <-- code + C++ comment
        /* used to return an error              <-- C comment
        return result;                          <-- C comment
        */                                      <-- C comment
    }                                           <-- brace
                                                <-- empty line

Single-quoted and double-quoted strings are recognized and comments within
strings are ignored.

Note that JavaScript's template literals are ignored by the current parser
and may be miscounted. For example, this line will be counted as code and a
C++ comment:

    let url = `http://localhost/${path}`;

### Syntax

    linecnt [-s] [-v] [-d dir-name] [-c] [-j] [ext [ext [ ...]]]

      -s    Process files in the current directory and all subdirectories
      -v    Produce verbose output
      -d    Start in the specified directory
      -c    Add common C/C++ extensions to the list
      -j    Add common Java extensions to the list
      -V    Print version information
      -W    Print warranty information
      -h    Print this help

Lines are counted in files identified by extensions. There is no default extension
list and at least one extension must be specified either explicitly or via the
shorthand `-c` and `-j` options.

Only current directory is scanned by default. Sub-directories may be scanned using
the `-s` option. Alternative directory may be specified via `-d` and may be either
a relative or an absolute directory.

### Examples

Scan `c`, `cpp` and `h` files in the current directory.

    linecnt cpp c h

Scan `.java` and `.inc` files in the directory `src`, relative to the current
directory, and all of its sub-directories.

    linecnt -d src -s -j inc

Scan commont C++ extensions in the directory `/prj/src` and all of its sub-directories.

    linecnt -d /prj/src -s -c

