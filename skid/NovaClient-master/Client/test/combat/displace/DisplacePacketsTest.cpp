#include <cassert>
#include <cstdio>
#include <functional>
#include <mutex>
#include <utility>
#include <vector>
struct Object{int id;};using jobject=Object*;
int refs=0,failAfter=-1;
std::vector<int> sent,callbacks;
std::function<void()> observer;
struct JNIEnv {
    bool IsSameObject(jobject a,jobject b){return a==b;}
    jobject NewGlobalRef(jobject p){if(failAfter==0)return nullptr;if(failAfter>0)--failAfter;++refs;return p;}
    void DeleteGlobalRef(jobject p){if(p)--refs;}
    bool ExceptionCheck(){return false;}
    void ExceptionClear(){}
} env;
struct JniAttach{JNIEnv* env=&::env;explicit operator bool()const{return true;}};
namespace nova::java {
int currentPlayer=1,currentWorld=2;
struct EntityHandle {
    int id=0;
    EntityHandle()=default;explicit EntityHandle(int id):id(id){}
    EntityHandle(EntityHandle&& o)noexcept:id(std::exchange(o.id,0)){}
    EntityHandle& operator=(EntityHandle&& o)noexcept{id=std::exchange(o.id,0);return *this;}
    explicit operator bool()const{return id!=0;}
    bool IsSame(const EntityHandle& o)const{return id&&id==o.id;}
    void Reset(){id=0;}
};
EntityHandle PlayerEntity(){return EntityHandle{currentPlayer};}
EntityHandle LevelEntity(){return EntityHandle{currentWorld};}
}
namespace nova::compat::lunar {
bool SendDisplacePacket(JNIEnv*,jobject,jobject p,jobject cb){sent.push_back(p->id);if(cb)callbacks.push_back(cb->id);if(observer)observer();return true;}
}
// @PRODUCTION_PACKETS@
int main(){
    using namespace nova::module::displace;
    Object manager{1},otherManager{2},move{3},ack{4},chat{5},callback{6};
    assert(!HoldOutput(&env,&manager,&move,nullptr));
    BeginBlink();assert(HoldOutput(&env,&manager,&move,&callback));
    assert(HoldOutput(&env,&manager,&ack,nullptr));assert(HoldOutput(&env,&manager,&chat,nullptr));
    assert(sent.empty()&&refs==7);BeginBlink();EndBlink(true);
    assert((sent==std::vector<int>{3,4,5})&&(callbacks==std::vector<int>{6})&&refs==0);
    assert(!HoldOutput(&env,&manager,&move,nullptr));
    // Other output owners can release through this FIFO without surrendering
    // their ownership; all actual dispatches retain manager/callback identity.
    sent.clear();BeginBlink();HoldOutput(&env,&manager,&ack,nullptr);
    HoldOutput(&env,&manager,&move,nullptr);EndBlink(true);
    assert((sent==std::vector<int>{4,3})&&refs==0);
    BeginBlink();HoldOutput(&env,&manager,&move,&callback);EndBlink(false);assert(refs==0&&sent.size()==2);
    for(bool changeWorld:{false,true}){
        BeginBlink();HoldOutput(&env,&manager,&move,nullptr);
        if(changeWorld)++nova::java::currentWorld;else ++nova::java::currentPlayer;
        EndBlink(true);assert(refs==0&&sent.size()==2);
    }
    BeginBlink();HoldOutput(&env,&manager,&move,nullptr);
    assert(!HoldOutput(&env,&otherManager,&ack,nullptr));assert(refs==0&&sent.size()==2);
    // Test every partially allocated packet reference, including callbacks.
    for(int fail=0;fail<3;++fail){
        BeginBlink();HoldOutput(&env,&manager,&move,nullptr);
        failAfter=fail;assert(!HoldOutput(&env,&manager,&ack,&callback));failAfter=-1;
        assert(refs==0);assert(sent.back()==3);
    }
    nova::java::currentWorld=0;BeginBlink();assert(!HoldOutput(&env,&manager,&move,nullptr));EndBlink(false);
    std::puts("PASS: actual Displace output FIFO, callbacks, repeated begin, release/drop, world/player/manager replacement and partial JNI allocation failures");
}
