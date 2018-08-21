# Wemos-Lolin-Mqtt

This is a simple project that displays text on a Wemos LOLIN32 with a integrated 128x64 OLED display.
It uses MQTT messages to change the content of the display.

## Bill of Materials

- [Wemos LOLIN32 with integrated OLED display](https://www.aliexpress.com/wholesale?catId=0&initiative_id=&SearchText=wemos+esp32+oled)

## Configuration

You have to change the definitions on the start of the code with your WiFi and MQTT broker information. Those informations are on a comment block like this:
```c++
/////////////////////////////////////////
// CHANGE TO MATCH YOUR CONFIGURATION  //
/////////////////////////////////////////
```

## Usage

Your MQTT message should have this format:

```json
{  
  "line1":"This is the first line",
  "line1Size":10,
  "line2":"This is the second line",
  "line2Size":16,
  "line3":"That's the 3rd line",
  "line3Size":24
}
```

The font sizes supported are: `10,16,24`. Any other value will be defaulted to 16.
