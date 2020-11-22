#ifndef LINECNT_VERSION_H
#define LINECNT_VERSION_H

// keep this version in sync with VERSION in azure-pipelines.yml
#define VERSION_MAJOR           2
#define VERSION_MINOR           1
#define VERSION_PATCH           1

//
// Build number is supplied by Azure DevOps via AZP_BUILD_NUMBER for
// release builds and assigned to BUILD_NUMBER in linecnt-release.props.
// It is always zero for manual development builds.
//
#ifndef BUILD_NUMBER
#define BUILD_NUMBER            0
#endif

#endif
