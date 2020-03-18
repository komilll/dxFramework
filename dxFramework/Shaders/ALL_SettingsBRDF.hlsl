//http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
#define NDF_BECKMANN    0
#define NDF_GGX         1
#define NDF_BLINN_PHONG 2

#define GEOM_IMPLICIT           0
#define GEOM_NEUMANN            1
#define GEOM_COOK_TORRANCE      2
#define GEOM_KELEMEN            3
#define GEOM_BECKMANN           4
#define GEOM_GGX                5
#define GEOM_SCHLICK_BECKMANN   6
#define GEOM_SCHLICK_GGX        7

#define FRESNEL_NONE    0
#define FRESNEL_SCHLICK 1
#define FRESNEL_CT      2

#define DEBUG_NONE        0
#define DEBUG_DIFF        1
#define DEBUG_SPEC        2
#define DEBUG_ALBEDO      3
#define DEBUG_NORMAL      4
#define DEBUG_ROUGHNESS   5
#define DEBUG_METALLIC    6
#define DEBUG_NDF         7
#define DEBUG_GEOM        8
#define DEBUG_FRESNEL     9