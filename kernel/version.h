/**
 * kernel/version.h
 *
 * Version, architecture and build stamp reported by the boot banner.
 *
 * The architecture is supplied by the build rather than branched on here, so
 * that adding a target never means editing shared code, and the date and time
 * are filled in by the preprocessor on every build.
 */

#ifndef VERSION_H
#define VERSION_H

#define KERNIE_VERSION    "0.1.0-dev"
#define KERNIE_BUILD_DATE __DATE__
#define KERNIE_BUILD_TIME __TIME__

#endif
