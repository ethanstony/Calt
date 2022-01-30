enum SIGNAL {EMPTY, SETCOORD, STARTGAME, REQUESTCOORD, RESPONSECOORD, REQUESTMOVE, RESPONSEMOVE, MOVEINFO, ATTACK,
SEARCH, RESPOND}

//player party --------------
enum PARTY {NONE, Red, Blue}
byte player = NONE;

enum STATE {SETUP, INPROGRESS, GAMESTART}
byte state = SETUP;

enum pieces {TANK, FIGHTER, RANGER, HEALER}
byte piece = TANK;
byte hp;
//position of this blink on the board
byte x;
byte y;
byte z;

Timer timer;
Timer effectTimer; //effect duration after hit

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
  SetPlayerColor();
}

//Map Coordinates SetUp-----------------------------------------------------------------------------------
void SetUpLoop() {
  FOREACH_FACE(f) {
    if(isDatagramReadyOnFace(f)) {
      const byte* data = getDatagramOnFace(f);
     
      switch(data[0]) {
        case SETCOORD:
          if(state != INPROGRESS) {
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
  if(buttonMultiClicked()) {
    player = (player+1) % 3;
  }
 
  if(buttonSingleClicked()) {
    piece = (piece+1) % 4;
  }
}
//This is only called on the origin! Then messages will be broadcasted to the board. Waits 1s and switch to game state
void SetUpMap() {
  x = 10, y = 10, z = 10;
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

byte rface;

void GameLoop() {
  if(timer.isExpired()) { //1s for each move
    PawnAutoDecide();
  }
  
  if(buttonSingleClicked()) {
    SendSearchSignal();
    rface = 6;
    timer.set(0);
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
      
      if(data[0] == MOVEINFO) {
        ResetTile(data[1],data[2]);
      }
     
      if(data[0] == ATTACK) {
        if(data[1] != RANGER) {
          DealDamage(data[3]);
        } else { //ranger's attack
          DealDamage(data[3]);
         
          byte panetration[4] = {ATTACK, TANK, data[2], data[3]};
          sendDatagramOnFace(panetration, 4, data[2]);
        }
      }
      z
      if(data[0] == SEARCH && data [4] != 0){
        if(player == NONE)
          setColor(ORANGE);
        transmitSearchSignal(data);
        if(timer.isExpired){
         timer.set(1000);
        rface = f;
        }
      }else if(data[0] == RESPOND && rface == 6){
          setColor(GREEN);
      }else if(data[0] == RESPOND){
        if(player == NONE)
        setColor(CYAN);
        transmitRespondSignal(data);
      }
      if(player != NONE){
      	sendRespondSignal()
      }
      markDatagramReadOnFace(f);
    }
  }
 
}

byte optimalMove(){
}

byte r = 3;

void SendSearchSignal() {
  byte data[5] = {SEARCH, x, y, z, r};
 
  FOREACH_FACE(f) {
    if (f !=rface)
    sendDatagramOnFace(data, 5, f);
  }
}

void transmitSearchSignal(byte *package){
  package[4]--;
  
  FOREACH_FACE(f) {
    if (f !=rface)
    sendDatagramOnFace(package, 5, f);
  }
}


void sendRespondSignal() {
  byte data[5] = {RESPOND, x, y, z, player};
 
  sendDatagramOnFace(data, 5, rface);
}

void transmitRespondSignal(byte *package) {
  sendDatagramOnFace(package, 5, rface);
}

//Each pawn move is split into decision of which path to take and communicate with the neighbor to move.
void PawnAutoDecide() {
  if(player == NONE) return;
 
  if(buttonSingleClicked()) //To be changed to check if enemy nearby
    PawnAttack(x, y-1, z+1);
 
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
//Pawn attacks position x,y,z
//NOTE: Damage can be modified here!
void PawnAttack(dx, dy, dz) {
  byte f = GetAttackFace(dx,dy,dz);
  if(f == 6) return; //invalid attack
  byte data[4] = {ATTACK, piece, f, 0};
  switch(piece) {
    case TANK:
      data[3] = 1; //assign damage
      sendDatagramOnFace(data, 4, f);
      break;
    case FIGHTER:
      data[3] = 1; //assign damage
      sendDatagramOnFace(data, 4, f);
      //propogate damage to the two near neighbors
      SendPropogateDmg(data, f);
      break;
    case RANGER:
      data[3] = 1; //assign damage
      sendDatagramOnFace(data, 4, f);
      break;
    case HEALER:
      data[3] = 1; //assign damage
      sendDatagramOnFace(data, 4, f);
      break;
  }
}

void SendPropogateDmg(byte data[], f) {
  if(f == 0) {
    sendDatagramOnFace(data, 4, 1);
    sendDatagramOnFace(data, 4, 5);
  } else if(f == 5) {
    sendDatagramOnFace(data, 4, 0);
    sendDatagramOnFace(data, 4, 4);
  } else {
    sendDatagramOnFace(data, 4, f+1);
    sendDatagramOnFace(data, 4, f-1);
  }
}

//deal damage to this pawn
void DealDamage() {
  effectTimer.set(200);
  if(player == NONE) return;
}

void SetDestiny(byte dx, byte dy, byte dz) {
  des[0] = dx;
  des[1] = dy;
  des[2] = dz;
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
  byte data[3] = {MOVEINFO, player, piece};
  sendDatagramOnFace(data, 3, face);
  ResetTile(NONE, TANK);
}

//Helper-----------------------------------------------------------------------------------
byte DiffBetween(byte a, byte b) {
  if(a > b) {
    return a - b;
  } else {
    return b - a;
  }
}

byte Distance(byte dx, byte dy, byte dz) {
  return DiffBetween(dx,des[0]) + DiffBetween(dy,des[1]) + DiffBetween(dz,des[2]);
}

void SetPlayerColor() {
  if(!effectTimer.isExpired()) {
    setColor(MAGENTA);
    return;
  }
 
  if(player == Red) {
    for(byte i = 0; i <= piece; i++) {
      setColorOnFace(YELLOW,i);
    }
    for(byte i = piece+1; i < 6; i++) {
      setColorOnFace(RED,i);
    }
  }
 
  if(player == Blue) {
    for(byte i = 0; i <= piece; i++) {
      setColorOnFace(YELLOW,i);
    }
    for(byte i = piece+1; i < 6; i++) {
      setColorOnFace(BLUE,i);
    }
  }
 
  if(player == NONE)
    setColor(GREEN);
}

byte GetAttackFace(dx, dy, dz) {
  if(dx > x && dz < z) {
    return 0;
  } else if(dy > y && dz < z) {
    return 1;
  } else if(dy > y && dx < x) {
    return 2;
  } else if(dz > z && dx < x) {
    return 3;
  } else if(dz > z && dy < y) {
    return 4;
  } else if(dx > x && dy < y) {
    return 5;
  } else {
    return 6;
  }
}

//reset by party
void ResetTile(byte party, byte piec) {
  switch(party) {
    case NONE:
      player = NONE;
      pawnState = DECIDE;
      break;
    case Red:
      player = Red;
      break;
    case Blue:
      player = Blue;
      break;
  }
  bestMove = 6;
  shortest = 20;
  piece = piec;
  occupied = false;
}
