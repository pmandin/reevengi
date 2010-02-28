#ifndef CONFIG_H
#define CONFIG_H 1

#define PACKAGE_NAME "reevengi"

#define PACKAGE_VERSION "0.15"

#define PACKAGE_STRING PACKAGE_NAME " " PACKAGE_VERSION

#define ENABLE_OPENGL 1

/*#define ENABLE_GAMMA 1*/

/* Enabled in VS project file */
/*#define ENABLE_MOVIES 1*/

#define ENABLE_SCRIPT_DISASM 1

/* MSVC++ does not have inline keyword (needed for FFmpeg) */
#define inline _inline

#endif /* CONFIG_H */
