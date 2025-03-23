# WiFiNuk

 Basado en WifiPAD (https://github.com/masutokw/WiFiPad), adaptado para permitir el uso inalámbrico de un Wii Nunchuk con el sistema de control de telescopios ESP32go (https://github.com/masutokw/esp32go) y OnStepX (https://github.com/hjd1964/OnStepX). Ambas conexiones se realizan a los APs por defecto de cada sistema.

 El software está preparado para usar una placa Wemos D1 Mini (comentar la línea #define ESP01) o un ESP-01, siguiendo estos esquemas básicos de conexionado:

![WifiChuk_WemosMini](https://github.com/user-attachments/assets/243550bf-fb8b-4244-99d0-7d1fd02b9525)

El interruptor permite seleccionar si se conecta a un sistema ESP32go (OFF) o OnStepX (ON).

![WifiChuk_ESP01](https://github.com/user-attachments/assets/ebe15ae3-2dde-4333-a078-cf286f8b1d27)

 Nota: Para conectar a OnStepX en la versión ESP01 deberá cambiarse manualmente la conexión por defecto.
