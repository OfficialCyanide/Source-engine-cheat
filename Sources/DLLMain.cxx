#include "../SDK/SDK.hxx"
#include "../Hooks/CreateMove/CreateMove.hxx"
#include "../Hooks/Panels/Panels.hxx"
#include "../Hooks/KeyEvent/KeyEvent.hxx"

Offsets gOffsets;
Interfaces gInts;

CreateInterface_t EngineFactory = nullptr;
CreateInterface_t ClientFactory = nullptr;
CreateInterface_t VGUIFactory = nullptr;
CreateInterface_t VGUI2Factory = nullptr;

DWORD WINAPI dwMainThread(LPVOID lpArguments){
	if(!gInts.Client){
		ClientFactory = reinterpret_cast<CreateInterfaceFn>(GetProcAddress(
				gSig.GetModuleHandleSafe("client.dll"),
				"CreateInterface"));
		EngineFactory = reinterpret_cast<CreateInterfaceFn>(GetProcAddress(
				gSig.GetModuleHandleSafe("engine.dll"),
				"CreateInterface"));
		VGUIFactory = reinterpret_cast<CreateInterfaceFn>(GetProcAddress(
				gSig.GetModuleHandleSafe("vguimatsurface.dll"),
				"CreateInterface"));

		gInts.Client = reinterpret_cast<CHLClient*>(ClientFactory("VClient017", nullptr));
		gInts.EntList = reinterpret_cast<EntList*>(ClientFactory("VClientEntityList003", nullptr));
		gInts.Engine = reinterpret_cast<EngineClient*>(EngineFactory("VEngineClient013", nullptr));
		gInts.Surface = reinterpret_cast<Surface*>(VGUIFactory("VGUI_Surface030", nullptr));

		assert(gInts.EntList);
		assert(gInts.Client);
		assert(gInts.Engine);
		assert(gInts.Surface);

		if(!gInts.Panels){
			VGUI2Factory = reinterpret_cast<CreateInterfaceFn>(GetProcAddress(gSig.GetModuleHandleSafe("vgui2.dll"),
																			  "CreateInterface"));
			gInts.Panels = reinterpret_cast<Panel*>(VGUI2Factory("VGUI_Panel009", nullptr));
			assert(gInts.Panels);

			if(gInts.Panels){
				auto panelHook = new VMTBaseManager();
				panelHook->Init(gInts.Panels);
				panelHook->HookMethod(&Hooked_PaintTraverse, gOffsets.iPaintTraverseOffset);
				panelHook->Rehook();
			}
		}

		DWORD dwClientModeAddress = gSig.GetClientSignature("8B 0D ? ? ? ? 8B 01 5D FF 60 28 CC");
		assert(dwClientModeAddress);
		gInts.ClientMode = **reinterpret_cast<ClientModeShared***>(dwClientModeAddress + 2);

		auto clientHook = new VMTBaseManager();
		auto clientModeHook = new VMTBaseManager();

		clientHook->Init(gInts.Client);
		clientHook->HookMethod(&Hooked_KeyEvent, gOffsets.iKeyEventOffset);
		clientHook->Rehook();

		clientModeHook->Init(gInts.ClientMode);
		clientModeHook->HookMethod(&Hooked_CreateMove, gOffsets.iCreateMoveOffset);
		clientModeHook->Rehook();
	}
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD dwReason, LPVOID lpReserved){
	if(dwReason == DLL_PROCESS_ATTACH){
		CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE) dwMainThread, nullptr, 0,
					 nullptr);
	}
	return true;
}