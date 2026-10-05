#pragma once
// Process/runtime services shared by native render backends. No JVM declarations.
void Dbg(const char* fmt,...);
void LogTo(const char* fmt,...);
void ReportBootstrapReady();
void ReportBootstrapFailed(const char* message);
bool IsCircuitOpen();
void OpenCircuit();
void RequestUnload();
