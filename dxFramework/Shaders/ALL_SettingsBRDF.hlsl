//http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
#define NDF_BECKMANN    0
#define NDF_GGX         1
#define NDF_BLINN_PHONG 2

#define GEOM_IMPLICIT           0
#define GEOM_NEUMANN            1
#define GEOM_COOK_TORRANCE      2
#define GEOM_KELEMEN            3
#define GEOM_SMITH              4
#define GEOM_BECKMANN           5
#define GEOM_GGX                6
#define GEOM_SCHLICK_BECKMANN   7
#define GEOM_SCHLICK_GGX        8

#define FRESNEL_NONE    0
#define FRESNEL_SCHLICK 1
#define FRESNEL_CT      2