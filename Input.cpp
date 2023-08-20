#include "Input.h"

Input* Input::GetInstance() {
	static Input instance;
	return &instance;
}

