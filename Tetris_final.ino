#include <LedControl.h>

LedControl lc = LedControl(11, 13, 10, 1);

#define JOY_X A0
#define JOY_Y A1
#define BTN 2

byte board[8];

int pieceX, pieceY;
int pieceType;
int pieceRotation;

bool isBomb = false;
unsigned long bombBlink = 0;
bool bombState = true;

unsigned long lastDrop = 0;
int dropDelay = 600;

int score = 0;
bool gameOver = false;

bool dropLatch = false;   // <-- TÄRKEIN KORJAUS

// -------- PALIKAT --------
const byte pieces[5][2][4][4] = {

  {
    {{1,1,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}},
    {{1,0,0,0},{1,0,0,0},{0,0,0,0},{0,0,0,0}}
  },

  {
    {{1,1,1,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}},
    {{1,0,0,0},{1,0,0,0},{1,0,0,0},{0,0,0,0}}
  },

  {
    {{1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
    {{1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}}
  },

  {
    {{1,0,0,0},{1,0,0,0},{1,1,0,0},{0,0,0,0}},
    {{1,1,1,0},{1,0,0,0},{0,0,0,0},{0,0,0,0}}
  },

  // Z
  {
    {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
    {{0,1,0,0},{1,1,0,0},{1,0,0,0},{0,0,0,0}}
  }
};
// --------------------------

void clearBoard(){
  for(int i=0;i<8;i++) board[i]=0;
}

void explode(){
  for(int y=-1;y<=1;y++){
    for(int x=-1;x<=1;x++){
      int bx = pieceX + x;
      int by = pieceY + y;
      if(bx>=0 && bx<8 && by>=0 && by<8){
        board[by] &= ~(1<<bx);
      }
    }
  }
}

void showScore(){

  lc.clearDisplay(0);
  delay(300);

  int leds = score;
  if(leds > 8) leds = 8;

  for(int k=0;k<8;k++){
    lc.clearDisplay(0);
    for(int x=0;x<leds;x++){
      lc.setLed(0,0,x,true);
    }
    delay(400);
    lc.clearDisplay(0);
    delay(300);
  }
}

void newPiece(){

  isBomb = (random(0,6) == 5);

  pieceRotation = 0;
  pieceX = 2;
  pieceY = 0;

  if(isBomb){
    pieceX = 3;
    if(collision(pieceX,pieceY)) gameOver = true;
    return;
  }

  pieceType = random(0,5);

  if(collision(pieceX,pieceY)) gameOver = true;
}

bool collision(int x,int y){

  if(isBomb){
    if(x<0||x>7||y>7) return true;
    if(y>=0 && board[y]&(1<<x)) return true;
    return false;
  }

  for(int py=0;py<4;py++){
    for(int px=0;px<4;px++){
      if(pieces[pieceType][pieceRotation][py][px]){
        int bx = x+px;
        int by = y+py;
        if(bx<0||bx>7||by>7) return true;
        if(by>=0 && board[by]&(1<<bx)) return true;
      }
    }
  }
  return false;
}

void lockPiece(){

  if(isBomb){
    explode();
    return;
  }

  for(int py=0;py<4;py++){
    for(int px=0;px<4;px++){
      if(pieces[pieceType][pieceRotation][py][px]){
        int bx = pieceX+px;
        int by = pieceY+py;
        if(by>=0) board[by]|=(1<<bx);
      }
    }
  }
}

void clearLines(){

  for(int y=0;y<8;y++){
    if(board[y]==0xFF){

      score++;

      for(int yy=y;yy>0;yy--){
        board[yy]=board[yy-1];
      }
      board[0]=0;

      if(dropDelay>120) dropDelay-=40;
    }
  }
}

void draw(){

  lc.clearDisplay(0);

  for(int y=0;y<8;y++){
    for(int x=0;x<8;x++){
      if(board[y]&(1<<x)){
        lc.setLed(0,y,x,true);
      }
    }
  }

  if(isBomb){
    if(millis()-bombBlink>200){
      bombBlink=millis();
      bombState=!bombState;
    }
    if(bombState){
      lc.setLed(0,pieceY,pieceX,true);
    }
    return;
  }

  for(int py=0;py<4;py++){
    for(int px=0;px<4;px++){
      if(pieces[pieceType][pieceRotation][py][px]){
        int bx = pieceX+px;
        int by = pieceY+py;
        if(by>=0 && by<8 && bx>=0 && bx<8){
          lc.setLed(0,by,bx,true);
        }
      }
    }
  }
}

void setup(){

  lc.shutdown(0,false);
  lc.setIntensity(0,8);
  lc.clearDisplay(0);

  pinMode(BTN,INPUT_PULLUP);

  randomSeed(analogRead(0));

  clearBoard();
  newPiece();
}

void loop(){

  if(gameOver){

    showScore();

    clearBoard();
    score=0;
    dropDelay=600;
    gameOver=false;

    delay(500);
    newPiece();
    return;
  }

  int joyX = analogRead(JOY_X);
  int joyY = analogRead(JOY_Y);

  // RESET latch kun keppi ylhäällä
  if(joyY < 500) dropLatch = false;

  // HARD DROP VAIN YHDEN KERRAN PER PAINALLUS
  if(joyY > 700 && !dropLatch){

    while(!collision(pieceX,pieceY+1)){
      pieceY++;
    }

    lockPiece();
    clearLines();
    newPiece();

    dropLatch = true;
    return;
  }

  if(joyX < 300){
    if(!collision(pieceX-1,pieceY)) pieceX--;
    delay(110);
  }
  if(joyX > 700){
    if(!collision(pieceX+1,pieceY)) pieceX++;
    delay(110);
  }

  if(!isBomb && digitalRead(BTN)==LOW){
    int oldRot = pieceRotation;
    pieceRotation = (pieceRotation+1)%2;
    if(collision(pieceX,pieceY)) pieceRotation=oldRot;
    delay(200);
  }

  if(millis()-lastDrop > dropDelay){

    if(!collision(pieceX,pieceY+1)){
      pieceY++;
    }
    else{
      lockPiece();
      clearLines();
      newPiece();
      return;
    }

    lastDrop = millis();
  }

  draw();
}
