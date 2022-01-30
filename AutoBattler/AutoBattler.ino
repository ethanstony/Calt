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

enum Modes{PREP, CALC, PLAY, FINISH};
byte mode = PREP;

byte isOrigin = 0;//is origin(also there is a red origin but not map on grid
byte side = 0;//1 = blue 2 = red 0 none

enum pieces{empty, tank, fighter, ranger};//classes
byte piece = empty;//player on current tile
byte health = 0;

bool setCalc = false;
byte pathLang = 14;
byte desiredPath = 6;

Timer PrepTimer;
boolean timerPrimed = false;

enum SIGNAL {EMPTY, SETCOORD, STARTGAME, REQUESTCOORD, RESPONSECOORD, REQUESTMOVE, RESPONSEMOVE, MOVERED, MOVEBLUE};

//player party --------------
enum PARTY {NONE, Red, Blue};
byte player = NONE;

enum STATE {SETUP0, SETUP, INPROGRESS, GAMESTART};
byte state = SETUP0;
//position of this blink on the board
byte x;
byte y;
byte z;

Timer timer;
//Main Code Body-----------------------------------------------------------------------------------
void setup() {
  mode = PREP;
  isOrigin = 0;//1 is origin 0 is not. there also exist red origin (shouldn't affect grid
  side = 0; // 1 is blue, 2 is red
  //setColor(WHITE);
}

void loop() {
  //state control
  switch(state) {
    case SETUP0:
      firstloop();     
      break;
    case SETUP:
      //while(true)
      //SetUpPlayerParty(); //change 
      //Once double clicked, we broadcast set-up-map to the board
      if(isOrigin == 1 && side == 1){
        SetUpMap();
      }
      SetUpLoop();
      break;
    case INPROGRESS:
      PrepareLoop();
      break;
    case GAMESTART:
      GameLoop();
      break;
   }

   paintOn();
}

void paintOn(){
  switch(state) {
    case SETUP0:
      break;
     default:
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
          break;
  }
}

//starting code
void inertLoop() {
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getSignalState(getLastValueReceivedOnFace(f)) == GO) {//a neighbor saying GO!
        if(getGameMode(getLastValueReceivedOnFace(f)) == 2){
          mode = CALC;
        }else{
          mode = PREP;
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
      
      PrepTimer.set(1000);
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
  state = SETUP;
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

void firstloop() {
       switch (signalState){
        case INERT:
          switch(mode){
            case PREP:
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
      if(mode == PREP){
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
}


//Map Coordinates SetUp-----------------------------------------------------------------------------------


void SetUpLoop() {
  FOREACH_FACE(f) {
    if(isDatagramReadyOnFace(f)) {
      const byte* data = getDatagramOnFace(f);
     
      switch(data[0]) {
        case SETCOORD:
          if(state != INPROGRESS) {
      setColor(GREEN);
            state = INPROGRESS;
  timer.set(1000);
            x = data[1];
            y = data[2];
            z = data[3];
            SendCoord();
          }
          break;
      }
      markDatagramReadOnFace(f);
    }
  }
}

void SetUpMap() {
  x = 10, y = 10, z = 10;
  //setColor(GREEN);
  
  SendCoord();
  state = INPROGRESS;
  timer.set(1000);
}
void SendCoord() {
  //face 0 right down
  byte data[4] = {SETCOORD, x+1, y, z-1};
  sendDatagramOnFace(data, 4, 0);
  //face 1 left down
  data[1] = x;
  data[2] = y+1;
  data[3] = z-1;
  sendDatagramOnFace(data, 4, 1);
  //face 2 left
  data[1] = x-1;
  data[2] = y+1;
  data[3] = z;
  sendDatagramOnFace(data, 4, 2);
  //face 3 left up
  data[1] = x-1;
  data[2] = y;
  data[3] = z+1;
  sendDatagramOnFace(data, 4, 3);
  //face 4 right up
  data[1] = x;
  data[2] = y-1;
  data[3] = z+1;
  sendDatagramOnFace(data, 4, 4);
  //face 5 right
  data[1] = x+1;
  data[2] = y-1;
  data[3] = z;
  sendDatagramOnFace(data, 4, 5);
}

//Preparation Time Before Pawns Move(Allow some time for setting up coordinates)------------
void PrepareLoop() {
  if(timer.isExpired()) {
    state = GAMESTART;
    timer.set(1000);
    //SetPlayerColor();
  }
}



//Game Start. Pawns move----------------------------------------------------------
enum PawnState {DECIDE, MOVE, RESET};
byte pawnState = DECIDE;
//destination this pawn is moving towards
byte des[] = {10, 10, 10};
//bestMove indicates the next neighbor to move to;
byte bestMove = 6;
byte shortest = 20;
//once a neighboring pawn is allowed to move in,  this tile is locked
bool occupied = false;

void GameLoop() {
  if(timer.isExpired()) { //1s for each move
    PawnAutoDecide();
  }
 
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      if(didValueOnFaceChange(f)) {

        switch(getLastValueReceivedOnFace(f)) {
          case REQUESTCOORD:
            if(player == NONE) //Only respond if this tile is empty
              NavResponse(f);
            break;
          case REQUESTMOVE:
            if(!occupied && player == NONE) {
              SendMoveResponse(f);
              occupied = true;
            }
            break;
          case RESPONSEMOVE:
            if(player != NONE)
              SendMove(f);
            break;
          case MOVERED:
            ResetTile(Red);
            break;
          case MOVEBLUE:
            ResetTile(Blue);
            break;
        }
      }
    }
   
    if(isDatagramReadyOnFace(f)) {
      const byte* data = getDatagramOnFace(f);
      //Handles coordinate from the neighbor
      if(data[0] == RESPONSECOORD) {
        byte dis = Distance(data[1], data[2], data[3]);
        if(shortest > dis) {
          shortest = dis;
          bestMove = f;
        }
      }
      markDatagramReadOnFace(f);
    }
  }
}
//Each pawn move is split into decision of which path to take and communicate with the neighbor to move.
void PawnAutoDecide() {
  if(player == NONE) return;
 
  if(pawnState == DECIDE) {
    timer.set(500);
    pawnState = MOVE;
    NavRequest();
    return;
  }
 
  if(pawnState == MOVE) {
    SendMoveRequest();
    timer.set(500);
  }
}

void setDestiny(byte a, byte b, byte c){
  des[0] = a;
  des[1] = b;
  des[2] = c;
}

//request for the coord of all neighbors
void NavRequest() {
  setValueSentOnAllFaces(REQUESTCOORD);
}
//respond with coord
void NavResponse(byte face) {
  byte data[4] = {RESPONSECOORD, x, y, z};
  sendDatagramOnFace(data, 4, face);
}
//request to move to the optimal neighbor
void SendMoveRequest() {
  if(bestMove == 6) return;
  setValueSentOnFace(REQUESTMOVE, bestMove);
}
//respond only if move is possible
void SendMoveResponse(byte face) {
  setValueSentOnFace(RESPONSEMOVE, face);
}
//send in info about this pawn.
void SendMove(byte face) {
  if(player == Red)
    setValueSentOnFace(MOVERED, face);
 
  if(player == Blue)
    setValueSentOnFace(MOVEBLUE, face);
 
  ResetTile(NONE);
}

//Helper-----------------------------------------------------------------------------------
byte DiffBetween(byte a, byte b) {
  if(a > b) {
    return a - b;
  } else {
    return b - a;
  }
}

byte Distance(byte x, byte y, byte z) {
  return DiffBetween(x,des[0]) + DiffBetween(y,des[1]) + DiffBetween(z,des[2]);
}

void SetPlayerColor() {
    if(player == Red)
      setColor(RED);
 
    if(player == Blue)
      setColor(BLUE);
}
//reset by party
void ResetTile(byte party) {
  switch(party) {
    case NONE:
      player = NONE;
      setColor(GREEN);
      bestMove = 6;
      shortest = 20;
      pawnState = DECIDE;
      break;
    case Red:
      player = Red;
      setColor(RED);
      bestMove = 6;
      shortest = 20;
      occupied = false;
      break;
    case Blue:
      player = Blue;
      setColor(BLUE);
      bestMove = 6;
      shortest = 20;
      occupied = false;
      break;
  }
}
