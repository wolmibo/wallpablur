#if __has_include("versioninfo.h")
#include "versioninfo.h"
#else
#warning "Missing vcs version info"
#define VERSION_VCS_STR "<?>"
#endif

#include "buildinfo.h"

#include <string_view>



std::string_view version_info() {
  return "wallpablur " VERSION_STR " (" VERSION_VCS_STR ")\n"
    "\n"
    "Build options: " BUILD_OPTIONS
    "\n";
}
