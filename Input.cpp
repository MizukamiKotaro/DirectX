#include "Input.h"

#include <cassert>

Input* Input::GetInstance() {
	static Input instance;
	return &instance;
}

void Input::Initialize(const HWND hwnd, const HINSTANCE hInstance) {

	HRESULT hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
		(void**)&dInput_, nullptr);
	assert(SUCCEEDED(hr));

	hr = dInput_->CreateDevice(GUID_SysKeyboard, &devKeyboard_, NULL);
	assert(SUCCEEDED(hr));

	hr = devKeyboard_->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(hr));

	hr = devKeyboard_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(hr));


}

void Input::Update() {

	devKeyboard_->Acquire();

	for (int num = 0; num < 256; num++) {
		keyPre_[num] = key_[num];
	}
	devKeyboard_->GetDeviceState(sizeof(key_), key_);
	
}