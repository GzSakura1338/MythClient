#include <cassert>
#include <iostream>
#include <vector>
using jobject=int;
struct Handles {int Level=1,GameController=2,HeldItem=3,Swing=4,StartUseItem=5,UsePacket=6,UsePacketCtor=7;} h;
std::vector<int> calls;
bool good=true,failController=false,controllerResult=false;
struct Env {
 int GetObjectField(int,int field){return field==h.Level?10:11;}
 int CallObjectMethod(int,int){return 12;}
 void CallVoidMethod(int,int method){assert(method==h.Swing);calls.push_back(1);}
 bool CallBooleanMethod(int controller,int method,int player,int world,int stack){
  assert(controller==11 && method==h.StartUseItem && player==9 && world==10 && stack==12);
  calls.push_back(2);calls.push_back(3); // slot sync and controller's C08
  if(failController)good=false;
  return controllerResult;
 }
 int NewObject(int type,int method,int stack){assert(type==h.UsePacket && method==h.UsePacketCtor && stack==12);calls.push_back(4);return 13;}
} env;
struct Frame {Env* env=&::env;int mc=8;int player(){return 9;}};
bool Good(Env*){return good;}
jobject UsePacket(Env* environment,jobject stack){return environment->NewObject(h.UsePacket,h.UsePacketCtor,stack);}
namespace java {bool SendPacket(int packet){assert(packet==13);calls.push_back(5);return true;}}
// @THROW_PRODUCTION@
int main(){
 assert(ThrowHeldItem());assert((calls==std::vector<int>{2,3}));
 calls.clear();controllerResult=true;assert(ThrowHeldItem());assert((calls==std::vector<int>{2,3}));
 calls.clear();failController=true;assert(!ThrowHeldItem());assert((calls==std::vector<int>{2,3}));
 std::cout<<"PASS: one vanilla controller use, native slot sync, no swing/duplicate C08, JNI failure gate\n";
}
