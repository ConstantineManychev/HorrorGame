#include "ServiceLocator.h"

static ServiceLocator* s_SharedLocator = nullptr;

ServiceLocator* ServiceLocator::getInstance()
{
    if (!s_SharedLocator)
    {
        s_SharedLocator = new (std::nothrow) ServiceLocator();
    }
    return s_SharedLocator;
}

void ServiceLocator::destroyInstance()
{
    CC_SAFE_DELETE(s_SharedLocator);
}


