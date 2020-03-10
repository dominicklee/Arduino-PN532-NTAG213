// Formats and clears a Mifare Ultralight (or NTAG213) tag
// Example written by Dominick Lee (dominicklee.com)

  #include <SoftwareSerial.h>
  #include <PN532_SWHSU.h>
  #include <PN532.h>
  #include <NfcAdapter.h>

  SoftwareSerial scanner(D2, D1); // RX | TX  
  PN532_SWHSU pn532hsu(scanner);
  PN532 nfc = PN532(pn532hsu);

uint8_t buf[4];
uint8_t uid[7]; 
uint8_t uidLength;
byte payload[300];  //to store raw content

void eraseTag(void);

void setup(void) {
    Serial.begin(115200);
    Serial.println("NTAG21x R/W");

    nfc.begin();
    nfc.SAMConfig();
}

void loop(void) {
    // wait until a tag is present
    while (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
      Serial.println("waiting for a tag");
      delay(1000);
    }

    nfc.mifareultralight_ReadPage(3, buf);
    int capacity = buf[2] * 8;
    Serial.print(F("Tag capacity "));
    Serial.print(capacity);
    Serial.println(F(" bytes"));

    eraseTag(); //this will delete everything on the tag and make it blank

    // wait until the tag is removed
    while (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
      Serial.println("please remove tag");
      delay(1000);
    }
}

/* Purpose: The eraseTag function formats an NFC tag to blank
 * Concept: This function will assume we are using NTAG213. We will clear 32 pages, starting at index 4.
 *          The format will reset the Index 4 and 5 to a blank defined header.
 *          Then the remaining 30 pages wil be filled with 00.
 * Global Dependencies: byte payload[300], uint8_t buf[4]
 * NFC Text Payload format: 
 *    Index 4 always gets: 01 03 A0 0C
 *    Index 5 always gets: 34 03 00 FE
 *    Remaining 30 pages will get 00
 *    30 pages * 4 bytes = 120 bytes will be set to 00
 */
void eraseTag()
{
  int payloadInd = 0; //index for reference
  payload[0] = 0; //clear payload

  //This is for Index 4
  payload[payloadInd] = 0x01; payloadInd++; //add new byte to payload arr
  payload[payloadInd] = 0x03; payloadInd++; //add new byte to payload arr
  payload[payloadInd] = 0xA0; payloadInd++; //add new byte to payload arr
  payload[payloadInd] = 0x0C; payloadInd++; //add new byte to payload arr

  //This is for Index 5
  payload[payloadInd] = 0x34; payloadInd++; //add new byte to payload arr
  payload[payloadInd] = 0x03; payloadInd++; //add new byte to payload arr
  payload[payloadInd] = 0x00; payloadInd++; //add new byte to payload arr (Ind = 6)
  payload[payloadInd] = 0xFE; payloadInd++; //add new byte to payload arr

  int bufferInd = 0; //start buffer index

  for (int i = 0; i < 120; i++) { //do this 120 times
    payload[payloadInd] = 0x00; payloadInd++; //add new byte to payload arr
  }

  //now we print it all back out and write to tag
  int bufInd = 0;
  int page = 4; //we always start at page 4 as convention

  for (int i = 0; i <= payloadInd; i++) {
    buf[bufInd] = payload[i];
    bufInd ++;
    if ((i + 1) % 4 == 0) {   //count by multiples of 4
      nfc.PrintHexChar(buf, 4);
      nfc.mifareultralight_WritePage (page, buf);  //send data
      bufInd = 0;
      page ++;  //increment page
    }
  }
  
}


