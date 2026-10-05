#include "minecraft/java/api/class_edit.h"
#include "minecraft/java/api/classfile.h"
#include "minecraft/hooks/Edits.h"
#include "../bytecode/audit.h"
#include <fstream>
#include <cstdio>
#include <string>
int main(int argc,char** argv){
 if(argc!=3)return 2;
 nova::hook::SelectLunarEdits(std::string(argv[1])=="lunar");
 int count=0;auto edits=nova::hook::AllEdits(count);
 std::ifstream in(argv[2],std::ios::binary);std::vector<u1> bytes{std::istreambuf_iterator<char>(in),{}};
 if(bytes.empty())return 3;
 for(int i=0;i<count;++i)for(int j=0;j<edits[i].appendCount;++j){
  const auto& a=edits[i].appends[j];if(std::string(a.hook)!="attackTarget")continue;
  ClassFileEditor editor(bytes.data(),bytes.size());
  auto ref=editor.addMethodRef("nova/test/Bridge","attackTarget",a.hookDesc);
  auto code=nova::hook::AppendCode(ref,a.loads,a.loadCount);
  bool ok=editor.prependToMethodCode(a.method,a.methodDesc,code,a.loadCount);std::string err;
  ok=ok&&audit::AuditClass(editor.serialize(),err);
  std::printf("%s controller attack-head hook: %s %s\n",argv[1],ok?"passed":"FAILED",err.c_str());return ok?0:1;
 }
 return 4;
}
