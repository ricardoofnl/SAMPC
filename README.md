# San Andreas Multiplayer Custom 0.3.9 (ARCHIVED)
This repository contains the full source code for the 0.3.9 version of SAMPC, formerly known as SAMPE. Please note that this project is no longer actively maintained or updated. Any usage of this code is at your own risk.

## Screenshot(s)
![Screenshot 1](https://raw.githubusercontent.com/dashr9230/SAMPC/refs/heads/main/images/sa-mp-000.jpg "SetPlayerCameraPos rotation test")
![Screenshot 2](https://raw.githubusercontent.com/dashr9230/SAMPC/refs/heads/main/images/sa-mp-001.jpg "Chatbubble test")
![Screenshot 3](https://raw.githubusercontent.com/dashr9230/SAMPC/refs/heads/main/images/sa-mp-002.jpg "SetVehicleFeature test")

## Changelog
**SA-MP 0.3.9 Alpha 1**
- Has some changes in the chat to match with SA-MP's handling, and direct-drawing mode. (Unfinished)
- sa-mp.cfg reading is added, but currently read fontface, fontweight, multicore, nohudscalefix keys with values
- The scoreboard currently will not work, because it missing some parts to handle keyboard events and cursor
- An unfinished code of 3D Text handling
- Unfinished code of player marker sync
- Changed aim sync to match up with what SA-MP 0.3.7 has
- Couple removed stuff, which fixes issue with secondary task crash, and server assertion, due to structure alignment
- Some more game patches to would have to work with custom object which was added in 0.3c, but due to crash with SAMP.col, its currently not working
- Some code of Renderware stuff to render models or 2D sprites in text draws.

**SA-MP 0.3.9 Alpha 2 (CLIENT ONLY)**
- Changed death window handling. Now supports custom "fontface", "fontweight" and "fontsize" settings in sa-mp.cfg
- "/testdw" client command added.

**SA-MP 0.3.9 Alpha 3 (CLIENT ONLY)**
- Chat window now mostly finalized. Will use Render-To-Surface drawing mode by default, and can be switched to direct drawing mode by setting "directmode" to 1
- Chat window is now scrollable with mouse wheel (only when chat input box is open)
- Client messages now supports keyboard mapping with "~k~" prefix, like "~k~~VEHICLE_HANDBRAKE~" to print "SPACE" or whatever to what user handbrake key set to
- "timestamp" and "pagesize" config keys added, and should save it to sa-mp.cfg
- "/audiomsg" client command and "audiomsgoff" config key added, which disables audio stream url message in the chat
- In debug mode, by pressing subtract key, now will do a raw save to rawpositions.txt
- Exception handing somewhat added. Currently only check game scripts handler and prints opcode related warnings
- Discord presence also added
- Audio streams now should stop when server restarts or kicked out
- "/fontsize" client command added, but currently sets the chat window, death window, and UI font size, face and weight

**SA-MP 0.3.9-A4 [Client] and 0.3.9-A2 [Server]**
- SetPlayerChatBubble() function now added
- "messageholelimit" server config key added. But currently only report the warning type (1)
- "playertimeout" server config key had some internal changes
- Also got fixed the "nohudscalefix" saving

**Vehicle params**
- The whole 0.3.7 VEHICLE_PARAMS system is now wired up. Everything travels on the
  RPC_VehicleParams packet the client already knew how to read, instead of the custom
  ScrSetVehicle ops and the door/window bits that used to ride along with the spawn RPC
- SetVehicleParamsCarDoors(), SetVehicleParamsCarWindows(), SetVehicleEngineState() and
  SetVehicleLightState() write into the vehicle params. The matching Get functions can
  return VEHICLE_PARAMS_UNSET (-1) now, like SA-MP does
- SetVehicleParamsEx() and GetVehicleParamsEx() added
- Params reach players that connect later, and reset when the vehicle respawns. Before,
  the door and window state was read out of the spawn RPC and then thrown away
- The engine and lights params are applied every frame, with and without
  ManualVehicleEngineAndLights()
