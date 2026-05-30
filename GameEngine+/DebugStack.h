/////////////////////////////////
// DebugStack.h - Header file for the LogStack function, which is used for logging the current call stack for debugging
/////////////////////////////////



/////////////////////////////////
// Includes - there are no includes....
#pragma once
/////////////////////////////////


/////////////////////////////////
// LogStack - Logs the current call stack for debugging purposes. This function can be called from anywhere in the code to print the call stack to the console, 
// which can help identify the sequence of function calls leading up to a certain point in the code, especially when debugging crashes or unexpected behavior. 
// The optional tag parameter allows for adding a custom label to the log output for easier identification of different stack traces in the logs.
void LogStack(const char* tag = "stack");
/////////////////////////////////
