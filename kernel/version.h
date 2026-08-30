#ifndef VERSION_H
#define VERSION_H

#define KERNIE_VERSION    "0.1.0-dev"
#if defined(__x86_64__)
#define KERNIE_ARCH       "x86_64"
#elif defined(__aarch64__)
#define KERNIE_ARCH       "arm64"
#else
#define KERNIE_ARCH       "unknown"
#endif
#define KERNIE_BUILD_DATE __DATE__
#define KERNIE_BUILD_TIME __TIME__

#endif
