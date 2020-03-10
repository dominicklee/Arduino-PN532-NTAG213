// Writes lines of text to a Mifare Ultralight (or NTAG213) tag
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

void writeTag(String textLines[10], int numLines);
int roundUp(int numToRound, int multiple);

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

    String lines[10];
    lines[0] = "Text 1";
    lines[1] = "Text 2";
    lines[2] = "Text 3";
    lines[3] = "Text 4";
    writeTag(lines, 4);

    // wait until the tag is removed
    while (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
      Serial.println("please remove tag");
      delay(1000);
    }
}

/* Purpose: The writeTag function writes lines of text to an NFC tag
 * Usage: First create a String array and load your text lines. Specify number of lines.
 *        This function should ony be called after the tag is placed on the reader.
 * Concept: This function will calculate the payload lengths, byte lengths, headers, footers, etc.
 *          Then the function will store the "formatted sequence" into payload
 *          Then the payload will be split into bytes of 4, which the buffer will write on the tag
 *          Assuming we are using NTAG213, we will have 36 pages, 4 bytes each
 * Global Dependencies: byte payload[300], uint8_t buf[4]
 * Function Dependencies: int roundUp(int numToRound, int multiple)
 * NFC Text Payload format: 
 *    Index 4 always gets: 01 03 A0 0C
 *    Index 5 always gets: 34 03 [totl payload length] 91
 *    Line 1 header: 01 [byte length] 54 02   where bytes would be "enText 1 "
 *    Line 1 text: en + actual message
 *    To append more text:  11 01 [byte length] 54 02
 *    Line 2 text: en + actual message
 *    Next to last line footer: 51 01 [byte length] 54 02
 *    Line 3 text: en + actual message
 *    Ending footer: FE
 */
void writeTag(String textLines[10], int numLines)
{
  int payloadInd = 0; //index for reference
  payload[0] = 0; //clear payload

  //This is for Index 4
  payload[payloadInd] = 0x01; payloadInd++; //add new byte to payload arr
  payload[payloadInd] = 0x03; payloadInd++; //add new byte to payload arr
  payload[payloadInd] = 0xA0; payloadInd++; //add new byte to payload arr
  payload[payloadInd] = 0x0C; payloadInd++; //add new byte to payload arr
  
  //now we need to prepare the whole payload in advance since we need to set payload length
  //totl payload length = for each line (lineText[i].length() + 7)
  //we will also need individual byte length for each line, for use later
  int totlPayloadLen = 0; //we don't know the payloadLen until we finish loading the payload arr
  int lineByteLen[10];
  for (int i = 0; i <= numLines; i++) { //iterate each line of text
    lineByteLen[i] = textLines[i].length() + 3; //set byte length for this line
  }

  //This is for Index 5
  payload[payloadInd] = 0x34; payloadInd++; //add new byte to payload arr
  payload[payloadInd] = 0x03; payloadInd++; //add new byte to payload arr
  //This is a placeholder. We don't know the payloadLen yet until we finish loading the payload arr
  payload[payloadInd] = 0x00; payloadInd++; //add new byte to payload arr (Ind = 6)
  if (numLines > 1) { //this should only run if we have 2+ lines
    payload[payloadInd] = 0x91; payloadInd++; //add new byte to payload arr
  }
  else {  //only one line
    payload[payloadInd] = 0xD1; payloadInd++; //add new byte to payload arr
  }

  int bufferInd = 0; //start buffer index

  for (int i = 0; i < numLines; i++) { //iterate each line of text
    
    //This is the header for each line of text
    payload[payloadInd] = 0x01; payloadInd++; //add new byte to payload arr
    payload[payloadInd] = lineByteLen[i]; payloadInd++; //add new byte to payload arr
    payload[payloadInd] = 0x54; payloadInd++; //add new byte to payload arr
    payload[payloadInd] = 0x02; payloadInd++; //add new byte to payload arr
    payload[payloadInd] = 0x65; payloadInd++; //add new byte to payload arr
    payload[payloadInd] = 0x6E; payloadInd++; //add new byte to payload arr

    for (int j = 0; j < textLines[i].length(); j++) {  //iterate each character of string
      payload[payloadInd] = textLines[i].charAt(j); payloadInd++; //add new byte to payload arr
    }
    //insert footer after byte readout

    if (numLines > 1) { //this should only run if we have 2+ lines
      if (i == (numLines - 1)) {  //declare line-end if we are finished
        payload[payloadInd] = 0xFE; payloadInd++; //add byte FE to end of payload arr
      }
      else if (i == (numLines - 2)) {  //declare next to last
        payload[payloadInd] = 0x51; payloadInd++; //add byte 51 (Q) to payload arr
      }
      else {
        payload[payloadInd] = 0x11; payloadInd++; //add byte 11 (p) to payload arr
      }
    }
    else {
      payload[payloadInd] = 0xFE; payloadInd++; //add byte FE to end of payload arr
    }
    
  }

  //Now we know the payload index. We can calculate by subtracting the 4 bytes from page 4 and 5
  totlPayloadLen = payloadInd - 8;
  payload[6] = totlPayloadLen; //rewrite the total payload size (Ind = 6)

  //now we print it all back out and write to tag
  int bufInd = 0;
  int page = 4; //we always start at page 4 as convention

  int payloadRounded = roundUp(payloadInd, 4);  //round up the payload bytes to nearest multiple of 4
  
  for (int i = 0; i <= payloadRounded; i++) {
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

//This function rounds up a number to the nearest multiple
int roundUp(int numToRound, int multiple)
{
    if (multiple == 0)
        return numToRound;

    int remainder = numToRound % multiple;
    if (remainder == 0)
        return numToRound;

    return numToRound + multiple - remainder;
}

