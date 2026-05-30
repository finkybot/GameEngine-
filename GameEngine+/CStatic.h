/////////////////////////////////
//CStatic.h - Header file for the CStatic component class, which represents static entities in the game world that do not have dynamic behavior. 
// This component can be used to mark entities as static for optimization purposes, such as excluding them from certain physics calculations or AI processing.
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "Component.h"
/////////////////////////////////



/////////////////////////////////
// CStatic component - represents static entities in the game world that do not have dynamic behavior. This component can be used to mark 
// entities as static for optimization purposes, such as excluding them from certain physics calculations or AI processing.
class CStatic : public Component {
	/////////////////////////////////
	// Public methods for the CStatic component. The constructor and destructor are defined, with the destructor marked as override to ensure proper cleanup of derived classes when deleted through a base class pointer.
public:
	CStatic() = default;
	~CStatic() override = default;
	/////////////////////////////////
};
/////////////////////////////////