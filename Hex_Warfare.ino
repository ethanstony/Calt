#define BackgroundRed makeColorRGB(255, 175, 175)
#define BackgroundBlue makeColorRGB(175, 175, 255)
#define RedTank makeColorRGB(100, 0, 0)
#define BlueTank makeColorRGB(0, 0, 100)
#define RedFighter makeColorRGB(255, 100, 0)
#define BlueFighter makeColorRGB(100, 0, 255)
#define RedRanger makeColorRGB(255, 255, 0)
#define BlueRanger makeColorRGB(0, 255, 255)

enum signalStates {INERT, GO, RESOLVE};
byte signalState = INERT;

enum Modes{SETUP, CALC, PLAY, FINISH};
byte mode = SETUP;

byte isOrigin = 0;
byte side = 0;

enum pieces{empty, tank, fighter, ranger};
byte piece = empty;
byte health = 0;

bool setCalc = false;
//bool setPath = false;
byte pathLang = 14;
byte desiredPath = 6;

enum movement {MOVE, ATTACK, STAY, RESERVED};
Timer PrepTimer;
boolean timerPrimed = false;

void inertLoop() {
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getSignalState(getLastValueReceivedOnFace(f)) == GO) {//a neighbor saying GO!
        if(getGameMode(getLastValueReceivedOnFace(f)) == 2){
          mode = CALC;
        }else{
          mode = SETUP;
          isOrigin = 0;
          side = 0;
          piece = empty;
          health = 0;
        }     
        signalState = GO;      
      }
      if (getSignalState(getLastValueReceivedOnFace(f)) == INERT && getGameMode(getLastValueReceivedOnFace(f)) == 1){
        isOrigin = 1;
        side = 2;
      }
    }
  }

  if(buttonSingleClicked()){
          if(side == 0){
            side = 1;
          }else{
            switch (piece){
              case empty:
                piece = tank;
                health = 6;
                break;
              case tank:
                piece = fighter;
                health = 3;
                break;
              case fighter:
                piece = ranger;
                health = 2;
                break;
              case ranger:
                piece = empty;
                health = 0;
                if(isOrigin!=1){
                  if(side == 1){
                    side++;
                  }else{
                    side--;
                  }
                }
                break;
            }
          }
        }
}

void  CALCinertLoop(){
  if(isOrigin == 1){
    if(timerPrimed == false){
      
      PrepTimer.set(2000);
      timerPrimed = true;
    }else{
      if(PrepTimer.isExpired()){
        timerPrimed == false;
        mode = PLAY;
        signalState = GO;
      }
    }
    pathLang = 0;
  }else{
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        if (getSignalState(getLastValueReceivedOnFace(f)) == GO) {//a neighbor saying GO!
          if(getPathLang(getLastValueReceivedOnFace(f)) == 15){
            mode = PLAY;
            signalState = GO; 
          }
             
        }
        if(pathLang > (getPathLang(getLastValueReceivedOnFace(f))+1)){
          pathLang = getPathLang(getLastValueReceivedOnFace(f))+1;
          desiredPath = f;
        }
      }
     }
   }
}
void PLAYinertLoop(){
  
}


void goLoop() {
  signalState = RESOLVE;//I default to this at the start of the loop. Only if I see a problem does this not happen

  //look for neighbors who have not heard the GO news
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getSignalState(getLastValueReceivedOnFace(f)) == INERT) {//This neighbor doesn't know it's GO time. Stay in GO
        signalState = GO;
      }
    }
  }
}

void resolveLoop() {
  signalState = INERT;//I default to this at the start of the loop. Only if I see a problem does this not happen

  //look for neighbors who have not moved to RESOLVE
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getSignalState(getLastValueReceivedOnFace(f)) == GO) {//This neighbor isn't in RESOLVE. Stay in RESOLVE
        signalState = RESOLVE;
      }
    }
  }
}

void setBlueOrigin(){
        isOrigin = 1;
        side = 1;
        signalState = GO;
}



byte getGameMode(byte data) {
  return (data & 3);//returns bits E and F
}

byte getPathLang(byte data) {
  return (data & 15);//returns bits E and F
}

byte getSignalState(byte data) {
  return ((data >> 4) & 3);//returns bits C and D
}


void displaySide(){
   switch (signalState) {
    case INERT:
        if(mode == CALC){
          setColor(RED);
          for(int i = 0; i < 6; i++){
            if(i<pathLang){
              setColorOnFace(GREEN, i);
            }
            
          }
          
          
        }else{
          if(side == 0){
            setColor(WHITE);
          }else{
            if(side == 1){
              if(isOrigin == 1){
                setColor(BLUE);
              }else{
                setColor(BackgroundBlue);
              }
              
            }else{
              if(isOrigin == 1){
                setColor(RED);
              }else{
                setColor(BackgroundRed);
              }
            }
            switch(piece){
              case empty:
                break;
              case tank:
                for(int i = 0; i < health; i++){
                  if(side == 1){
                    setColorOnFace(BlueTank, i);
                  }else{
                    setColorOnFace(RedTank, i);
                  }
                  
                }
                break;
              case fighter:
                for(int i = 0; i < health; i++){
                  if(side == 1){
                    setColorOnFace(BlueFighter, i);
                  }else{
                    setColorOnFace(RedFighter, i);
                  }
                  
                }
                break;
              case ranger:
                for(int i = 0; i < health; i++){
                  if(side == 1){
                    setColorOnFace(BlueRanger, i);
                  }else{
                    setColorOnFace(RedRanger, i);
                  }
                  
                }
                break;
            }

          }
          
           
          }  
      
      break;
    case GO:
       setColor(GREEN);
      break;
    case RESOLVE:
      setColor(YELLOW);
      break;
  }
}

void setup() {
  // put your setup code here, to run once:
  mode = SETUP;
  isOrigin = 0;
  side = 0;
}

void loop() {
      switch (signalState){
        case INERT:
          switch(mode){
            case SETUP:
                if(buttonDoubleClicked()){
                  if(isOrigin==0||side != 1){
                    setBlueOrigin();
                  }else{
                  
                    mode = CALC;
                    signalState = GO;
                  }
              
                }
                inertLoop();
                break;
            case CALC:
              CALCinertLoop();
              break;
            case PLAY:
              break;
            case FINISH:
               break;
          }
          break;
        case GO:
          goLoop();
          break;
        case RESOLVE:
          resolveLoop();
          break;
      }
              
      displaySide();
      byte sendData = (signalState << 4);
      if(mode == SETUP){
        if(isOrigin == 1&&side==1){
          setValueSentOnFace(sendData+1,0 );
           byte i = 1;
          bool makered = true;
          while(i<=5){
          
            setValueSentOnFace(sendData,i );
         
            i++;
          
          }
        }else{
          setValueSentOnAllFaces(sendData);
        }  
      }else if(mode == CALC){
        if (setCalc == false){
          setValueSentOnAllFaces(sendData+2);
          if(signalState == RESOLVE){
            setValueSentOnAllFaces(sendData+pathLang);
            setCalc = true;
            
          }
          
        }else{
          setValueSentOnAllFaces(sendData+pathLang);
        }
        
      }else if(mode == PLAY){
        setValueSentOnAllFaces(sendData+15);
        
      }
      

  
  // put your main code here, to run repeatedly:

}
