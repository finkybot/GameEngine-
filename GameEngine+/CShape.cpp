/////////////////////////////////
// CShape.cpp - Implementation of the CShape component class, which serves as a base class for specific shape components like rectangles and circles. This class defines common properties and methods for shapes, such as mid-length for collision detection, 
// and pure virtual methods that must be implemented by derived shape classes to provide specific shape behavior and properties.Components in an ECS architecture are often just data holders, so it's common for them to have minimal or no logic in their implementation files, 
// with most of the functionality being defined in the header file as pure virtual methods to be implemented by derived classes.
/////////////////////////////////



/////////////////////////////////
// Includes and necessary headers for the CShape component implementation.
#include "CShape.h"
/////////////////////////////////



/////////////////////////////////
// Constructor and virtual destructor for the CShape component. The constructor initializes the shape with default properties, while the virtual destructor allows for proper cleanup of derived shape classes when deleted through a base class pointer.
CShape::CShape() {}
CShape::~CShape() {}
/////////////////////////////////
