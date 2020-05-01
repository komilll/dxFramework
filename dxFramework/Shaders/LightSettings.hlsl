#ifndef _LIGHT_SETTINGS_H_
#define _LIGHT_SETTINGS_H_

struct Light
{
    float3 positionWS;
    float radius;
    float3 color;
    
    int type;
};

static const uint LIGHT_TYPE_POINT       = 0;
static const uint LIGHT_TYPE_DIRECTIONAL = 1;
static const uint LIGHT_TYPE_AREA_SPHERE = 2;

#endif // _LIGHT_SETTINGS_H_