/*
This is just to figure out how to work with the colors and faces.
It was pain. So much pain.
*/


byte curr=0;
Color curCol=WHITE;

Timer tracker;

Color nextColor(){
	switch(curr++%7){
    	case 0:
        	return RED;
            break;
    	case 1:
        	return MAGENTA;
            break;
    	case 2:
        	return ORANGE;
            break;
    	case 3:
        	return YELLOW;
            break;
    	case 4:
        	return GREEN;
            break;
    	case 5:
        	return CYAN;
            break;
    	case 6:
        	return BLUE;
            break;
    }
    
}



void setup() {
    setColor(OFF);
  tracker.set(6000);
}

void loop() {
  if (tracker.isExpired()) {
	curCol = nextColor();
    tracker.set(6000);
  }
  setColorOnFace(curCol, ((6000-tracker.getRemaining())/1000)%6);  
}
