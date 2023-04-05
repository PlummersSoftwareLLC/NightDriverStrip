#include "jsonserializer.h"

namespace JSONSerializer 
{
    uint32_t ToUInt32(CRGB color) 
    {
        return (color.r << 16) | (color.g << 8) | color.b;
    }

    bool SerializeToJSON(JsonObject& jsonObject, CRGBPalette16 palette) 
    {        
        StaticJsonDocument<512> doc;
 
        JsonArray colors = doc.createNestedArray("cs");

        for (auto& color: palette.entries)
            colors.add(ToUInt32(color));
    
        return jsonObject.set(doc.as<JsonObjectConst>());
    }

    CRGBPalette16 DeserializeCRGBPalette16FromJSON(const JsonObjectConst&  jsonObject) 
    {
        CRGB colors[16];

        if (jsonObject.containsKey("cs"))
        {
            int colorIndex = 0;

            JsonArrayConst componentsArray = jsonObject["cs"].as<JsonArrayConst>();
            for (JsonVariantConst value : componentsArray) 
            {
                colors[colorIndex] = CRGB(value.as<uint32_t>());
            
                if (++colorIndex > 15)
                    break;
            }
        }

        return CRGBPalette16(colors); 
    }
}