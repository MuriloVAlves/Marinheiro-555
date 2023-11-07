#include <ESP32Servo.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#define LED  2
#define SERVO       25
#define SSID_REDE  "Marinheiro555"
#define SENHA_REDE  "Segredo123"

//Configuracoes fisicas
Servo meuServo;
bool StatusLED = LOW;

//Variaveis globais
unsigned long agora = millis();
unsigned int anguloServo = 90;

//Configuracoes para o WiFi
IPAddress local_IP(192,168,1,22);
IPAddress gateway(192,168,1,5);
IPAddress subnet(255,255,255,0);

//Configuracoes para o server
WebServer server(80);
WebSocketsServer Socket = WebSocketsServer(81);

//Pagina Web
String site = "<!DOCTYPE html><html> <head> <title>Marinheiro 555</title> <style type='text/CSS'> #header{ background-color: #547B94; border-radius: 10px; padding: 5px; width: 100%; overflow: hidden; height: 20%; } .titulo{ font-family: Brush Script MT, Brush Script Std, cursive; text-align: center; font-size: 30px; /*color: #c9a959;*/ color: #e6c99f; } body{ background-color: #f4f2ef; background-size: 100% 100%; overflow: hidden; } .wrapper{ width: 80%; padding: 5%; } #powerbtn{ border-radius: 5px; text-shadow: 5px; border: 0px; padding-top: 10px; padding-bottom: 10px; font-size: 20px; width: 100%; height: 10em; } .OFF{ background-color: #547B94; color: #e6c99f; } .ON{ background-color: #e4850f; color: #2B2519; } .center{ display: flex; justify-content: center; align-items: center; height: 100%; /*border: 3px solid green;*/ } .slidecontainer { position: absolute; width: 100%; /* Width of the outside container */ } .slider { -webkit-appearance: none; appearance: none; width: 80%; height: 10em; background: #d3d3d3; outline: none; opacity: 0.7; -webkit-transition: .2s; transition: opacity .2s; } .slider:hover { opacity: 1; } .slider::-webkit-slider-thumb{ -webkit-appearance: none; appearance: none; width: 20em; height: 20em; border-radius: 50%; background: #2076ac; cursor: pointer; } .spacer{ padding-top: 10%; } .angle{ display: flex; justify-content: center; align-items: center; height: 100%; padding: 2%; font-size: 20px; } .angle { height: 10em; font-size: 20px; } </style> </head> <body> <header id='header'> <div class='titulo'> <h1>Marinheiro 555</h1> </div> </header> <div class='wrapper'> <div class='center'> <input type='button' value='Ligar' id='powerbtn' class='OFF'> </div> <div class = 'spacer'></div> <div class='angle'>Angulo: <span id='angulo'></span></div> <div class='slidecontainer'> <input type='range' min='0' max='180' value='90' class='slider' id='anguloServo'> </div> </div> <footer> </footer> <script> var Socket; var angulo = document.getElementById('anguloServo'); var resposta = document.getElementById('angulo'); resposta.innerText = angulo.value; var botao = document.getElementById('powerbtn'); botao.addEventListener('click', powerChanged); angulo.addEventListener('input', sendServo); function startWebSocket(){ Socket = new WebSocket('ws://'+window.location.hostname+':81/'); Socket.onmessage = function(event){ processarComando(event); } } function processarComando(event) { console.log(event.data); let dados = event.data.split(';'); switch (dados[0]){ case 'Ligado': botao.className = 'ON'; botao.value = 'Desligar'; break; case 'Desligado': botao.className = 'OFF'; botao.value = 'Ligar'; break; } let anguloESP32 = dados[1]; console.log(anguloESP32); angulo.value = anguloESP32; resposta.innerText = angulo.value; } function powerChanged(){ console.log(botao.value); handleSocketSend(botao.value); if(botao.value == 'Ligar'){ botao.className = 'ON'; botao.value = 'Desligar'; } else{ botao.className = 'OFF'; botao.value = 'Ligar'; } } function sendServo(){ resposta.innerText = angulo.value; console.log(angulo.value); handleSocketSend(angulo.value); } function handleSocketSend(valor){ Socket.send(valor); } window.onload = function(event){ startWebSocket(); } </script> </body></html>";

//Funcao para simbolizar o status do led
void changeStatus(){
  StatusLED = !StatusLED;
  digitalWrite(LED,StatusLED);
}
void statusHigh(){
  StatusLED = HIGH;
  digitalWrite(LED,StatusLED);
}

void statusLow(){
  StatusLED = LOW;
  digitalWrite(LED,StatusLED);
}

void Blink(int vezes, int tempo){
  for(int i = 0; i < 2*vezes; i++){
    changeStatus();
    delay(tempo/(2*vezes));
  }
}

void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length){
  switch(type){
    case WStype_DISCONNECTED:
      Serial.println("Cliente desconectado");
      Blink(3,300);
      break;
      
    case WStype_CONNECTED:
      Serial.println("Cliente conectado");
      Blink(3,300);
      BroadcastStatus();
      break;
      
    case WStype_TEXT:
      String resposta = "";
      for (int i = 0; i < length; i++){
        resposta += (char)payload[i];
      }
      Serial.println(resposta);
      if (resposta == "Ligar"){
        statusHigh();
        BroadcastStatus();
      }
      else if (resposta == "Desligar"){
        statusLow();
        BroadcastStatus();
      }
      else{
        anguloServo = resposta.toInt();
        meuServo.write(anguloServo); 
        agora = millis();
      }
      break;
  }
}
void BroadcastStatus(){
      String str = "";
      switch(StatusLED){
        case HIGH:
        str = "Ligado";
        break;
        case LOW:
        str = "Desligado";
        break;
      }
      //Colocar o angulo do servo para o broadcast
      str += ";"+ String(anguloServo);
      int str_len = str.length()+1;
      char char_arr[str_len];
      str.toCharArray(char_arr,str_len);
      Socket.broadcastTXT(char_arr);
}

void setup() {  
  //Incicializando o servo motor
  meuServo.attach(SERVO);
  
  //Inicializando as saidas do ESP
  pinMode(LED,OUTPUT);

    //Inicializando a porta Serial
  Serial.begin(115200);
  Serial.println("Inicializando...");
  
  //Inicializando o WiFi
  
  Serial.print("Configurando o ponto de acesso... ");
  Serial.println(WiFi.softAPConfig(local_IP,gateway,subnet) ? "Pronto": "Falhou");

  Serial.print("Criando ponto de acesso... ");
  Serial.println(WiFi.softAP(SSID_REDE,SENHA_REDE)? "Pronto": "Falhou");

  Serial.print("Endereco de IP - ");
  Serial.println(WiFi.softAPIP());

  //Para conectar a um roteador
  //WiFi.begin(SSID_REDE,SENHA_REDE);  
  // Serial.print("Conectando...");
  //   while(WiFi.status() != WL_CONNECTED){
  //     Serial.print('.');
  //     delay(500);
  //     changeStatus();
  //   }
  //  Serial.println("");
  //  Serial.print("Conectado com o IP: ");
  //  Serial.println(WiFi.localIP());
  //  statusLow();

   
  server.on("/", []() {
    server.send(200,"text/html",site);
  });

  server.begin();
  Socket.begin();
  Socket.onEvent(webSocketEvent);
}

void loop() {
  server.handleClient();
  Socket.loop();
  if (millis() - agora > 500){
    BroadcastStatus();
    //Serial.println("Broadcasting");
    agora = millis();
  }
}
