#include <array>
#include <atomic>

#undef max

namespace GOTHIC_NAMESPACE 
{
	bool PolishInputNames = false;
	struct MouseState {
		std::array<std::atomic<bool>, 8> buttonsPressed{}; // 0=Left,1=Right,2=Middle,3=X1,4=X2,5=X3,6=X4,7=X5
		std::array<std::atomic<bool>, 8> buttonsToggled{}; // 0=Left,1=Right,2=Middle,3=X1,4=X2,5=X3,6=X4,7=X5
	} mouseState;

	struct FocusCandidate
	{
		zCVob* vob = nullptr;
		float score = std::numeric_limits<float>::max();
	};


	//bool* keyEvent = reinterpret_cast<bool*>(0x0086ccc8);
	//bool* keyToggle = reinterpret_cast<bool*>(0x0086ced0);
	//bool* keyRepeatEnabled = reinterpret_cast<bool*>(0x0086d0d8);
	zCArray<int>* keyBuffer = reinterpret_cast<zCArray<int>*>(0x0086d2dc);

	constexpr int ButtonMap[8] =
	{
		MOUSE_BUTTONLEFT,  MOUSE_BUTTONRIGHT, MOUSE_BUTTONMID, 
		MOUSE_XBUTTON1,   MOUSE_XBUTTON2,   MOUSE_XBUTTON3, 
		MOUSE_XBUTTON4,   MOUSE_XBUTTON5
	};

	void __fastcall P_UpdateDInput(::Union::Registers& reg)
	{
		zinput->SetState(GAME_SMOVE, FALSE);

		// Use GetAsyncKeyState to check mouse buttons directly
		constexpr int VK_Buttons[8] = {
			VK_LBUTTON,  // 0
			VK_RBUTTON,  // 1  
			VK_MBUTTON,  // 2
			VK_XBUTTON1, // 3
			VK_XBUTTON2, // 4
			0,           // 5 - no VK code for X3
			0,           // 6 - no VK code for X4
			0            // 7 - no VK code for X5
		};

		for(int i = 0; i < 8; i++)
		{
			//mouseState.buttonsPressed[i].store(false);
			mouseState.buttonsToggled[i].store(false);
		}

		for(int i = 0; i < 5; i++) // Only first 5 buttons work with GetAsyncKeyState
		{
			auto keyId = ButtonMap[i];
			//keyToggle[keyId] = false;
			
			bool pressed = (GetAsyncKeyState(VK_Buttons[i]) & 0x8000) != 0;
			bool wasPressed = mouseState.buttonsPressed[i].load();

			mouseState.buttonsPressed[i].store(pressed);

			if (pressed)
			{
				bool toggled = !wasPressed;
				mouseState.buttonsToggled[i].store(toggled);

				if(toggled && keyBuffer && keyBuffer->parray != nullptr)
				{
					keyBuffer->InsertEnd(keyId);
				}
			}
		}
	}
	auto Partial__UpdateDInput = ::Union::CreatePartialHook(reinterpret_cast<void*>(0x004c7f05), &P_UpdateDInput);
	
	constexpr float TrampolineFloorDistanceLimit = -102.0f;

	void __fastcall P_FixTrampolineJump(::Union::Registers& reg)
	{
		auto ai = reinterpret_cast<zCAIPlayer*>(reg.esi);
		if (ai->aboveFloor < TrampolineFloorDistanceLimit)
		{
			// Skip the jump's upward snap and hitFloor assignment; keep aboveFloor intact.
			reg.eip = 0x0062b731;
		}
	}
	auto Partial__FixTrampolineJump = ::Union::CreatePartialHook(reinterpret_cast<void*>(0x0062b6d4), &P_FixTrampolineJump);

	void __fastcall P_FixTrampolineFloorAlignment(::Union::Registers& reg)
	{
		auto ai = reinterpret_cast<zCAIPlayer*>(reg.ecx);
		auto floorY = *reinterpret_cast<const float*>(reg.esp);
		if (ai->feetY - floorY < TrampolineFloorDistanceLimit)
		{
			// Skip AlignToFloor and consume its float argument, matching the callee's RET 4.
			reg.esp += sizeof(float);
			reg.eip = 0x0062b31e;
		}
	}
	auto Partial__FixTrampolineFloorAlignment = ::Union::CreatePartialHook(reinterpret_cast<void*>(0x0062b319), &P_FixTrampolineFloorAlignment);

	void __fastcall P_ClearKeyBuffer(::Union::Registers& reg)
	{
		/*for(int i = 0; i < MAX_MOUSE_BTNS_AND_CODES; i++)
		{
			keyEvent[i] = false;
			keyToggle[i] = false;
		}*/
		for(int i = 0; i < 8; i++)
		{
			mouseState.buttonsPressed[i].store(false);
			mouseState.buttonsToggled[i].store(false);
		}
	}
	auto Partial__ClearKeyBuffer = ::Union::CreatePartialHook(reinterpret_cast<void*>(0x004c8b81), &P_ClearKeyBuffer);

	void __fastcall P_InitDInput(::Union::Registers& reg)
	{
		/*for(int i = MIN_MOUSE_BTNS_AND_CODES; i < MOUSE_XBUTTON2; i++)
		{
			keyEvent[i] = false;
			keyToggle[i] = false;
			keyRepeatEnabled[i] = (i >= MIN_MOUSE_BTNS && i <= MOUSE_XBUTTON2);
		}*/
		for(int i = 0; i < 8; i++)
		{
			mouseState.buttonsPressed[i].store(false);
			mouseState.buttonsToggled[i].store(false);
		}
	}
	auto Partial__InitDInput = ::Union::CreatePartialHook(reinterpret_cast<void*>(0x004c7c8d), &P_InitDInput);

	void __fastcall P_KeyPressed(::Union::Registers& reg)
	{
		// param_1 is at [ESP + 4] because __thiscall pushes return address
		int* pStack = reinterpret_cast<int*>(reg.esp);
		int keyId = pStack[1]; // [ESP+4] = pStack[1]
		
		// Check if it's one of our custom mouse buttons
		if (keyId >= MOUSE_BUTTONLEFT && keyId <= MOUSE_XBUTTON5)
		{
			reg.eax = mouseState.buttonsPressed[keyId - MOUSE_BUTTONLEFT].load() ? 1 : 0;
			// Jump to RET 4 instruction to properly clean up stack
			reg.eip = 0x004c87ab;
		}
	}
	auto Partial__KeyPressed = ::Union::CreatePartialHook(reinterpret_cast<void*>(0x004c87a0), &P_KeyPressed);

	void __fastcall P_KeyToggled(::Union::Registers& reg)
	{
		// param_1 is at [ESP + 4] because __thiscall pushes return address
		int* pStack = reinterpret_cast<int*>(reg.esp);
		int keyId = pStack[1]; // [ESP+4] = pStack[1]
		
		// Check if it's one of our custom mouse buttons
		if (keyId >= MOUSE_BUTTONLEFT && keyId <= MOUSE_XBUTTON5)
		{
			// Read and atomically clear the toggle flag (exchange with false)
			bool toggled = mouseState.buttonsToggled[keyId - MOUSE_BUTTONLEFT].exchange(false);
			reg.eax = toggled ? 1 : 0;
			// Jump to RET 4 instruction to properly clean up stack
			reg.eip = 0x004c87c2;
		}
	}
	auto Partial__KeyToggled = ::Union::CreatePartialHook(reinterpret_cast<void*>(0x004c87b0), &P_KeyToggled);

	void __fastcall P_GetKey(::Union::Registers& reg)
	{
		auto key = reg.ecx;
		if (key > 515)
		{
			reg.eip = 0x004c89dd;
		}
	}
	auto Partial__GetKey = ::Union::CreatePartialHook(reinterpret_cast<void*>(0x004c8987), P_GetKey);

	void LoadSettings()
	{
		int polishLanguage = 0;
		auto userLocale = ::Union::Locale::GetUserLocale();
		if (userLocale.Language == ::Union::LANGUAGE::POLISH)
		{
			polishLanguage = 1;
		}

		PolishInputNames = zoptions->ReadBool(zSTRING{"G1Fixes"}, zSTRING{"PolishInputNames"}, polishLanguage);
	}

	void ReplaceInputName(int value, const zSTRING& name)
	{
		for(int i = 0; i < zCInput::values.GetNum(); i++)
		{
			zSCtrlValue& v = zCInput::values[i];
			if(v.value == value)
			{
				v.name = name;
				return;
			}
		}
	}

	void InitPolishInputNames()
	{
		// Gothic expects Windows-1250 bytes; escapes keep the source encoding-independent.
		ReplaceInputName(MOUSE_DX, "MYSZ-X");
		ReplaceInputName(MOUSE_DY, "MYSZ-Y");
		ReplaceInputName(MOUSE_WHEELUP, "K\xD3\xA3KO W G\xD3R\xCA");
		ReplaceInputName(MOUSE_WHEELDOWN, "K\xD3\xA3KO W D\xD3\xA3");
		ReplaceInputName(MOUSE_BUTTONLEFT, "MYSZ - LEWY");
		ReplaceInputName(MOUSE_BUTTONRIGHT, "MYSZ - PRAWY");
		ReplaceInputName(MOUSE_BUTTONMID, "MYSZ - \x8CRODKOWY");
		ReplaceInputName(MOUSE_XBUTTON1, "MYSZ - X1");
		ReplaceInputName(MOUSE_XBUTTON2, "MYSZ - X2");
		ReplaceInputName(MOUSE_XBUTTON3, "MYSZ - X3");
		ReplaceInputName(MOUSE_XBUTTON4, "MYSZ - X4");
		ReplaceInputName(MOUSE_XBUTTON5, "MYSZ - X5");
		ReplaceInputName(KEY_UP, "STRZA\xA3KA W G\xD3R\xCA");
		ReplaceInputName(KEY_LEFT, "STRZA\xA3KA W LEWO");
		ReplaceInputName(KEY_DOWN, "STRZA\xA3KA W D\xD3\xA3");
		ReplaceInputName(KEY_RIGHT, "STRZA\xA3KA W PRAWO");

		ReplaceInputName(KEY_SEMICOLON, "\x8CREDNIK");
		ReplaceInputName(KEY_APOSTROPHE, "APOSTROF");
		ReplaceInputName(KEY_LSHIFT, "LEWY SHIFT");
		ReplaceInputName(KEY_RSHIFT, "PRAWY SHIFT");
		ReplaceInputName(KEY_LCONTROL, "LEWY CTRL");
		ReplaceInputName(KEY_RCONTROL, "PRAWY CTRL");
		ReplaceInputName(KEY_LALT, "LEWY ALT");
		ReplaceInputName(KEY_RALT, "PRAWY ALT");
		ReplaceInputName(KEY_EQUALS, "R\xD3WNA SI\xCA");
		ReplaceInputName(KEY_SPACE, "SPACJA");
		ReplaceInputName(KEY_PAUSE, "PAUZA");

		ReplaceInputName(KEY_MULTIPLY, "NUM GWIAZDKA");
		ReplaceInputName(KEY_ADD, "NUM PLUS");
		ReplaceInputName(KEY_SUBTRACT, "NUM MINUS");
		ReplaceInputName(KEY_DIVIDE, "NUM UKO\x8CNIK");

		ReplaceInputName(KEY_LWIN, "LEWY KLAWISZ WINDOWS");
		ReplaceInputName(KEY_RWIN, "PRAWY KLAWISZ WINDOWS");

		ReplaceInputName(KEY_COMMA, "PRZECINEK");
		ReplaceInputName(KEY_PERIOD, "KROPKA");
		ReplaceInputName(KEY_SLASH, "UKO\x8CNIK");

		ReplaceInputName(KEY_GRAVE, "TYLDA");

		ReplaceInputName(KEY_LBRACKET, "LEWY NAWIAS");
		ReplaceInputName(KEY_RBRACKET, "PRAWY NAWIAS");
	}

	void Game_Init()
	{
		LoadSettings();

		zSCtrlValue v;
		v.value = MOUSE_DX;				v.name="MOUSE-X";		zCInput::values.Insert(v);					
		v.value = MOUSE_DY;				v.name="MOUSE-Y";		zCInput::values.Insert(v);		
		v.value = MOUSE_WHEELUP;		v.name="WHEEL UP";		zCInput::values.Insert(v);
		v.value = MOUSE_WHEELDOWN;		v.name="WHEEL DOWN";	zCInput::values.Insert(v);
		v.value = MOUSE_BUTTONLEFT;		v.name="MOUSE LEFT";	zCInput::values.Insert(v);
		v.value = MOUSE_BUTTONRIGHT;	v.name="MOUSE RIGHT";	zCInput::values.Insert(v);
		v.value = MOUSE_BUTTONMID;		v.name="MOUSE MID";		zCInput::values.Insert(v);
		v.value = MOUSE_XBUTTON1;		v.name="MOUSE X1";		zCInput::values.Insert(v);
		v.value = MOUSE_XBUTTON2;		v.name="MOUSE X2";		zCInput::values.Insert(v);
		v.value = MOUSE_XBUTTON3;		v.name="MOUSE X3";		zCInput::values.Insert(v);
		v.value = MOUSE_XBUTTON4;		v.name="MOUSE X4";		zCInput::values.Insert(v);
		v.value = MOUSE_XBUTTON5;		v.name="MOUSE X5";		zCInput::values.Insert(v);

		if (PolishInputNames)
		{
			InitPolishInputNames();
		}
	}

	void Game_ApplySettings()
	{
		LoadSettings();
	}

	void __fastcall oCGame_Init(oCGame* self, void* vtable);
	auto Hook_oCGame_Init = Union::CreateHook(SIGNATURE_OF(&oCGame::Init), &oCGame_Init, Union::HookType::Hook_Detours);
	void __fastcall oCGame_Init(oCGame* self, void* vtable)
	{
		Hook_oCGame_Init(self, vtable);
		Game_Init();
	}

	void __fastcall CGameManager_ApplySomeSettings(CGameManager* self, void* vtable);
	auto Hook_CGameManager_ApplySomeSettings = Union::CreateHook(SIGNATURE_OF(&CGameManager::ApplySomeSettings), &CGameManager_ApplySomeSettings, Union::HookType::Hook_Detours);
	void __fastcall CGameManager_ApplySomeSettings(CGameManager* self, void* vtable)
	{
		Hook_CGameManager_ApplySomeSettings(self, vtable);
		Game_ApplySettings();
	}

}
