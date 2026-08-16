// Minimal gamemode shipped with the SAMPC server package, enough to get a
// server running so you can confirm the build works. Replace it with your own.

#include <a_samp>

main()
{
	print("\n----------------------------------");
	print(" Bare script loaded.");
	print("----------------------------------\n");
}

public OnGameModeInit()
{
	SetGameModeText("Bare Script");
	AddPlayerClass(0, 1958.3783, 1343.1572, 15.3746, 269.1425, 0, 0, 0, 0, 0, 0);
	return 1;
}

public OnPlayerRequestClass(playerid, classid)
{
	SetPlayerPos(playerid, 1958.3783, 1343.1572, 15.3746);
	SetPlayerCameraPos(playerid, 1958.3783, 1343.1572, 15.3746);
	SetPlayerCameraLookAt(playerid, 1958.3783, 1343.1572, 15.3746);
	return 1;
}

public OnPlayerSpawn(playerid)
{
	SetPlayerInterior(playerid, 0);
	return 1;
}