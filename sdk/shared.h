
#ifndef _SAMP_SHARED_H
#define _SAMP_SHARED_H

#define DEVELOPER "RD42"
#define SAMP_VERSION "0.3.9"
#define BUILD_INFO "SA:MP-" SAMP_VERSION "-" DEVELOPER "-" __DATE__ "-" __TIME__

#define NETGAME_VERSION 4242
#define NETGAME_VERSION_V2 4062 // 0.3.7
#define GPCI_KEY 1001

#define MAX_PLAYER_NAME 24
#define MAX_COMPUTER_ID 40
#define MAX_VERSION_NAME 24
#define MAX_PLAYERS 200
#define MAX_VEHICLES 700
#define MAX_OBJECTS 255
#define MAX_MENUS 128
#define MAX_TEXT_DRAWS 2048
#define MAX_PLAYER_TEXT_DRAWS 256
#define MAX_GANG_ZONES 1024
#define MAX_CMD_INPUT 128
#define MAX_SPAWNS 500
#define MAX_MENU_ITEMS 12
#define MAX_MENU_LINE 32
#define MAX_COLUMNS 2
#define MAX_PICKUPS 400
#define MAX_TEXT_DRAW_LINE 800
#define MAX_MAP_ICON 200
#define MAX_ACTORS 1000
#define MAX_GAME_TEXT 200
#define MAX_CHAT_BUBBLE_TEXT 144
#define MAX_LABEL_TEXT 2048
#define MAX_LABEL_GLOBAL 1024
#define MAX_LABEL_PLAYER 1024
#define MAX_DIALOG_CAPTION 64
#define MAX_LICENSE_PLATE_TEXT 32
#define INVALID_PLAYER_ID_EX 65535
#define INVALID_VEHICLE_ID 0xFFFF
#define INVALID_MENU_ID 0xFF
#define INVALID_TEXT_DRAW 0xFFFF
#define INVALID_PLAYER_TEXT_DRAW 0xFFFF
#define INVALID_ACTOR_ID 0xFFFF
#define INVALID_LABEL_ID 0xFFFF // INVALID_3DTEXT_ID

#define RPC_PING_UPDATE_TIME 3000 // in ms (3 seconds)

#define DIALOG_STYLE_MSGBOX 0
#define DIALOG_STYLE_INPUT 1
#define DIALOG_STYLE_LIST 2
#define DIALOG_STYLE_PASSWORD 3
#define DIALOG_STYLE_TABLIST 4
#define DIALOG_STYLE_TABLIST_HEADERS 5

// 700 for client vehicle is questionable, since in the last seen game can only spawn
// 127 vehicles (originally), but got patched to 200, but still the 211 the max
#define MAX_CLIENT_VEHICLS 200  
#define MAX_CLIENT_PLAYERS 200
#define MAX_CLIENT_ACTORS 50

#define VEHICLE_PARAMS_UNSET -1
#define VEHICLE_PARAMS_OFF 0
#define VEHICLE_PARAMS_ON 1

#define TEXT_DRAW_FONT_SPRITE 4
#define TEXT_DRAW_FONT_MODEL_PREVIEW 5

#define EVENT_TYPE_PAINTJOB 1
#define EVENT_TYPE_CARCOMPONENT 2
#define EVENT_TYPE_CARCOLOR 3
#define EVENT_ENTEREXIT_MODSHOP 4
#define EVENT_TYPE_STUNT_JUMP 5

#define REJECT_REASON_BAD_VERSION 1
#define REJECT_REASON_BAD_NICKNAME 2
#define REJECT_REASON_BAD_MOD 3
#define REJECT_REASON_BAD_PLAYERID 4
#define REJECT_REASON_IP_LIMIT_REACHED 5

#define PLAYER_STATE_NONE 0
#define PLAYER_STATE_ONFOOT 1
#define PLAYER_STATE_DRIVER 2
#define PLAYER_STATE_PASSENGER 3
#define PLAYER_STATE_EXIT_VEHICLE 4
#define PLAYER_STATE_ENTER_VEHICLE_DRIVER 5
#define PLAYER_STATE_ENTER_VEHICLE_PASSENGER 6
#define PLAYER_STATE_WASTED 7
#define PLAYER_STATE_SPAWNED 8
#define PLAYER_STATE_SPECTATING 9

#define UPDATE_TYPE_NONE 0
#define UPDATE_TYPE_ONFOOT 1
#define UPDATE_TYPE_INCAR 2
#define UPDATE_TYPE_PASSENGER 3

#define SPECTATE_TYPE_NONE 0
#define SPECTATE_TYPE_PLAYER 1
#define SPECTATE_TYPE_VEHICLE 2

#define SPECIAL_ACTION_NONE 0
#define SPECIAL_ACTION_DUCK 1
#define SPECIAL_ACTION_USEJETPACK 2
#define SPECIAL_ACTION_ENTER_VEHICLE 3
#define SPECIAL_ACTION_EXIT_VEHICLE 4
#define SPECIAL_ACTION_DANCE1 5
#define SPECIAL_ACTION_DANCE2 6
#define SPECIAL_ACTION_DANCE3 7
#define SPECIAL_ACTION_DANCE4 8
#define SPECIAL_ACTION_HANDSUP 10
#define SPECIAL_ACTION_USECELLPHONE 11
#define SPECIAL_ACTION_SITTING 12
#define SPECIAL_ACTION_STOPUSECELLPHONE 13
#define SPECIAL_ACTION_DRINK_BEER 20
#define SPECIAL_ACTION_SMOKE_CIGGY 21
#define SPECIAL_ACTION_DRINK_WINE 22
#define SPECIAL_ACTION_DRINK_SPRUNK 23
#define SPECIAL_ACTION_CUFFED 24
#define SPECIAL_ACTION_CARRY 25
#define SPECIAL_ACTION_URINATE 68

#define WEAPON_FIST 0
#define WEAPON_BRASSKNUCKLE 1
#define WEAPON_GOLFCLUB 2
#define WEAPON_NITESTICK 3
#define WEAPON_KNIFE 4
#define WEAPON_BAT 5
#define WEAPON_SHOVEL 6
#define WEAPON_POOLSTICK 7
#define WEAPON_KATANA 8
#define WEAPON_CHAINSAW 9
#define WEAPON_DILDO 10
#define WEAPON_DILDO2 11
#define WEAPON_VIBRATOR 12
#define WEAPON_VIBRATOR2 13
#define WEAPON_FLOWER 14
#define WEAPON_CANE 15
#define WEAPON_GRENADE 16
#define WEAPON_TEARGAS 17
#define WEAPON_MOLTOV 18
#define WEAPON_COLT45 22
#define WEAPON_SILENCED 23
#define WEAPON_DEAGLE 24
#define WEAPON_SHOTGUN 25
#define WEAPON_SAWEDOFF 26
#define WEAPON_SHOTGSPA 27
#define WEAPON_UZI 28
#define WEAPON_MP5 29
#define WEAPON_AK47 30
#define WEAPON_M4 31
#define WEAPON_TEC9 32
#define WEAPON_RIFLE 33
#define WEAPON_SNIPER 34
#define WEAPON_ROCKETLAUNCHER 35
#define WEAPON_HEATSEEKER 36
#define WEAPON_FLAMETHROWER 37
#define WEAPON_MINIGUN 38
#define WEAPON_SATCHEL 39
#define WEAPON_BOMB 40
#define WEAPON_SPRAYCAN 41
#define WEAPON_FIREEXTINGUISHER 42
#define WEAPON_CAMERA 43
#define WEAPON_NIGHT_VIS_GOGGLES 44
#define WEAPON_THERMAL_GOGGLES 45
#define WEAPON_PARACHUTE 46
#define WEAPON_VEHICLE 49
#define WEAPON_HELIBLADES 50
#define WEAPON_EXPLOSION 51
#define WEAPON_DROWN 53
#define WEAPON_COLLISION 54

#define TRAIN_PASSENGER_LOCO 537
#define TRAIN_FREIGHT_LOCO 538
#define TRAIN_PASSENGER 569
#define TRAIN_FREIGHT 570
#define TRAIN_TRAM 449
#define HYDRA 520

#define UPDATE_TYPE_NONE 0
#define UPDATE_TYPE_FULL 1
#define UPDATE_TYPE_MINIMAL 2

#define VALID_KILL 1
#define TEAM_KILL 2
#define SELF_KILL 3

#define INVALID_PLAYER_ID 255
#define NO_TEAM 255

#define PI 3.14159265f

#define PACK_VEHICLE_HEALTH(f) (unsigned char)(f / 4)
#define UNPACK_VEHICLE_HEALTH(b) (float)b * 4

#ifndef ARRAY_SIZE
	#define ARRAY_SIZE(a) ( sizeof((a)) / sizeof(*(a)) )
#endif
#ifndef SAFE_DELETE
	#define SAFE_DELETE(p) { if (p) { delete (p); (p) = NULL; } }
#endif
#ifndef SAFE_RELEASE
	#define SAFE_RELEASE(p)	{ if (p) { (p)->Release(); (p) = NULL; } }
#endif

#define TO_STR(x) #x
#define STR(x) TO_STR(x)

//typedef unsigned short ELEMENTID;
typedef unsigned short PLAYERID;
typedef unsigned short VEHICLEID;

typedef struct {
	unsigned char r, g, b, a;
} RGBA, *PRGBA;

typedef struct {
	// New format, 24 bits for each of X, Y, Z = 72 bits/9 bytes
	char data[9];

	// Old format
	// short X,Y,Z;
} C_VECTOR1;

typedef struct VECTOR {
	float X, Y, Z;

	VECTOR()
	{
		X = Y = Z = 0.0f;
	}

	VECTOR(float f)
	{
		X = Y = Z = f;
	}

	VECTOR(float x, float y, float z)
	{
		X = x;
		Y = y;
		Z = z;
	}

	inline void operator=(const VECTOR& r)
	{
		X = r.X;
		Y = r.Y;
		Z = r.Z;
	}

	inline void operator=(const VECTOR* r)
	{
		X = r->X;
		Y = r->Y;
		Z = r->Z;
	}

	inline void operator=(const float f)
	{
		X = Y = Z = f;
	}
} VECTOR, *PVECTOR;

typedef struct {
	float X, Y;
} VECTOR2D, *PVECTOR2D;

typedef struct
{
	float W, X, Y, Z;
} QUATERNION, *PQUATERNION;

typedef struct {
	VECTOR right;
	unsigned long flags;
	VECTOR up;
	float pad_u;
	VECTOR at;
	float pad_a;
	VECTOR pos;
	float pad_p;
} MATRIX4X4, *PMATRIX4X4;

typedef struct { // size = 50
	BYTE byteTeam;
	int iSkin;
	int iBaseSkin;
	BYTE _pad;
	VECTOR vecPos;
	float fRotation;
	int iSpawnWeapons[3];
	int iSpawnWeaponsAmmo[3];
} PLAYER_SPAWN_INFO;

typedef struct {
	WORD lrAnalog;
	WORD udAnalog;
	WORD wKeys;
	VECTOR vecPos;
	QUATERNION quatRot;
	BYTE byteHealth;
	BYTE byteArmour;
	BYTE byteCurrentWeapon : 6;
	BYTE byteSpecialKey : 2;
	BYTE byteSpecialAction;
	VECTOR vecMoveSpeed;
	
	
	VECTOR vecSurfOffsets;
	VEHICLEID SurfVehicleId;
} ONFOOT_SYNC_DATA;

typedef struct { // size=31
	BYTE byteCamMode;
	VECTOR vecAimf1;
	VECTOR vecAimPos;
	float fAimZ;
	BYTE byteCamExtZoom : 6;	// 0-63 normalized
	BYTE byteWeaponState : 2;	// see eWeaponState
	BYTE byteAspectRatio;
} AIM_SYNC_DATA;

typedef struct {
	VEHICLEID VehicleID;
	WORD lrAnalog;
	WORD udAnalog;
	WORD wKeys;
	C_VECTOR1 cvecRoll;
	C_VECTOR1 cvecDirection;
	VECTOR vecPos;
	VECTOR vecMoveSpeed;
	float fCarHealth;
	BYTE bytePlayerHealth;
	BYTE bytePlayerArmour;
	BYTE byteCurrentWeapon : 6;
	BYTE byteSpecialKey : 2;
	BYTE byteSirenOn;
	BYTE byteLandingGearState;
	BYTE byteTires[4];
	VEHICLEID TrailerID;
	unsigned long dwHydraThrustAngle;
	float fTrainSpeed;
	BYTE ucInfo;
} INCAR_SYNC_DATA;

typedef struct {
	VEHICLEID VehicleID;
	BYTE byteSeatFlags : 7;
	BYTE byteDriveBy : 1;
	BYTE byteCurrentWeapon : 6;
	BYTE byteSpecialKey : 2;
	BYTE bytePlayerHealth;
	BYTE bytePlayerArmour;
	WORD lrAnalog;
	WORD udAnalog;
	WORD wKeys;
	VECTOR vecPos;
} PASSENGER_SYNC_DATA;

typedef struct {
	unsigned short lrAnalog;
	unsigned short udAnalog;
	WORD wKeys;
	VECTOR vecPos;
} SPECTATOR_SYNC_DATA;

typedef struct { // size=52
	VEHICLEID TrailerID;
	VECTOR vecPos;
	QUATERNION quatRot;	
	VECTOR vecMoveSpeed;
	VECTOR vecTurnSpeed;
} TRAILER_SYNC_DATA;

typedef struct {
	int iModel;
	int iType;
	float fX;
	float fY;
	float fZ;
} PICKUP;

typedef struct { // size must be 63 bytes
	union
	{
		BYTE byteFlags;				// 0
		struct
		{
			BYTE byteBox : 1;
			BYTE byteLeft : 1;
			BYTE byteRight : 1;
			BYTE byteCenter : 1;
			BYTE byteProportional : 1;
			BYTE bytePadding : 3;
		};
	};
	float fLetterWidth;				// 1
	float fLetterHeight;			// 5
	DWORD dwLetterColor;			// 9
	float fLineWidth;				// 13
	float fLineHeight;				// 17
	DWORD dwBoxColor;				// 21
	BYTE byteShadow;				// 25
	BYTE byteOutline;				// 26
	DWORD dwBackgroundColor;		// 27
	BYTE byteStyle;					// 31
	BYTE byteSelectable;			// 32
	float fX;						// 33
	float fY;						// 37
} TEXT_DRAW_TRANSMIT;

typedef struct _CAR_MOD_INFO // (size = 23)
{
	unsigned char ucCarMod[17];
	unsigned char bytePaintJob;
	int iColor0;
	int iColor1;
} CAR_MOD_INFO;

typedef struct // (size = 16)
{
	BYTE byteEngine;
	BYTE byteLights;
	BYTE byteAlarm;
	BYTE byteDoors;
	BYTE byteBonnet;
	BYTE byteBoot;
	BYTE byteObjective;
	BYTE byteSiren;
	BYTE byteDriverDoor;
	BYTE bytePassengerDoor;
	BYTE byteBackLeftDoor;
	BYTE byteBackRightDoor;
	BYTE byteDriverWindow;
	BYTE bytePassengerWindow;
	BYTE byteBackLeftWindow;
	BYTE byteBackRightWindow;
} VEHICLE_PARAMS;

struct MENU_INT
{
	bool bMenu;
	bool bRow[MAX_MENU_ITEMS];
	bool bPadding[8 - ((MAX_MENU_ITEMS + 1) % 8)];
};

typedef struct
{
	unsigned short usActorID;
	int iModelID;
	VECTOR vecPosition;
	float fFacingAngle;
	float fHealth;
	bool bInvurnable;
} ACTOR_TRANSMIT;

typedef struct // (size = 40)
{
	VEHICLEID VehicleID;
	int iModelID;
	VECTOR vecPos;
	float fRotation;
	BYTE byteColor1;
	BYTE byteColor2;
	float fHealth;
	BYTE byteInterior;
	DWORD dwDoorsState;
	DWORD dwPanelsState;
	BYTE byteLightsState;
	BYTE byteTyresState;
	BYTE byteHasSiren;
} VEHICLE_TRANSMIT;

#endif // _SAMP_SHARED_H
