/*
This file contains items needed to load user defined remote controls.
Eventually this will include loading the data from preferences.
For now, we will hard code the items and let the user manually change.
*/

CRGB hexToCrgb (String hex);
int hexStringToInt (String hexString);



enum ButtonActions{
    BRIGHTNESS_UP,
    BRIGHTNESS_DOWN,
    POWER_ON,
    POWER_OFF,
    FILL_COLOR,
    TRIGGER_EFFECT,
    CHANGER,
    CHANGEG,
    CHANGEB,
    JUMP3,
    JUMP7,
    FADE3,
    FADE7,
    STROBE,
    AUTO,
    FLASH,
    QUICK,
    SLOW,
    DIY1,
    DIY2,
    DIY3,
    DIY4,
    DIY5,
    DIY6
};



class RemoteButton
{
    public:
        String name;
        int keyCode;
        ButtonActions buttonAction;
        String actionArgs;
        RemoteButton (String name, int keyCode, ButtonActions buttonAction, String actionArgs) {
            this->name = name;
            this->keyCode = keyCode;
            this->buttonAction = buttonAction;
            this->actionArgs = actionArgs;
        };
};

   
enum RemoteTypes{
    //don't really need this anymore since we are just defining the buttons as we want
    keys24,
    keys44,
    custom
};

class UserRemoteControl {
    public:
        //Eventually, the button type and / or button count will depend on a user setting / config file.
        std::vector<RemoteButton> buttons {}; 
        //RemoteTypes remoteType;
        void getRemoteButtons();
        int buttonCount;
        int currentBrightness = 200;
        //UserRemoteControl (RemoteTypes remoteType, int buttonCount = 0) {
        UserRemoteControl (int buttonCount = 0) {
            //std::vector<RemoteButton> myButons {};
            //this->buttons = myButons;
            //this->remoteType = remoteType;
            buttonCount = buttonCount;
            /*
            
            switch (remoteType) {
                case RemoteTypes::keys24:
                    this->buttonCount = 24;
                    break;
                case RemoteTypes::keys44:
                    this->buttonCount = 44;
                    break;
                case RemoteTypes::custom:
                    this->buttonCount = buttonCount;
                    break;
            }
            */
            getRemoteButtons();
        };

};
