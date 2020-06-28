#pragma once


struct vec3
{
	float x, y, z;
};

class ViewMatrix {
public:
	float matrix[16];
};

struct vec4
{
	float x, y, z, w;
};

class Player
{
public:
	uintptr_t PlayerState;
	float headposx; //0x0004 
	float headposy; //0x0008 
	float headposz; //0x000C 
	char pad_0x0010[0x24]; //0x0010
	float footposx; //0x0034 
	float footposy; //0x0038 
	float footposz; //0x003C 
	char pad_0x0040[0xB8]; //0x0040
	__int32 HP; //0x00F8 
	char pad_0x00FC[0x8]; //0x00FC

}; //Size=0x0084

class EntityList
{
public:
	Player* PlayerPointer[32];
};