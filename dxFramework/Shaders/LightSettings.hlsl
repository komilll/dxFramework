#ifndef _LIGHT_SETTINGS_H_
#define _LIGHT_SETTINGS_H_

#define LIGHT_TYPE_NONE        0
#define LIGHT_TYPE_POINT       1
#define LIGHT_TYPE_DIRECTIONAL 2
#define LIGHT_TYPE_AREA        4
#define LIGHT_TYPE_ENVIRONMENT 8

struct Light
{
    float3 positionWS;
    float radius;
    float3 color;
    
    uint type;
};

#endif // _LIGHT_SETTINGS_H_