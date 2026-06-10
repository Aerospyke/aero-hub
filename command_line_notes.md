### "Working" Command for JSBSim/QT JSB/FG
/Applications/FlightGear.app/Contents/MacOS/FlightGear --fdm=null \
--native-fdm=socket,in,30,,5508,udp \
--log-level=warn \
--prop:/sim/ai/enabled=false \
--prop:input/joysticks/js=0 \
--disable-ai-traffic \
--disable-sound \
--disable-real-weather-fetch \
--disable-save-on-exit \
--aircraft=737-300 \
--airport=KLAX \
--runway=25R \
--timeofday=noon \
--fog-nicest \
--geometry=800x600 \
--language=en \
--prop:/autopilot/route-manager/disable-route-manager=true \
--prop:/sim/rendering/random-objects=false \
--prop:/sim/rendering/random-vegetation=false \
--prop:/sim/rendering/particles=false \
--prop:/sim/rendering/fps-display=true \
--prop:/sim/voice=false \
--prop:/sim/traffic-manager/enabled=false

## Note: on macOS, there are render bugs. To resolve, via main menu: View->RenderingOptions->Advanced->"Low Specifications, Use Shaders off


/Applications/FlightGear.app/Contents/MacOS/FlightGear --fdm=null \
--native-fdm=socket,in,60,,5508,udp \
--fg-root="/Users/conor.haines/Library/Application Support/FlightGear/fgdata_2024_1" \
--log-level=warn \
--prop:/sim/ai/enabled=false \
--prop:input/joysticks/js=0 \
--disable-ai-traffic \
--disable-sound \
--disable-real-weather-fetch \
--disable-save-on-exit \
--aircraft=737 \
--airport=KLAX \
--runway=25R \
--timeofday=noon \
--fog-nicest \
--geometry=800x600 \
--language=en \
--prop:/autopilot/route-manager/disable-route-manager=true \
--prop:/sim/rendering/random-objects=false \
--prop:/sim/rendering/random-vegetation=false \
--prop:/sim/rendering/particles=false \
--prop:/sim/rendering/fps-display=true \
--prop:/sim/voice=false \
--prop:/sim/traffic-manager/enabled=false


/Applications/FlightGear.app/Contents/MacOS/FlightGear --fdm=null \
--native-fdm=socket,in,60,,5508,udp \
--fg-root="/Users/conor.haines/Library/Application Support/FlightGear/fgdata_2024_1" \
--log-level=warn \
--prop:/sim/ai/enabled=false \
--prop:input/joysticks/js=0 \
--disable-ai-traffic \
--disable-sound \
--disable-real-weather-fetch \
--disable-save-on-exit \
--aircraft=c172p \
--airport=KLAX \
--runway=25R \
--timeofday=noon \
--fog-nicest \
--geometry=800x600 \
--language=en \
--prop:/autopilot/route-manager/disable-route-manager=true \
--prop:/sim/rendering/random-objects=false \
--prop:/sim/rendering/random-vegetation=false \
--prop:/sim/rendering/particles=false \
--prop:/sim/rendering/fps-display=true \
--prop:/sim/voice=false \
--prop:/sim/traffic-manager/enabled=false

