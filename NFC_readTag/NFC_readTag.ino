// Reads a Mifare Ultralight (or NTAG213) tag
// Example written by Dominick Lee (dominicklee.com)

  #include <SoftwareSerial.h>
  #include <PN532_SWHSU.h>
  #include <PN532.h>
  #include <NfcAdapter.h>

  SoftwareSerial scanner(D2, D1); // RX | TX  
  PN532_SWHSU pn532hsu(scanner);
  PN532 nfc = PN532(pn532hsu);

uint8_t password[4] =  {0x12, 0x34, 0x56, 0x78};
uint8_t buf[4];
uint8_t uid[7]; 
uint8_t uidLength;
byte payload[300];  //to store raw content

void readTag(bool verbose = true);

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

    // if NTAG21x enables r/w protection, uncomment the following line 
    // nfc.ntag21x_auth(password);

    readTag();  //reads the tag (set false to hide verbose)

    // wait until the tag is removed
    while (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
      Serial.println("please remove tag");
      delay(1000);
    }
}

//This function reads a tag page-by-page (to buffer), stores its text (to payload),
// and then it parses payload (to lines String array), and cleans up the system characters
void readTag(bool verbose)
{
    nfc.mifareultralight_ReadPage(3, buf);
    int capacity = buf[2] * 8;
    if (verbose) {
      Serial.print(F("Tag capacity "));
      Serial.print(capacity);
      Serial.println(F(" bytes"));
    }

    for (int i=4; i<capacity/4; i++) {
        nfc.mifareultralight_ReadPage(i, buf);
        if (verbose) {
          nfc.PrintHexChar(buf, 4);
        }
        strcat((char *)payload, (char *)buf); //copy to payload
    }
    
    //Serial.println((char *)payload);

    String lines[10];
    bool encodeText = false;
    int bookmarkIndx = 0;
    int numLines = 0;  //max timeout after 10 lines (but we can exit this using break)
    int totalLines = 0; //will be set after done parsing
    do {
      lines[numLines] = ""; //start it off
      for (int i = bookmarkIndx; i < 300; i++) { //iterating through whole string
        
        if ((int)payload[i] == 02) {  //start of text
          encodeText = true;
        }
        
        if ((int)payload[i] == 01) {  //end of text
          encodeText = false;
          bookmarkIndx = i + 1; //set the resume index to the next char
          break;
        }

        if (encodeText) {
          char c = (char)payload[i];
          //Serial.println(c);
          lines[numLines] += c;
        }
        
        if ((int)payload[i] == 254) {  //end of payload
          totalLines = numLines;  //will be used later
          numLines = 10;  //break the outer loop too
          break;
        }
        
      }
      if (lines[numLines].length() > 0) {
        numLines ++; //count down
      }
      //Serial.println("next line");
    } while (numLines < 10);

    //clean up system chars
    for (int i = 0; i <= totalLines; i++) {
      lines[i].remove(0, 3);
      lines[i].remove(lines[i].length() - 1);
      if (verbose) {
        Serial.println(lines[i]);
      }
    }
    
    payload[0] = 0; //clear payload
}

