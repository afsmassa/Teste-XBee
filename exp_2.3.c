/*IMPLEMENTACAO CODIGO USANDO BUFFER PROPRIO
   Estrategia:
   Buffer com tamanho maximo de 28 bytes
   1-Lota-se o buffer deslocando-se os bytes pra direita
   2-Encontrado o START_BYTE le o "length" e o frameType
   3-Se frameType = 0x92, continua. Se não, ignora e continua até achar
   outr START_BYTE
   4-Calcula-se o checkSum e compara
   5-Se checkSum correto, apresenta resultado
*/

//Definição MACROS PRIMARIAS
#define LENGTH 2
#define NB_BYTE_SOURCE_ADD 8
#define NB_BYTE_NETWORK_ADD 2
#define NB_BYTE_DATA 2
#define START_BYTE 0x7E
#define BUFFER_SIZE 28
#define AMOSTRAGEM 1023
#define TENSAO_MAX 1.2
#define NB_ANALOG_IN 4

//considerando frame type 0x92
#define MAX_SIZE_FRAME 29 // 4 Analog In ativas MODIFICADO
#define MIN_SIZE_FRAME 20 // 0 Analog In ativas
#define FRAME_IO_SAMPLE_RX_INDICATOR 0x92

//Definição MACROS SECUNDARIAS (Nao mexa)
#define ULT_POS_BUF_SERIAL (MAX_SIZE_FRAME - 1)

//Variaveis de leitura
union length2B {
  unsigned char bt [2]; //2 bytes
  unsigned short int data; // 2 bytes
};

union length4B {
  unsigned char bt [4]; //4 bytes
  unsigned long int data; // 4 bytes
};

union length2B comprimento;
byte startByte;
byte frameType;
byte checkSum;
union length4B sourceAddressHigh; //limitação do Arduino UNO
union length4B sourceAddressLow;
union length2B networkAdd;
byte receiveOpt;
byte nbSamples;
union length2B analogSample[NB_ANALOG_IN];
byte analogChanMask;
union length2B digChanMask;
byte bufferSerial[MAX_SIZE_FRAME]; //tipo unsigned char = byte

//Variaveis gerais
int verifCheckSum;
int i, j, k;
float volts[NB_ANALOG_IN];
char bufferMessage[50];
int nbAnalogIn;
int serialAvailable;
unsigned long qtdPacotes = 0;
unsigned long qtdAcertos = 0;
float taxaAcerto = 0;
int posDado;
unsigned long qtdFrameType = 0;
unsigned long qtdFrameTotal = 0;
float taxaFrameType = 0;


void setup() {

  Serial.begin(38400);

  //inicializa o bufferSerial com zeros
  for (i = 0; i < MAX_SIZE_FRAME; i++) {
    bufferSerial[i] = 0;
  }

  //inicializa o bufferMessage com zeros
  for (i = 0; i < 50; i++) {
    bufferMessage[i] = 0;
  }
}

void loop() {
  // put your main code here, to run repeatedly:

  serialAvailable = Serial.available();
  if (serialAvailable > 0) {



    //Desloca todos os bytes de uma unidade para a direita
    for (i = MAX_SIZE_FRAME - 1; i > 0; i--) {
      bufferSerial[i] = bufferSerial[i - 1];
    }
    bufferSerial[0] = Serial.read();
 /*   for (i = 0; i < MAX_SIZE_FRAME ; i++) {

      sprintf(bufferMessage, "%02X ", bufferSerial[i]); //sprintf nao suporta float
      Serial.print(bufferMessage);

    }
    Serial.println("");
*/
    if (bufferSerial[ULT_POS_BUF_SERIAL] == START_BYTE) {

      qtdFrameTotal++;


      //--Estrutura fixa do frame--//
      startByte = bufferSerial[ULT_POS_BUF_SERIAL];

      comprimento.bt [1] = bufferSerial[ULT_POS_BUF_SERIAL - 1];
      comprimento.bt [0] = bufferSerial[ULT_POS_BUF_SERIAL - 2];

      frameType = bufferSerial[ULT_POS_BUF_SERIAL - 3];
      //---------------------------//



      qtdPacotes++;

      //PARTE FIXA DO FRAME TYPE 0x92
      sourceAddressHigh.bt [3] = bufferSerial[ULT_POS_BUF_SERIAL - 4];
      sourceAddressHigh.bt [2] = bufferSerial[ULT_POS_BUF_SERIAL - 5];
      sourceAddressHigh.bt [1] = bufferSerial[ULT_POS_BUF_SERIAL - 6];
      sourceAddressHigh.bt [0] = bufferSerial[ULT_POS_BUF_SERIAL - 7];

      sourceAddressLow.bt [3] = bufferSerial[ULT_POS_BUF_SERIAL - 8];
      sourceAddressLow.bt [2] = bufferSerial[ULT_POS_BUF_SERIAL - 9];
      sourceAddressLow.bt [1] = bufferSerial[ULT_POS_BUF_SERIAL - 10];
      sourceAddressLow.bt [0] = bufferSerial[ULT_POS_BUF_SERIAL - 11];

      networkAdd.bt [1] = bufferSerial[ULT_POS_BUF_SERIAL - 12];
      networkAdd.bt [0] = bufferSerial[ULT_POS_BUF_SERIAL - 13];

      receiveOpt = bufferSerial[ULT_POS_BUF_SERIAL - 14];
      nbSamples = bufferSerial[ULT_POS_BUF_SERIAL - 15];

      digChanMask.bt[1] = bufferSerial[ULT_POS_BUF_SERIAL - 16];
      digChanMask.bt[0] = bufferSerial[ULT_POS_BUF_SERIAL - 17];

      analogChanMask = bufferSerial[ULT_POS_BUF_SERIAL - 18];
      //--------------

      //NAO CONSIDERAMOS DADOS DIGITAIS POR ENQUANTO

      //Calcular a quantidade de entradas analógicas ativas
      nbAnalogIn = 0;
      if ((analogChanMask & B0001) == B0001) {
        nbAnalogIn++;
      }
      if ((analogChanMask & B0010) == B0010) {
        nbAnalogIn++;
      }
      if ((analogChanMask & B0100) == B0100) {
        nbAnalogIn++;
      }
      if ((analogChanMask & B1000) == B1000) {
        nbAnalogIn++;
      }

      posDado = ULT_POS_BUF_SERIAL - 19;//posicao do MSB do primeiro dado

      //registra os dados
      for (j = 0; j < nbAnalogIn; j++ ) {
        for (i = NB_BYTE_DATA - 1; i >= 0 ; i--) {
          analogSample[j].bt [i] = bufferSerial[posDado];
          posDado--;
        }
      }

      checkSum = bufferSerial[posDado];

      //---SOMANDO BYTES DO FRAME PARA VERIFICAÇÂO, FrameType até AnalogSample---//

      verifCheckSum = 0;
      verifCheckSum += frameType;

      for (i = NB_BYTE_SOURCE_ADD / 2 - 1; i >= 0 ; i--) {
        verifCheckSum += sourceAddressHigh.bt [i];
      }

      for (i = NB_BYTE_SOURCE_ADD / 2 - 1; i >= 0 ; i--) {
        verifCheckSum += sourceAddressLow.bt [i];
      }

      for (i = NB_BYTE_NETWORK_ADD - 1; i >= 0 ; i--) {
        verifCheckSum += networkAdd.bt [i];
      }

      verifCheckSum += receiveOpt;

      verifCheckSum += nbSamples;

      for (i = 1; i >= 0 ; i--) {
        verifCheckSum += digChanMask.bt[i];
      }

      verifCheckSum += analogChanMask;

      for (j = 0; j < nbAnalogIn; j++ ) {
        for (i = NB_BYTE_DATA - 1; i >= 0 ; i--) {
          verifCheckSum += analogSample[j].bt [i];
        }
      }
      verifCheckSum = verifCheckSum & B11111111;

      verifCheckSum = 0xFF - verifCheckSum;
      //-----------------------------------------------------------//

      //Print taxa de acertos do FrameType
      if (frameType==FRAME_IO_SAMPLE_RX_INDICATOR) {
        qtdFrameType++;
 //       Serial.println("==========");
        taxaFrameType = (100 * qtdFrameType) / qtdFrameTotal;
 //       Serial.print("Taxa de acertos Frame Type 0x92(%): ");
        Serial.println(taxaFrameType);
//        Serial.println("==========");
      }
      //---VERIFICACAO DO CHECKSUM---//
      //Terceiro if, se frametype e checksum baterem, apresenta os valores
      if ((verifCheckSum == checkSum)&&(frameType==FRAME_IO_SAMPLE_RX_INDICATOR) ) {

/*        Serial.println("--");
        Serial.print("Serial Avaialble (dec): ");
        Serial.println(serialAvailable, DEC);
        Serial.println("-");
        Serial.print("Start byte: ");
        Serial.println(startByte, HEX); //Start byte = 0x7E

        sprintf(bufferMessage, "Length: %04X", comprimento.data);
        Serial.println(bufferMessage);

        Serial.print("Frame type: ");
        Serial.println(frameType, HEX);

        Serial.print("64-bit source address: ");
        sprintf(bufferMessage, "%08lX ", sourceAddressHigh.data); //%08lx = 8 digitos(caracteres) formato long
        Serial.print(bufferMessage);
        sprintf(bufferMessage, "%08lX", sourceAddressLow.data);
        Serial.print(bufferMessage);
*/
        //DISCRIMINA O NOME DO NÓ END
        switch (sourceAddressLow.data) {

          case 0x4089CB96:
            Serial.println(" - END 1");
            break;
          case 0x40683F8D:
            Serial.println(" - END 2");
            break;
          case 0x4089CB93:
            Serial.println(" - END 3");
            break;

          default:
            Serial.println(" - Nao reconhecido");
            break;
        }
/*
        Serial.print("16-bit Source Network Address: ");
        sprintf(bufferMessage, "%04X", networkAdd.data);
        Serial.println(bufferMessage);

        Serial.print("Receive options: ");
        Serial.println(receiveOpt, HEX);

        Serial.print("Number of samples: ");
        Serial.println(nbSamples, HEX);

        Serial.print("Digital channel mask: ");
        Serial.println(digChanMask.data, HEX);

        Serial.print("Analog channel mask: ");
        Serial.print(analogChanMask, BIN);
        Serial.print(" - # AnalogIn ativas: ");
        Serial.println(nbAnalogIn);

        for (j = 0; j < nbAnalogIn; j++ ) {
          sprintf(bufferMessage, "Analog Sample %i: %X (%i)", j, analogSample[j].data, analogSample[j].data);
          Serial.println(bufferMessage);
        }

        Serial.print("CheckSum : ");
        Serial.println(checkSum, HEX);

        Serial.print("Calculo CheckSum : ");
        Serial.println(verifCheckSum, HEX);
*/
    //    Serial.println("### CHECKSUM CORRETO!!! ###");
                Serial.println("CC");

/*        qtdAcertos++;

        //CALCULO e PRINT DA TENSAO em V

        for (j = 0; j < nbAnalogIn; j++) {
          volts[j] = (analogSample[j].data * TENSAO_MAX) / AMOSTRAGEM;
          sprintf(bufferMessage, "Tensao %i (V): ", j); //sprintf nao suporta float
          Serial.print(bufferMessage);
          Serial.println(volts[j]);
        }

        //Print taxa de acertos do checkSum
        taxaAcerto = (100 * qtdAcertos) / qtdPacotes;
        Serial.print("Taxa de acertos Check Sum(%): ");
        Serial.println(taxaAcerto);
*/
      }//checksum batendo


/*      else
      {

        Serial.print("Frame Type Diferente!: ");
        Serial.println(frameType, HEX);

      }
   */
    }//byte = StartByte

  }//serial available
}//loop





