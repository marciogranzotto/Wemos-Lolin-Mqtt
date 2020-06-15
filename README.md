# Wemos-Lolin-Mqtt

![Project Image](https://i.imgur.com/SDDD0ZT.jpg)

A simple project that displays text on a Wemos LOLIN32 with a integrated 128x64 OLED display.
It uses MQTT messages to change the content of the display.

## Bill of Materials

- [Wemos LOLIN32 with integrated OLED display](https://s.click.aliexpress.com/e/_d7c49Vr)

## Configuration

You have to change the definitions on the start of the code with your WiFi and MQTT broker information. Check the comment block:
```c++
/////////////////////////////////////////
// CHANGE TO MATCH YOUR CONFIGURATION  //
/////////////////////////////////////////
```

## Usage

Your MQTT message should have this format:

```json
{
  "pages": [
    {  
      "1": {"text":"This is the 1st line","size":10},
      "2": {"text":"This is the 2nd line","size":16},
      "3": {"text":"This is the 3rd line","size":21}
    },
    {  
      "1": {"text":"This is the 1st line of the 2nd page","size":10},
      "2": {"text":"This is the 2nd line of the 2nd page","size":16},
      "3": {"text":"This is the 3rd line of the 2nd page","size":21}
    }
  ]
}
```

The font sizes supported are: `10,16,24`. Any other value will be defaulted to 16.

To change between pages, you can use the included button on the back of the board, or change the button pin in the `#define BUTTON_PIN`.

I use Node-RED to send the MQTT messages to it, but you can you whatever you want.

### Align text

I added the ability to align the text right by sending a `true` payload to the topic `YOUR_TOPIC/align_right`. Send `false` again to align it to the left.
This is useful to avoid burn-in.

## EXPERIMENTAL:

### Motion sensor

You can uncomment `#define SENSOR_PIN` and set it to a pin which you can connect a motion sensor.
When the sensor detects motion, it will turn the screen on for `SECONDS_TO_TURN_OFF`.
Every time the sensors gets triggered, it will reset the timer back to 0.
