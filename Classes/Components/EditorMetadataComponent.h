#ifndef __EDITOR_METADATA_COMPONENT_H__
#define __EDITOR_METADATA_COMPONENT_H__

#include "cocos2d.h"
#include "CommonDefines.h"

_CSTART

class EditorMetadataComponent : public cocos2d::Component
{
public:
    static EditorMetadataComponent* create()
    {
        EditorMetadataComponent *pRet = new(std::nothrow) EditorMetadataComponent();
        if (pRet && pRet->init())
        {
            pRet->autorelease();
            return pRet;
        }
        else
        {
            delete pRet;
            pRet = nullptr;
            return nullptr;
        }
    }
    
    std::string type;
    std::string textureFileName;
    
    virtual bool init() override 
    { 
        setName("EditorMetadata");
        return true; 
    }
};

_CEND

#endif

