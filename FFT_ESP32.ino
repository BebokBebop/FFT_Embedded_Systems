#include <Wire.h>
#include "arduinoFFT.h" // Standardowa biblioteka FFT dla arduino
arduinoFFT FFT = arduinoFFT();
#define SAMPLES 1024 // wielkość próbki
#define SAMPLING_FREQUENCY 40000 // częstotliwość pobierania próbki
#define amplitude 200 // amplituda dopasowana do mikrofonu
unsigned int sampling_period_us; // jak długo czekać przed pobraniem kolejnej wartości
próbki
double vReal[SAMPLES]; // tablica przechowująca najpierw wartości próbki dźwięku, a
 // następnie realną część wyniku FFT
double vImag[SAMPLES]; // tablica przechowująca urojoną część wyniku FFT
unsigned long newTime, oldTime; // zmienne służące do odmierzania czasu
byte peak[] = { 0,0,0,0,0,0,0,0 };// przechowuje wysokość kolumny danego zakresu
 //częstotliwości
int displayTab[8][3]; // tablica pośrednia przy translacji
 // peak → translatedDisplayTab
bool translatedDisplayTab[24]; // tablica gotowa do wysłania do urządzeń wyjściowych
//piny odpowiedzialne za sterowanie rejestrami przesuwnymi
int SRCK_pin = 25;
int SERIN_pin = 14;
int DISP_pin = 27;
int CLR_pin = 26;
int RCK_pin = 33;
/////////////////////////////////////////////////////////////////////////
void setup() {
 Serial.begin(115200);
 sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
 pinMode(SRCK_pin, OUTPUT);
 pinMode(DISP_pin, OUTPUT);
 pinMode(SERIN_pin, OUTPUT);
 pinMode(CLR_pin, OUTPUT);
 pinMode(RCK_pin, OUTPUT);
 peak[0] = 0;
 peak[1] = 0;
 peak[2] = 0;
 peak[3] = 0;
 peak[4] = 0;
 peak[5] = 0;
 peak[6] = 0;
 peak[7] = 0;
 digitalWrite(CLR_pin, HIGH);
 digitalWrite(DISP_pin, HIGH);
 digitalWrite(SRCK_pin, LOW);
 digitalWrite(RCK_pin, LOW);
}
void loop() {
 for (int i = 0; i < SAMPLES; i++) {
 newTime = micros() - oldTime;
 oldTime = newTime;
 vReal[i] = analogRead(A0); // Konwersja ADC trwa około 1uS na ESP32
 vImag[i] = 0;
 while (micros() < (newTime + sampling_period_us)) {}// poczekaj przed wczytaniem
 // kolejnej próbki
 }
 //przemnożenie przez maskę (w tym wypadku okna Hamminga)
 FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
 //wykonaj FFT
 FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
 //oblicz wartość absolutną liczby złożonej (vReal[i], vImag[i]) i zapisz do vReal[i]
 FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
 for (int i = 2; i < (SAMPLES / 2); i++) {
 if (vReal[i] > 2000) { // prymitywny filtr szumu
 // Ręcznie dopasowane zakresy częstotliwości
 if (i <= 5 && i > 3) checkPeak(0, (int)vReal[i] / amplitude); // ~125Hz
 if (i > 5 && i <= 12) checkPeak(1, (int)vReal[i] / amplitude); // ~250Hz
 if (i > 12 && i <= 14) checkPeak(2, (int)vReal[i] / amplitude); // ~500Hz
 if (i > 14 && i <= 16) checkPeak(3, (int)vReal[i] / amplitude); // ~1000Hz
 if (i > 16 && i <= 20) checkPeak(4, (int)vReal[i] / amplitude); // ~2000Hz
 if (i > 20 && i <= 24) checkPeak(5, (int)vReal[i] / amplitude); // ~4000Hz
 if (i > 24 && i <= 28) checkPeak(6, (int)vReal[i] / amplitude); // ~8000Hz
 if (i > 28 && i <= 46) checkPeak(7, (int)vReal[i] / amplitude); // ~16000Hz
 }
 }
 translate(); // zamień tablicę peak na ciąg zer i jedynek odpowiadających
// urządzeniom wyjściowym
 displayTable();// prześlij wynik na urządzenia wyjściowe
 // zresetuj wierzchołki
 peak[0] = 0;
 peak[1] = 0;
 peak[2] = 0;
 peak[3] = 0;
 peak[4] = 0;
 peak[5] = 0;
 peak[6] = 0;
 peak[7] = 0;
}
//szuka maksymalnego wierzchołka w danym zakresie częstotliwości
void checkPeak(int band, int size) {
 int dmax = 3;
 if (size > dmax) size = dmax;
 if (size > peak[band]) { peak[band] = size; }
}
//zamienia wierzchołki zakresów częstotliwości na tablicę zer i jedynek
//dopasowaną do wyświetlacza
void translate() {
 for (int i = 0; i < 8; i++) {
 displayTab[i][0] = false;
 displayTab[i][1] = false;
 displayTab[i][2] = false;
 switch ((int)(peak[i] * 3 / 50)) {
 case 3: displayTab[i][2] = true;
 case 2: displayTab[i][1] = true;
 case 1: displayTab[i][0] = true;
 case 0:
 break;
 default:
 displayTab[i][0] = false;
 displayTab[i][1] = true;
 displayTab[i][2] = false;
 break;
 }
 }

//urządzenia wyjściowe nie są idealnie poukładane, więc trzeba wykonać kolejną
//transformację
 translatedDisplayTab[13] = displayTab[0][0];
 translatedDisplayTab[24] = displayTab[0][1];
 translatedDisplayTab[23] = displayTab[0][2];
 translatedDisplayTab[17] = displayTab[1][0];
 translatedDisplayTab[19] = displayTab[1][1];
 translatedDisplayTab[22] = displayTab[1][2];
 translatedDisplayTab[18] = displayTab[2][0];
 translatedDisplayTab[20] = displayTab[2][1];
 translatedDisplayTab[21] = displayTab[2][2];
 translatedDisplayTab[7] = displayTab[3][0];
 translatedDisplayTab[1] = displayTab[3][1];
 translatedDisplayTab[11] = displayTab[3][2];
 translatedDisplayTab[6] = displayTab[4][0];
 translatedDisplayTab[2] = displayTab[4][1];
 translatedDisplayTab[12] = displayTab[4][2];
 translatedDisplayTab[5] = displayTab[5][0];
 translatedDisplayTab[3] = displayTab[5][1];
 translatedDisplayTab[16] = displayTab[5][2];
 translatedDisplayTab[9] = displayTab[6][0];
 translatedDisplayTab[4] = displayTab[6][1];
 translatedDisplayTab[15] = displayTab[6][2];
 translatedDisplayTab[10] = displayTab[7][0];
 translatedDisplayTab[8] = displayTab[7][1];
 translatedDisplayTab[14] = displayTab[7][2];
}
// wysyła wynik do urządzeń wyjściowych
void displayTable() {
 digitalWrite(DISP_pin, HIGH); //wyłącz output
 for (int i = 0; i < 24; i++) {
 if (translatedDisplayTab[24 - i]) {
 digitalWrite(SERIN_pin, HIGH); // ‘1’ - SERIN = HIGH
 }
 else {
 digitalWrite(SERIN_pin, LOW); // ‘0’ - SERIN = LOW
 }
 digitalWrite(SRCK_pin, HIGH); // rejestr zapisuje SERIN
 digitalWrite(SRCK_pin, LOW); //
 }
 digitalWrite(RCK_pin, HIGH); // zapisane z SERIN wyślij na rejestr
 digitalWrite(RCK_pin, LOW); //
 digitalWrite(DISP_pin, LOW); // włącz output
}
