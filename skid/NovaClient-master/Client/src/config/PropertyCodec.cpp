#include "config/PropertyCodec.h"

namespace nova::config {
namespace {
class NameKeys {
public:
    std::string operator()(const char* base) {
        for(auto& entry:used_)if(entry.first==base) {
            ++entry.second;return entry.first+"#"+std::to_string(entry.second);
        }
        used_.emplace_back(base,1);return base;
    }
private:
    std::vector<std::pair<std::string,int>> used_;
};
std::vector<Properties::PropDesc> Children(const Properties::PropDesc& row) {
    std::vector<Properties::PropDesc> children;
    for(const auto& child:row.getSubProps())
        if((child.getProp()!=nullptr && child.getProp()!=row.getProp()) ||
            (child.getProp()==nullptr && child.hasSubProp()))children.push_back(child);
    return children;
}
}
json::Value SaveRows(const std::vector<Properties::PropDesc>& rows) {
    auto out=json::Value::Object();NameKeys keys;
    for(const auto& row:rows) {
        auto* property=row.getProp();const auto key=keys(row.getName());
        const auto children=Children(row);
        if(!property) {if(!children.empty())out[key]=SaveRows(children);continue;}
        json::Value value;property->save(value);
        if(children.empty())out[key]=std::move(value);
        else {
            auto entry=json::Value::Object();entry["value"]=std::move(value);entry["sub"]=SaveRows(children);
            out[key]=std::move(entry);
        }
    }
    return out;
}
void LoadRows(const json::Value& in,const std::vector<Properties::PropDesc>& rows) {
    if(!in.isObject())return;
    NameKeys keys;
    for(const auto& row:rows) {
        auto* property=row.getProp();const auto* entry=in.Find(keys(row.getName()));
        const auto children=Children(row);
        if(!entry)continue;
        if(!property) {if(!children.empty())LoadRows(*entry,children);continue;}
        // Old scalar settings remain readable after acquiring a submenu.
        if(children.empty() || !entry->isObject()) {property->load(*entry);continue;}
        if(const auto* own=entry->Find("value"))property->load(*own);
        if(const auto* sub=entry->Find("sub"))LoadRows(*sub,children);
    }
}
}
