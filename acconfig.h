/* Define the package name here */
#undef PACKAGE

/* Define the version number here */
#undef VERSION

@BOTTOM@

/* Determine the appropriate sound code to use */
#ifdef HAVE_SYS_SOUNDCARD_H
#define OSS_SOUND
#else
#define NO_SOUND
#endif
