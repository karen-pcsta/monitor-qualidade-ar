// Modelo de credenciais (equivalente ao .env deste projeto)
// 1. Copiar este arquivo para "secrets.h", na mesma pasta do .ino
// 2. Preencher os dados abaixo
// O secrets.h esta no .gitignore e NAO deve ser publicado

#pragma once

const char* WIFI_NOME      = "";   // rede de 2,4 GHz
const char* WIFI_SENHA     = "";
const char* MQTT_CLIENT_ID = "";   // dispositivo MQTT do ThingSpeak (Devices > MQTT)
const char* MQTT_USUARIO   = "";
const char* MQTT_SENHA     = "";
const char* TOPICO         = "channels/0000000/publish";  // trocar 0000000 pelo Channel ID
