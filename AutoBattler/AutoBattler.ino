enum SIGNAL {EMPTY, SETCOORD, STARTGAME, REQUESTCOORD, RESPONSECOORD, REQUESTMOVE, RESPONSEMOVE, MOVERED, MOVEBLUE}

//player party --------------
enum PARTY {NONE, Red, Blue}
byte player = NONE;

enum STATE {SETUP, INPROGRESS, GAMESTART}
byte state = SETUP;
//position of this blink on the board
byte x;
byte y;
byte z;

Timer timer;
//Main Code Body-----------------------------------------------------------------------------------
void setup() {
  setColor(WHITE);
}

void loop() {
  //state control
  switch(state) {
    case SETUP:
      SetUpPlayerParty();
      //Once double clicked, we broadcast set-up-map to the board
      if(buttonDoubleClicked())
        SetUpMap();
     
      SetUpLoop();
      break;
    case INPROGRESS:
      PrepareLoop();
      break;
    case GAMESTART:
      GameLoop();
      break;
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

void SetUpPlayerParty() {
  if(buttonSingleClicked()) {
    player = (player+1) % 3;
    SetPlayerColor();
  }
}
//This is only called on the origin! Then messages will be broadcasted to the board. Waits 1s and switch to game state
void SetUpMap() {
  x = 10, y = 10, z = 10;
  setColor(GREEN);
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
    SetPlayerColor();
  }
}

//Game Start. Pawns move----------------------------------------------------------
enum PawnState {DECIDE, MOVE, RESET}
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

void SetDestiny(x, y, z) {
  des[0] = x;
  des[1] = y;
  des[2] = z;
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
