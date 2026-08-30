/**
 * kernel/version.h
 *
 * Version, architecture and build stamp reported by the boot banner.
 *
 * The architecture is derived from the compiler rather than configured, so it
 * cannot drift from the target actually being built, and the date and time are
 * filled in by the preprocessor on every build.
 */

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
