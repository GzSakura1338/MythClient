#include <cassert>
#include <cstdio>
#include <functional>
#include <memory>
#include <mutex>
#include <chrono>
#include <string>
#include <vector>
#include <random>
#include "minecraft/modules/render/cswin/VictoryText.h"
namespace mock {
int interest=2,plays=0,draws=0;bool player=true;std::string text="VICTORY!",name="NovaUser";
bool nick=false;
std::function<void()> onRead;
struct Clock:std::chrono::steady_clock {static inline time_point current{};static time_point now(){return current;}};
}
namespace Properties {
struct BoolProperty {bool value=false;bool get()const{return value;}};
struct ModeProperty {int value=0;int get()const{return value;}};
}
namespace nova::event {
enum class EventType{Incoming,Outgoing,Pre,Post};
struct PacketEvent{EventType type=EventType::Incoming;int packet=1;bool cancelled=false;bool IsCancelled()const{return cancelled;}};
struct GameTickEvent{EventType type=EventType::Pre;};struct RespawnEvent{};
namespace Priority{constexpr int Lowest=20;}
template<class E>std::function<void(E&)> callback;
struct Subscription{std::function<void()> reset;void Reset(){if(reset){reset();reset={};}}};
struct EventBus{
static EventBus& Get(){static EventBus bus;return bus;}
template<class E,class F>Subscription Subscribe(F f,int=10){callback<E> = f;return {[]{callback<E> = {};}};}
};
template<class E>void Post(E e={}){if(callback<E>)callback<E>(e);}
}
namespace nova::ui::cswin {
constexpr double Duration=12.016;
enum class Style{CT,T};
struct Snapshot{std::string name="Player";int portrait=0;Style style=Style::T;};
enum class Track{Inhuman,EZ4ENCE};
struct Audio{
    static inline bool playing=false;
    static inline Track last=Track::Inhuman;
    bool Play(Track track){++mock::plays;playing=true;last=track;return true;}
    double DurationSeconds()const{return last==Track::EZ4ENCE?6.3:11.99;}
    void Stop(){playing=false;}
};
Style drawnStyle=Style::T;
std::string drawnName;
void Draw(double,const Snapshot& player,double){++mock::draws;drawnStyle=player.style;drawnName=player.name;}
}
namespace nova::minecraft {int PlayerPortrait(int i){return i;}}
namespace nova::java {
std::string PresentationName(std::string_view original){return mock::nick&&original=="NovaUser"?"You":std::string(original);}
void SetIncomingPacketsWanted(bool on){mock::interest+=on?1:-1;}
bool IsDisconnectPacket(int p){return p==2;}bool IsRespawnPacket(int p){return p==3;}bool IsChatSessionStart(int p){return p==4;}
struct Player{bool Valid(){return mock::player;}bool ScoreboardName(std::string& name){name=mock::name;return !name.empty();}};
Player PlayerEntity(){return {}; }int PlayerSkinTexture(Player){return 0;}
struct Announcement{std::string text;};bool ReadAnnouncement(int p,Announcement& out){out.text=mock::text;if(mock::onRead){auto callback=std::move(mock::onRead);mock::onRead={};callback();}return p==1;}
}
namespace nova::module {
enum class Category{Render};
class Module{
public:
    Module(const char*,Category,const char*,const char*){}virtual ~Module()=default;
    Properties::BoolProperty option;bool enabled=false;
    Properties::ModeProperty music;
    Properties::ModeProperty style;
    void setEnabled(bool v){enabled=v;}
    void Enable(){enabled=true;onEnable();}void Disable(){enabled=false;onDisable();}
    void Update(){onUpdate();}void Render(){onRender();}
protected:
    virtual void onEnable(){}virtual void onDisable(){}virtual void onUpdate(){}virtual void onRender(){}
    void enableOverlay(){}
    Properties::BoolProperty& addBool(std::string label,bool value){assert(label=="Only On Victory" && !value);option.value=value;return option;}
    Properties::ModeProperty& addMode(std::string label,int value,std::vector<std::string> choices){
        if(label=="Style") {assert(value==1 && choices==std::vector<std::string>({"CT","T","Auto"}));style.value=value;return style;}
        assert(label=="Music" && value==0 && choices==std::vector<std::string>({"Inhuman","EZ4ENCE","Auto","None"}));music.value=value;return music;
    }
};
}
// @PRODUCTION_MODULE@
int main(){
    using namespace nova;using event::Post;
    auto m=module::MakeCSWinModule();
    m->Enable();assert(mock::plays==1 && mock::interest==2); // Default retains manual playback.
    mock::player=false;Post<event::GameTickEvent>();m->Update();m->Render();assert(mock::draws==1);
    mock::Clock::current+=std::chrono::milliseconds(12017);m->Update();
    assert(!m->enabled && !ui::cswin::Audio::playing); // Manual playback still auto-disables.
    m->Disable();mock::player=true;m->option.value=true;m->Enable();
    assert(mock::plays==1 && mock::interest==3);
    Post<event::GameTickEvent>();m->Update();m->Render();assert(mock::draws==1); // Waiting is invisible.
    for(auto text:{"Alice: Victory!","DEFEAT!","Winner: SomeoneElse"}){mock::text=text;Post<event::PacketEvent>();m->Update();}
    assert(mock::plays==1);mock::text="胜利！";
    Post<event::PacketEvent>({event::EventType::Outgoing,1});Post<event::PacketEvent>({event::EventType::Incoming,1,true});m->Update();assert(mock::plays==1);
    Post<event::PacketEvent>();Post<event::PacketEvent>();assert(mock::plays==1); // No audio on receive thread.
    m->Update();assert(mock::plays==2);Post<event::PacketEvent>();m->Update();assert(mock::plays==2);
    for(int boundary:{2,3,4}) {
        Post<event::PacketEvent>({event::EventType::Incoming,boundary});
        mock::player=false;Post<event::GameTickEvent>();Post<event::RespawnEvent>();m->Update();
        assert(ui::cswin::Audio::playing && m->enabled && mock::plays==2);
        mock::player=true;Post<event::GameTickEvent>();
    }
    mock::Clock::current+=std::chrono::milliseconds(12017);m->Update();
    assert(m->enabled && !ui::cswin::Audio::playing && mock::interest==3);
    m->Disable();assert(mock::interest==2);
    // A recognized win survives both packet-level transfer and the applied
    // respawn event, including a temporarily absent player before rendering.
    for(int boundary:{2,3,4}){
        const int before=mock::plays;
        m->Enable();Post<event::GameTickEvent>();Post<event::PacketEvent>();
        Post<event::PacketEvent>({event::EventType::Incoming,boundary});Post<event::RespawnEvent>();
        mock::player=false;Post<event::GameTickEvent>();m->Update();m->Render();
        assert(mock::plays==before+1 && ui::cswin::drawnName=="NovaUser");
        mock::player=true;m->Disable();
    }
    int before=mock::plays;
    m->Enable();Post<event::GameTickEvent>();Post<event::PacketEvent>();Post<event::RespawnEvent>();
    mock::Clock::current+=std::chrono::seconds(30);m->Update();assert(mock::plays==before+1);m->Disable();
    // Victory arriving before the first player tick must still start; the next
    // tick fills the portrait rather than being required for detection.
    before=mock::plays;m->Enable();mock::text="You won! Want to play again? Click here!";
    mock::player=false;Post<event::PacketEvent>();Post<event::GameTickEvent>();m->Update();
    assert(mock::plays==before+1);mock::player=true;Post<event::GameTickEvent>();m->Render();
    assert(ui::cswin::drawnName=="NovaUser");m->Disable();
    // A world transition while text is being decoded must not discard the win.
    before=mock::plays;m->Enable();Post<event::GameTickEvent>();
    mock::onRead=[]{Post<event::RespawnEvent>();};Post<event::PacketEvent>();m->Update();
    assert(mock::plays==before+1);m->Disable();
    // Disabling the module still cancels both queued and concurrently decoded wins.
    before=mock::plays;m->Enable();Post<event::PacketEvent>();m->Disable();m->Enable();m->Update();
    assert(mock::plays==before);
    mock::onRead=[&]{m->Disable();m->Enable();};Post<event::PacketEvent>();m->Update();
    assert(mock::plays==before);
    // An old playback may have expired before the render/update thread notices.
    Post<event::PacketEvent>();m->Update();assert(mock::plays==before+1);
    mock::Clock::current+=std::chrono::seconds(13);Post<event::PacketEvent>();m->Update();
    assert(mock::plays==before+2);m->Disable();
    // Reset counters for the original manual/music/style regression cases.
    mock::plays=2;mock::draws=1;m->Enable();
    m->option.value=false;m->Update();assert(mock::plays==3 && mock::interest==2);
    m->option.value=true;m->Update();assert(mock::interest==3);m->Render();assert(mock::draws==1);
    mock::name="Player";Post<event::GameTickEvent>();mock::text="Winner: Player";Post<event::PacketEvent>();m->Update();assert(mock::plays==4);
    mock::Clock::current+=std::chrono::milliseconds(12017);m->Update();m->Render();
    assert(m->enabled && !ui::cswin::Audio::playing && mock::draws==1 && mock::interest==3);
    m->Update();assert(mock::plays==4); // Do not replay without a new announcement.
    Post<event::GameTickEvent>();Post<event::PacketEvent>();Post<event::PacketEvent>();m->Update();
    assert(m->enabled && ui::cswin::Audio::playing && mock::plays==5 && mock::interest==3);
    m->Disable();m->Disable();assert(mock::interest==2);
    m->Enable();m.reset();assert(mock::interest==2 && !event::callback<event::PacketEvent>);
    m=module::MakeCSWinModule();m->music.value=1;m->Enable();
    assert(ui::cswin::Audio::last==ui::cswin::Track::EZ4ENCE);
    m->music.value=0;m->Update();assert(ui::cswin::Audio::last==ui::cswin::Track::EZ4ENCE); // Changes apply to next play.
    mock::Clock::current+=std::chrono::milliseconds(6299);m->Update();assert(m->enabled);
    mock::Clock::current+=std::chrono::milliseconds(2);m->Update();assert(!m->enabled);m->Disable();
    m->Enable();assert(ui::cswin::Audio::last==ui::cswin::Track::Inhuman);m->Disable();
    m->music.value=2;int counts[2]{};
    for(int i=0;i<64;++i){m->Enable();++counts[int(ui::cswin::Audio::last)];m->Disable();}
    assert(counts[0]>0 && counts[1]>0 && mock::interest==2);
    m->music.value=3;const int playsBeforeNone=mock::plays,drawsBeforeNone=mock::draws;
    m->Enable();m->Render();assert(mock::plays==playsBeforeNone && !ui::cswin::Audio::playing && mock::draws==drawsBeforeNone+1);
    mock::Clock::current+=std::chrono::milliseconds(12017);m->Update();assert(!m->enabled);m->Disable();
    m->option.value=true;m->Enable();Post<event::GameTickEvent>();Post<event::PacketEvent>();m->Update();m->Render();
    assert(mock::plays==playsBeforeNone && !ui::cswin::Audio::playing && mock::draws==drawsBeforeNone+2);
    mock::Clock::current+=std::chrono::milliseconds(12017);m->Update();assert(m->enabled);m->Disable();m->option.value=false;
    for(int selected:{0,1}) {
        m->style.value=selected;m->Enable();m->Render();assert(int(ui::cswin::drawnStyle)==selected);
        m->style.value=1-selected;m->Update();m->Render();assert(int(ui::cswin::drawnStyle)==selected);
        m->Disable();
    }
    m->style.value=2;int styles[2]{};
    for(int i=0;i<64;++i){m->Enable();m->Render();++styles[int(ui::cswin::drawnStyle)];m->Disable();}
    assert(styles[0]>0 && styles[1]>0);
    m.reset();
    mock::name="NovaUser";mock::text="Winner: NovaUser";mock::nick=true;
    m=module::MakeCSWinModule();m->option.value=true;m->Enable();Post<event::GameTickEvent>();
    int beforeNick=mock::plays;Post<event::PacketEvent>();m->Update();m->Render();
    assert(mock::plays==beforeNick+1&&ui::cswin::drawnName=="You");
    mock::nick=false;m->Render();assert(ui::cswin::drawnName=="NovaUser");
    mock::nick=true;m->Disable();m->Enable();Post<event::GameTickEvent>();
    beforeNick=mock::plays;mock::text="Winner: You";Post<event::PacketEvent>();m->Update();assert(mock::plays==beforeNick);
    mock::text="Winner: NovaUser";Post<event::PacketEvent>();m->Update();assert(mock::plays==beforeNick+1);
    m->Disable();m.reset();mock::nick=false;
    std::puts("PASS: Lunar nick presentation follows toggles; winner identity stays real");
    std::puts("PASS: CSWin persistent victory listening, manual auto-disable, deduplication, session reset and incoming ownership");
    std::puts("PASS: fixed music, per-play random selection and track duration");
    std::puts("PASS: CT/T style, stable during playback, per-play random style");
}
