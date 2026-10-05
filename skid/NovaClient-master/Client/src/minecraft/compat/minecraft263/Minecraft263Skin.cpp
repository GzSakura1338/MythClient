#include "minecraft/compat/minecraft263/Minecraft263Game.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/compat/easecation/EaseCation.h"
#include "ui/skin/SkinTextures.h"

namespace nova::compat::mc263 {
using namespace names;
std::uint64_t PlayerSkinTexture(const java::EntityHandle& entity){
    access::Frame f;auto player=static_cast<jobject>(entity.raw());if(!f.instance(player,AbstractClientPlayer))return 0;
    auto skin=f.call(player,AbstractClientPlayer_getSkin);
    auto path=IsEaseCation()?f.call(skin,easecation::names::SkinTexture):f.call(f.call(skin,PlayerSkin_body),ClientAsset_Texture_texturePath);if(!path)return 0;
    auto key=f.text(f.call(path,{"java.lang.Object","toString","()Ljava/lang/String;"}));
    if(auto existing=ui::skins::Find(key))return existing;
    auto manager=f.call(f.client(),Minecraft_getTextureManager);
    auto texture=f.call(manager,TextureManager_getTexture,path);if(!texture)return 0;
    jobject image=nullptr;bool ownImage=false;
    if(f.instance(texture,DynamicTexture))image=f.call(texture,DynamicTexture_getPixels);
    else if(f.instance(texture,ReloadableTexture)){
        auto contents=f.call(texture,ReloadableTexture_loadContents,f.get(manager,TextureManager_Field_resourceManager));
        image=f.call(contents,TextureContents_image);ownImage=image!=nullptr;
    }
    if(!image||(UsesGlfwModernRuntime()?f.get<jlong>(image,easecation::names::ImagePixels)==0:f.call<bool>(image,NativeImage_isClosed)))return 0;
    const int width=f.call<int>(image,NativeImage_getWidth),height=f.call<int>(image,NativeImage_getHeight);
    std::uint64_t result=0;
    if(width>0&&width<=1024&&height>0&&height<=1024){
        auto array=f.call<jintArray>(image,NativeImage_getPixelsABGR);
        if(array&&f.env->GetArrayLength(array)==width*height){
            std::vector<std::uint32_t> pixels(static_cast<size_t>(width)*height);
            f.env->GetIntArrayRegion(array,0,width*height,reinterpret_cast<jint*>(pixels.data()));
            if(f.check())result=ui::skins::Store(key,width,height,std::move(pixels));
        }
    }
    if(ownImage){access::Frame cleanup(f.env);cleanup.invoke(image,NativeImage_close);}
    return result;
}
}
