#include "globals.h"


int hexStringToInt (String hexString){
    //This function is used to a convert hex code stored into a string to an integer reprsenting the value of the hex code if it were a true hex code.
    int hexStringInt = 0;
    char c[hexString.length()+1];
    hexString.toCharArray(c, hexString.length()+1);
    hexStringInt = strtol(c, 0, 16);
    return hexStringInt;
}

CRGB hexToCrgb (String hexString) {
    //This function converts a hex code stored as a tring to a CRGB color object.
    hexString.replace("#","");
    if (hexString.length() < 6){
        while (hexString.length() < 6 ) {
            hexString += "0";
        }
    }
    String rHexString = hexString.substring(0,2);
    String gHexString = hexString.substring(2,4);
    String bHexString = hexString.substring(4,6);
    

   CRGB myColor = CRGB(100,50,25);
   if (rHexString.length() == 2 && gHexString.length() == 2 && bHexString.length() == 2 ){
    debugI("all hex strings are 2 characters long\n");
    
        //myColor = CRGB(25,50,25);
        //debugI("setting rhexstring to rhexInt\n");
        int rHexInt = hexStringToInt(rHexString);
    
        //debugI("setting bhexstring to bhexInt\n");
        int bHexInt = hexStringToInt(bHexString);
    
        //debugI("setting ghexstring to ghexInt\n");
        int gHexInt = hexStringToInt(gHexString);
        myColor = CRGB (rHexInt,gHexInt,bHexInt);
        //debugI("Hex strings r %s g %s b %s\n", rHexString, gHexString, bHexString);
        //debugI("Color parts integer is rHexInt %i\n",rHexInt);
    } else {
        //debugI("Not all hex strings are 2 characters long. HEx has length  %i r is %i b is %i g is %i\n", hexString.length(),rHexString.length(),gHexString.length(),bHexString.length());
        //debugI("Hex strings r %s g %s b %s\n", rHexString, gHexString, bHexString);
        myColor = CRGB(5,5,5);
    }
    return myColor;
}

void UserRemoteControl::getRemoteButtons() {
    //debugI("We are creating the user remote");
    /*
    The ButtonActions ENUM in userremote.h will tell you what actions are available.
    Eventually, the key definitions will be brought in from a config file of some kind.
    One issue with that is how we represent the keycode. A config file will save that as a string and not an integer in hex notation.
    It should be assumed that said text file will interact with the webserver, thus the hex code / integer will be passed to the backend as a string.
    On the front end, we can convert the user inputed hex value to an integer. On the back end we convert that string into an integer.
    There are some c++ methods / libraries that will convert the string into an integer as if the string is an integer. I.e. "1234" is converted to 1234 not the hex value of "1234" converted to an int.
    This scenario was ralized when working with the RHB hex values defined here.
    */

    //Row 1
    buttons.push_back(RemoteButton ("Brightness Up",0xFF3AC5,BRIGHTNESS_UP, ""));
    buttons.push_back(RemoteButton ("Brightness Down",0xFFBA45,BRIGHTNESS_DOWN, ""));
    buttons.push_back(RemoteButton ("Power On",0xFF827D,POWER_ON, ""));
    buttons.push_back(RemoteButton ("Power Off",0xFF02FD,POWER_OFF, ""));
    //Row 2
    buttons.push_back(RemoteButton ("Full Red",0xFF1AE5,FILL_COLOR, "FF0000"));
    buttons.push_back(RemoteButton ("Full Green",0xFF9A65,FILL_COLOR, "00FF00"));
    buttons.push_back(RemoteButton ("Full Blue",0xFFA25D,FILL_COLOR, "0000FF"));
    buttons.push_back(RemoteButton ("Full White",0xFF22DD,FILL_COLOR, "999999")); //because we don't want FULL white
    //Row 3
    buttons.push_back(RemoteButton ("Color 1",0xFF2AD5,FILL_COLOR, "E18E28"));
    buttons.push_back(RemoteButton ("Color 2",0xFFAA55,FILL_COLOR, "1B9205"));
    buttons.push_back(RemoteButton ("Color 3",0xFF926D,FILL_COLOR, "170C96"));
    buttons.push_back(RemoteButton ("Color 4",0xFF12ED,FILL_COLOR, "F1BFDC"));
    //Row 4
    buttons.push_back(RemoteButton ("Color 5",0xFF0AF5,FILL_COLOR, "FBBE56"));
    buttons.push_back(RemoteButton ("Color 6",0xFF8A75,FILL_COLOR, "229248"));
    buttons.push_back(RemoteButton ("Color 7",0xFFB24D,FILL_COLOR, "200991"));
    buttons.push_back(RemoteButton ("Color 8",0xFF32CD,FILL_COLOR, "D72FB9"));
    //Row 5
    buttons.push_back(RemoteButton ("Color 9",0xFF38C7,FILL_COLOR, "EDD917"));
    buttons.push_back(RemoteButton ("Color 10",0xFFB847,FILL_COLOR, "2A92A1"));
    buttons.push_back(RemoteButton ("Color 11",0xFF7887,FILL_COLOR, "7415B4"));
    buttons.push_back(RemoteButton ("Color 12",0xFFF807,FILL_COLOR, "39AAC3"));
    //Row 6
    buttons.push_back(RemoteButton ("Color 13",0xFF18E7,FILL_COLOR, "D0E30F"));
    buttons.push_back(RemoteButton ("Color 14",0xFF9867,FILL_COLOR, "2E7CC7"));
    buttons.push_back(RemoteButton ("Color 15",0xFF58A7,FILL_COLOR, "C121B1"));
    buttons.push_back(RemoteButton ("Color 16",0xFFD827,FILL_COLOR, "2D87D7"));
    
    //Row 7
    buttons.push_back(RemoteButton ("Jump 3",0xFF28D7,JUMP3, ""));
    buttons.push_back(RemoteButton ("Jump 7",0xFFA857,JUMP7, ""));
    buttons.push_back(RemoteButton ("Fade 3",0xFF6897,FADE3, ""));
    buttons.push_back(RemoteButton ("Fade 7",0xFFE817,FADE7, ""));
    
    //Row 8
    buttons.push_back(RemoteButton ("Increase Red",0xFF08F7,CHANGER, "10"));
    buttons.push_back(RemoteButton ("Increase Green",0xFF8877,CHANGEG, "10"));
    buttons.push_back(RemoteButton ("Increase Blue",0xFF48B7,CHANGEB, "10"));
    buttons.push_back(RemoteButton ("Quick",0xFFC837,QUICK, ""));//fade speed is fast

    //Row 9
    buttons.push_back(RemoteButton ("Descrease Red",0xFF30CF,CHANGER, "-10"));
    buttons.push_back(RemoteButton ("Descrease Green",0xFFB04F,CHANGEG, "-10"));
    buttons.push_back(RemoteButton ("Decrease Blue",0xFF708F,CHANGEB, "-10"));
    buttons.push_back(RemoteButton ("Quick",0xFFF00F,SLOW, ""));//fade speed is slow

    //Row 10
    buttons.push_back(RemoteButton ("DIY 1",0xFF10EF,DIY1, ""));
    buttons.push_back(RemoteButton ("DIY 2",0xFF906F,DIY2, ""));
    buttons.push_back(RemoteButton ("DIY 3",0xFF50AF,DIY3, ""));
    buttons.push_back(RemoteButton ("Auto",0xFFD02F,AUTO, ""));
    //Row 11
    buttons.push_back(RemoteButton ("DIY 4",0xFF20DF,DIY4, ""));
    buttons.push_back(RemoteButton ("DIY 5",0xFFA05F,DIY5, ""));
    buttons.push_back(RemoteButton ("DIY 6",0xFF609F,DIY6, ""));
    buttons.push_back(RemoteButton ("FLASFlash",0xFFE01F,FLASH, ""));

}



