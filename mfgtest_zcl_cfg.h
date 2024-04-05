#include "config.h"
#include "app/framework/plugin/osramds-basic-server/BasicServer.h"
//#include "GlobalDefs.h"

#define APP_DEFAULT_APPLICATION_VERSION_ATTRIBUTE_VALUE     APPLICATION_VERSION
#define APP_DEFAULT_HW_VERSION_ATTRIBUTE_VALUE                 HW_VERSION_MAJOR
#define APP_DEFAULT_POWER_SOURCE_ATTRIBUTE_VALUE                   POWERSOURCE

//----------------------------------------------------------------------------------------------------------------------
// Model Identifier (string of 15 char, or less)
//----------------------------------------------------------------------------------------------------------------------
#define APP_DEFAULT_MODEL_IDENTIFIER_ATTRIBUTE_VALUE             MODELIDENTIFIER
//----------------------------------------------------------------------------------------------------------------------
// Software build (string of 15 char, or less)
//----------------------------------------------------------------------------------------------------------------------
#define APP_DEFAULT_SW_BUILD_ID_ATTRIBUTE_VALUE                                        swBuildVersion.str
