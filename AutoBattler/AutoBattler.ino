#define GREY makeColorRGB(105,105,105)
 
enum SIGNAL {EMPTY, SETCOORD, STARTGAME, REQUESTCOORD, RESPONSECOORD, REQUESTMOVE, RESPONSEMOVE, MOVEINFO, ATTACK, SEARCH, RESPOND, SEARCHENEMY, SEARCHRES};
enum SEARCHSTATE {UNEXPLORED, EXPLORED, TRAIL, YOU};
byte knowledge = UNEXPLORED;
byte moreinfo = knowledge;

//player party --------------
enum PARTY {NONE, Red, Blue};
byte player = NONE;

enum STATE {SETUP, INPROGRESS, GAMESTART ,GAMEEND};
byte state = SETUP;

enum pieces {TANK, FIGHTER, RANGER, HEALER};
byte piece = TANK;
byte hp = 1;
//position of this blink on the board
byte x;
byte y;
byte z;

Timer timer;
Timer effectTimer; //effect duration after hit
Timer gameTimer;
byte matchCount = 0;

//Main Code Body-----------------------------------------------------------------------------------
void setup() {
  setColor(WHITE);
}

void loop() {
  //state control
  switch(state) {
    case SETUP:
      setValueSentOnAllFaces(0);
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
    case GAMEEND://add end state
      EndLoop();
      break;
  }
  SetPlayerColor();
}


void EndLoop() {
  if(buttonDoubleClicked() && matchCount > 0){
    gameTimer.set(2000);
    matchCount = 0;
    setValueSentOnAllFaces(63);
  }
  FOREACH_FACE(f) {
    if(!isValueReceivedOnFaceExpired(f)){
      if(getLastValueReceivedOnFace(f) == 63 && matchCount > 0){
      gameTimer.set(2000);
      matchCount = 0;
      setValueSentOnAllFaces(63);
      }
    }
  }
  
  if (gameTimer.isExpired() && matchCount == 0) {
    state = SETUP;
    setColor(GREEN);
    matchCount = 0;
    player = NONE;
    piece = TANK;
    
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
  if(buttonLongPressed()) {
    player = (player+1) % 3;
    SetUpHP();
  }
 
  if(buttonSingleClicked() && player != NONE) {
    piece = (piece+1) % 4;
    SetUpHP();
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
//*************************************************************** Edit Hp here
void SetUpHP() {
  if(player == NONE) return;
  switch(piece) {
    case TANK:
      hp = 5;
      break;
    case FIGHTER:
      hp = 3;
      break;
    case RANGER:
      hp = 3;
      break;
    case HEALER:
      hp = 2;
      break;
  }
}

//Preparation Time Before Pawns Move(Allow some time for setting up coordinates)------------
void PrepareLoop() {
  if(timer.isExpired()) {
    setColor(GREEN);
    state = GAMESTART;
    timer.set(1000);
    gameTimer.set(15000);
  }
}

//Game Start. Pawns move----------------------------------------------------------
enum PawnState {SEARCHING, DECIDE, MOVE, RESET};
byte pawnState = SEARCHING;
//destination this pawn is moving towards
byte des[] = {10, 10, 10};
//bestMove indicates the next neighbor to move to;
byte bestMove = 6;
byte shortest = 20;
//once a neighboring pawn is allowed to move in,  this tile is locked
bool occupied = false;
//Marks if an enemy is found
bool searching = true;

void GameLoop() {
  if (gameTimer.isExpired()){
      matchCount = 1;
      state = GAMEEND;
      
      //gameTimer.set(5000);
  }
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
          case SEARCHRES:
            pawnState = SEARCHING;
            PawnAttack(f);
            break;
        }
      }
    }
   
    if(isDatagramReadyOnFace(f)) {
      const byte* data = getDatagramOnFace(f);
      switch(data[0]){
      //Handles coordinate from the neighbor
        case RESPONSECOORD:
        byte dis = Distance(data[1], data[2], data[3]);
        byte self = Distance(x, y, z);
        if(self > dis && shortest > dis) {
          shortest = dis;
          bestMove = f;
        }
          break;
     
        case MOVEINFO:
        ResetTile(data[1],data[2],data[3]);
      break;
     
        case SEARCHENEMY:
        if(player != NONE && player != data[1]) {
          setColor(ORANGE);
          setValueSentOnFace(SEARCHRES,f);
        }
      break;
     
        case ATTACK:
        if(data[1] != RANGER) {
          DealDamage(data[3]);
        } else { //ranger's attack
          DealDamage(data[3]);
          byte panetration[4] = {ATTACK, TANK, data[2], data[3]};
          sendDatagramOnFace(panetration, 4, data[2]);
        }
      break;
          
        case SEARCH:
          if (data [4] != 0 && knowledge == UNEXPLORED){
            transmitSearchSignal(data);
            rface = f;
            knowledge = EXPLORED;
            moreinfo = EXPLORED;
            if(player != NONE){
              sendRespondSignal();
              }
            }
          break;
          
        case RESPOND:
          if(player == NONE)
            moreinfo = TRAIL;
          transmitRespondSignal(data);
          if(rface == 6)
            moreinfo = YOU;
          break;
      }
          
      markDatagramReadOnFace(f);
    }
  }
 
}

//Each pawn move is split into decision of which path to take and communicate with the neighbor to move.
void PawnAutoDecide() {
  if(player == NONE) return;
 
  if(pawnState == SEARCHING) {
    timer.set(500);
    pawnState = DECIDE;
    SearchForEnemy();
    return;
  }
 
  if(x == des[0] && y == des[1] && z == des[2]) {
    pawnState = SEARCHING;
    return;
  }
  
  if(pawnState == DECIDE) {
    timer.set(500);
    pawnState = MOVE;
    NavRequest();
    return;
  }
 
  if(pawnState == MOVE) {
    pawnState = SEARCHING;
    timer.set(500);
    SendMoveRequest();
  }
}

void SearchForEnemy() {
  byte data[2] = {SEARCHENEMY, player};
  FOREACH_FACE(f) {
    sendDatagramOnFace(data, 2, f);
  }
}
//Pawn attacks position x,y,z
//********************************************************NOTE: edit damage here!
void PawnAttack(byte f) {
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

void SendPropogateDmg(byte data[], byte f) {
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
void DealDamage(byte dmg) {
  if(player == NONE) return;
  effectTimer.set(500);
  if(hp <= dmg) {
    ResetTile(NONE,TANK,6);
  }
  else
    hp -= dmg;
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
  if(bestMove == 6) {
    return;
  }
  setValueSentOnFace(REQUESTMOVE, bestMove);
}
//respond only if move is possible
void SendMoveResponse(byte face) {
  setValueSentOnFace(RESPONSEMOVE, face);
}
//send in info about this pawn.
void SendMove(byte face) {
  byte data[4] = {MOVEINFO, player, piece, hp};
  sendDatagramOnFace(data, 4, face);
  ResetTile(NONE, TANK, 6);
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
//****************************************************************** Edit pawn color here
void SetPlayerColor() {
  if(!effectTimer.isExpired()) {
    setColor(MAGENTA);
    return;
  }
 
  if(player == Red) {
    setColor(RED);
    SetPieceStyle();
  }
 
  if(player == Blue) {
    setColor(BLUE);
    SetPieceStyle();
  }
 
  if(player == NONE){
    if(state == GAMEEND){
      setColor(WHITE);
    }else{
      setColor(GREEN);
    }
    
  }
    
}
//****************************************************************** Edit pawn style here
void SetPieceStyle() {
  switch(piece) {
    case TANK:
      setColorOnFace(GREY,0);
      setColorOnFace(GREY,1);
      setColorOnFace(GREY,2);
      setColorOnFace(GREY,5);
      break;
    case FIGHTER:
      setColorOnFace(YELLOW,0);
      setColorOnFace(YELLOW,1);
      setColorOnFace(YELLOW,2);
      break;
    case RANGER:
      setColorOnFace(ORANGE,2);
      setColorOnFace(ORANGE,5);
      break;
    case HEALER:
      setColorOnFace(GREEN,0);
      break;
     
  }
}

byte GetAttackFace(byte dx, byte dy, byte dz) {


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
void ResetTile(byte party, byte piec, byte health) {
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
  hp = health;
  bestMove = 6;
  shortest = 20;
  piece = piec;
  occupied = false;
}
