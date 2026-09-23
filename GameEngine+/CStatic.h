/////////////////////////////////
//CStatic.h - Header file for the CStatic component class, which represents static entities in the game world that do not have dynamic behavior. This component can be used to mark entities as static for optimization purposes, such as excluding them from certain physics calculations or AI processing.
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "Component.h"
/////////////////////////////////



/////////////////////////////////
//	|	CStatic component - represents static entities in the game world that do not have dynamic behavior. This component can be used to mark entities as static for optimization purposes, such as excluding them from certain physics calculations or AI processing.
//	|_______________________________________________________________________
class CStatic : public Component {

public:
	CStatic() = default;
	~CStatic() override = default;
};
/////////////////////////////////