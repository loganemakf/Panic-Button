#include "secrets.hpp" // make sure this file exists!
#include <Arduino.h>
#include <DFRobot_DF1201S.h>
#include <ESP_Mail_Client.h>
#include <Preferences.h>
#include <WiFi.h>
typedef esp_mail_session_config_t ESP_Mail_Session;

#define KILL_PIN D3
#define BTN_LED_PIN D2
#define PANIC_LED_1 D8
#define PANIC_LED_2 D7
#define PANIC_LED_3 D4
#define PANIC_LED_4 D9

#ifndef SECRETS_HPP
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASS "your_wifi_pass"
const char *my_email = "your_email_addr";
const char *email_recips[] = {"other", "email", "recipients"};
const char *email_addrs[] = {"other", "email", "addrs"};
#define SMTP_HOST "your smtp server"
#define SMTP_PORT 465
#define SMTP_EMAIL "your send-from email/login"
#define SMTP_PASS "your send-from email/login password"
#define SMTP_USR_DOM "your user domain"
#define MSG_SENDER_NAME "message sender name"
#define MSG_SENDER_EMAIL "message sender email"
#define MSG_SUBJECT "outgoing message subject"
const char *messages[] = {"messages to send"};
#endif

DFRobot_DF1201S player;
Preferences eepromAccess;
TaskHandle_t FlashButtonTask;
TaskHandle_t PanicLightTask;
TaskHandle_t MP3Task;

// function prototypes
void flashButtonLED(void *);
void lightPanicLEDs(void *);
void initMP3Serial(void *);
void playSound();
void sendEmail(String, String, String);
void killPower();

void flashButtonLED(void *param) {
  pinMode(BTN_LED_PIN, OUTPUT);

  for (;;) {
    digitalWrite(BTN_LED_PIN, HIGH);
    delay(90);
    digitalWrite(BTN_LED_PIN, LOW);
    delay(55);
    digitalWrite(BTN_LED_PIN, HIGH);
    delay(90);
    digitalWrite(BTN_LED_PIN, LOW);
    delay(55);
    digitalWrite(BTN_LED_PIN, HIGH);
    delay(90);
    digitalWrite(BTN_LED_PIN, LOW);
    delay(350);
  }
}

void lightPanicLEDs(void *params) {
  int leds[] = {PANIC_LED_1, PANIC_LED_2, PANIC_LED_3, PANIC_LED_4};

  ledcSetup(4, 5000, 8);
  for (int led : leds) {
    ledcAttachPin(led, 4);
  }

  // smooth (but quick) ramp-up at boot
  for (int i = 0; i < 255; i += 10) {
    ledcWrite(4, i);
    delay(15);
  }

  delay(3000);

  for (;;) {
    ledcWrite(4, 255);
    delay(30);
    ledcWrite(4, 0);
    delay(30);
  }
}

/*
 * Requires Serial to have started already.
 */
void initMP3Serial(void *params) {
  Serial2.begin(115200);
  while (!player.begin(Serial2)) {
    Serial.println("MP3 init failed :(");
  }
  playSound();
  for (;;)
    ;
}

void playSound() {
  player.setPrompt(false);
  player.setLED(false);
  player.setVol(14);
  player.switchFunction(player.MUSIC);
  player.setPlayMode(player.SINGLE);
  player.playFileNum(2);
  player.start();
}

/*
 * Adapted from ESP-Mail-Client examples. Requires Serial to have started
 * already.
 */
void sendEmail(String msgHTML, String recipientName, String recipientEmail) {
  // Define the SMTP Session object which used for SMTP transsport
  SMTPSession smtp;

  // Define the session config data which used to store the TCP session
  // configuration
  ESP_Mail_Session session;

  // Set the session config
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = SMTP_EMAIL;
  session.login.password = SMTP_PASS;
  session.login.user_domain = SMTP_USR_DOM;

  // Set the NTP config time
  session.time.ntp_server = "pool.ntp.org,time.nist.gov";
  session.time.gmt_offset = -5; // -5 = EST
  session.time.day_light_offset = 0;

  // Define the SMTP_Message class variable to handle the message being
  // transport
  SMTP_Message message;

  // Set the message headers
  message.sender.name = MSG_SENDER_NAME;
  message.sender.email = MSG_SENDER_EMAIL;
  message.subject = MSG_SUBJECT;
  message.addRecipient(recipientName, recipientEmail);
  message.html.content = msgHTML;

  // Connect to server with the session config
  smtp.connect(&session);

  // Start sending Email and close the session
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());
}

void killPower() {
  // pulse kill-pin to unlatch power buttom
  digitalWrite(KILL_PIN, HIGH);
  delay(100);
  digitalWrite(KILL_PIN, LOW);
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

void setup() {

  unsigned long bootTime = millis();

  xTaskCreatePinnedToCore(flashButtonLED, "Blink btn LED", 10000, NULL, 1,
                          &FlashButtonTask, 1);

  xTaskCreatePinnedToCore(lightPanicLEDs, "Light Panic sign", 10000, NULL, 1,
                          &PanicLightTask, 1);

  xTaskCreatePinnedToCore(initMP3Serial, "Init mp3 and play", 20000, NULL, 1,
                          &MP3Task, 1);

  Serial.begin(115200);
  pinMode(KILL_PIN, OUTPUT);
  digitalWrite(KILL_PIN, LOW);

  // keep track of the number of button presses using Preferences NV storage
  eepromAccess.begin("panic-btn", false);
  unsigned int btnPresses = eepromAccess.getUInt("presses", 0);
  eepromAccess.putUInt("presses", ++btnPresses);
  eepromAccess.end();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to " WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    // Serial.print('.');
    delay(500);

    // if wifi is taking a long time to connect, just give up to save battery
    if (millis() - bootTime > 15000)
      killPower();
  }
  Serial.println("Connected!");

  int num_emails = sizeof(email_addrs) / sizeof(email_addrs[0]);
  int num_messages = sizeof(messages) / sizeof(messages[0]);
  randomSeed(analogRead(A0));
  long selected_msg_index = random(num_messages);
  long selected_recipient_index = random(num_emails * 5);
  Serial.print("Message: ");
  Serial.println(messages[selected_msg_index]);
  Serial.print("Recipient index: ");
  Serial.println(selected_recipient_index);

  String baseMsg =
      "<h3 style=\"font-style: italic; color: Gray\">This is an "
      "alert message to inform you that </h3>"
      "<h1 style=\"color:red; text-align: center; font-size: 3.5rem;\">";
  baseMsg.concat(messages[selected_msg_index]);
  baseMsg.concat("</h1>");

  Serial.println("Sending email...");

  if (btnPresses > 10 && selected_recipient_index < num_emails) {
    // additionally send email to randomly selected recipient
    sendEmail(baseMsg, email_recips[selected_recipient_index],
              email_addrs[selected_recipient_index]);

    baseMsg.concat("<br /><h5>Recipient: ");
    baseMsg.concat(email_recips[selected_recipient_index]);
    baseMsg.concat("</h5>");
  }

  baseMsg.concat("<br /><h5>Button presses: ");
  baseMsg.concat(btnPresses);
  baseMsg.concat("</h5>");
  sendEmail(baseMsg, my_name, my_email);

  Serial.println("Email(s) sent! Shutting down...");

  // spin for a bit to make sure the sound has time to play
  while (millis() - bootTime < 7500)
    ;

  killPower();
}

void loop() {
  //  put your main code here, to run repeatedly:
}