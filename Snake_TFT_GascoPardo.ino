#include "Gesture.h"
#include <Arduino.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>


// --------------------------------------------
// --------------- IMPORTANTE -----------------
// --------------------------------------------
// Para que funcione con nuestro TFT LCD,
// es necesario descomentar la linea:
//   #define SUPPORT_8347D
// en la librería MCUFRIEND_kbv.
// --------------------------------------------



// Pines para la pantalla táctil
#define YP A3 // debe ser un pin analógico
#define XM A2 // debe ser un pin analógico
#define YM 9  // puede ser un pin digital
#define XP 8  // puede ser un pin digital

// Calibración de la pantalla táctil
#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940

// Constantes del juego
#define MAX_LENGTH 25
#define SNAKE_COLOR_1 0xF800
#define SNAKE_COLOR_2 0x001F
#define BG_COLOR 0x0000
#define FOOD_COLOR 0x07E0
#define NUM_FOOD_ITEMS 8




// ------------------------------------
// --- Definición de la Clase Food ----
// ------------------------------------

class Food {
  private:
    int positions[NUM_FOOD_ITEMS][2];
    MCUFRIEND_kbv &tft;

  public:
    Food(MCUFRIEND_kbv &tft) : tft(tft) {
      generateNewPositions();
    }

    void setPosition(int index, int x, int y) {
      if (index < 0 || index >= NUM_FOOD_ITEMS) return;
      positions[index][0] = x;
      positions[index][1] = y;
    }

    void getPosition(int index, int &x, int &y) {
      if (index < 0 || index >= NUM_FOOD_ITEMS) {
        x = -1; // Valor de error
        y = -1; // Valor de error
        return;
      }
      x = positions[index][0];
      y = positions[index][1];
    }

    void draw() {
      for (int i = 0; i < NUM_FOOD_ITEMS; i++) {
        tft.fillRect(positions[i][0] * 10, positions[i][1] * 10, 10, 10, FOOD_COLOR);
      }
    }

    void generateNewPositions() {
      for (int i = 0; i < NUM_FOOD_ITEMS; i++) {
        positions[i][0] = random(0, tft.width() / 10);
        positions[i][1] = random(0, tft.height() / 10);
      }
    }

    void generateNewPosition(int index) {
      if (index < 0 || index >= NUM_FOOD_ITEMS) return;
      positions[index][0] = random(0, tft.width() / 10);
      positions[index][1] = random(0, tft.height() / 10);
    }

};



// ------------------------------------
// --- Definición de la Clase Snake ---
// ------------------------------------

class Snake {
  private:
    int length;
    uint16_t color;
    MCUFRIEND_kbv &tft;
    int currentDirection;
    int positions[100][2];

  public:
    Snake(uint16_t initColor, int startX, int startY, int startDirection,
          MCUFRIEND_kbv &tft)
      : tft(tft) {
      length = 20;
      color = initColor;
      positions[0][0] = startX / 10;
      positions[0][1] = startY / 10;
      positions[1][0] = startX / 10;
      positions[1][1] = (startY / 10) + 1;
      currentDirection = startDirection;
    }

    int getLength() {
      return length;
    }

    void setLength(int newLength) {
      length = newLength;
    }

    int getCurrentDirection() {
      return currentDirection;
    }

    void setCurrentDirection(int newCurrentDirection) {
      currentDirection = newCurrentDirection;
    }

    uint16_t getColor() {
      return color;
    }

    void setColor(uint16_t newColor) {
      color = newColor;
    }

    void grow() {
      length += 1;
      positions[length - 1][0] = positions[length - 2][0];
      positions[length - 1][1] = positions[length - 2][1];
    }

    void getPosition(int index, int &x, int &y) {
      if (index < 0 || index >= length) {
        x = -1; // Valor de error
        y = -1; // Valor de error
        return;
      }
      x = positions[index][0];
      y = positions[index][1];
    }

    void move() {
      int oldTailX = positions[length - 1][0];
      int oldTailY = positions[length - 1][1];

      for (int i = length - 1; i > 0; i--) {
        positions[i][0] = positions[i - 1][0];
        positions[i][1] = positions[i - 1][1];
      }
      switch (currentDirection) {
        case 0: // Arriba
          positions[0][1]--;
          if (positions[0][1] < 0) {
            positions[0][1] = tft.height() / 10 - 1;
          }
          break;
        case 1: // Derecha
          positions[0][0]++;
          if (positions[0][0] >= tft.width() / 10) {
            positions[0][0] = 0;
          }
          break;
        case 2: // Abajo
          positions[0][1]++;
          if (positions[0][1] >= tft.height() / 10) {
            positions[0][1] = 0;
          }
          break;
        case 3: // Izquierda
          positions[0][0]--;
          if (positions[0][0] < 0) {
            positions[0][0] = tft.width() / 10 - 1;
          }
          break;
      }

      clearTail(oldTailX, oldTailY);
    }

    void draw() {
      for (int i = 0; i < length; i++) {
        tft.fillRect(positions[i][0] * 10, positions[i][1] * 10, 10, 10, color);
      }
    }

    void clearTail(int x, int y) {
      tft.fillRect(x * 10, y * 10, 10, 10, BG_COLOR);
    }

    bool checkCollisionWithSelf() {
      int headX = positions[0][0];
      int headY = positions[0][1];
      for (int i = 1; i < length; i++) {
        if (positions[i][0] == headX && positions[i][1] == headY) {
          return true;
        }
      }
      return false;
    }

    int checkFoodCollision(Food &food) {
      int headX = positions[0][0];
      int headY = positions[0][1];
      int foodX, foodY;
      for (int i = 0; i < NUM_FOOD_ITEMS; i++) {
        food.getPosition(i, foodX, foodY);
        if (headX == foodX && headY == foodY) {
          return i;
        }
      }
      return -1;
    }
};



// ------------------------------------
// - Definición de variables globales -
// ------------------------------------

int winner;
int state = 0;
int delayTime = 100;
int levelIterations = 1000;
MCUFRIEND_kbv tft;
Food *food = NULL;
Snake *snake1 = NULL;
Snake *snake2 = NULL;
int gestureSensors = 0;
int updateIterations = 3;
bool sensor1Initialized = false;
bool sensor2Initialized = false;
pag7660_gesture_t result1, result2;
pag7660 Gesture1(GESTURE_COMBINED_MODE, true);
pag7660 Gesture2(GESTURE_COMBINED_MODE, false);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);




// ------------------------------------
// - Procedimientos de inicialización -
// ------------------------------------

void setup() {
  Serial.begin(9600);
  setupTFT();
  setupGestureSensors();
  setupSnakes();
  setupFood();
}

void setupTFT() {
  uint16_t identifier = tft.readID();
  tft.begin(identifier);
  tft.setRotation(1);
  tft.fillScreen(BG_COLOR);
}

void setupGestureSensors() {
  sensor1Initialized = Gesture1.init();
  sensor2Initialized = Gesture2.init();

  if (!sensor1Initialized && !sensor2Initialized) {
    state = -1;
  }
}

void setupSnakes() {
  if (sensor1Initialized) {
    snake1 = new Snake(SNAKE_COLOR_1, 20, 10, 0, tft);
  }

  if (sensor2Initialized) {
    snake2 = new Snake(SNAKE_COLOR_2, 280, 200, 0, tft);
  }
}

void setupFood() {
  randomSeed(analogRead(0));
  food = new Food(tft);
}




// ------------------------------------
// ----- Procedimiento principal ------
// ------------------------------------

void loop() {

  // Init error (state == -1)
  if (state == -1) {
    displayErrorMenu();
  }

  // Main menu (state == 0)
  else if (state == 0) {
    displayMainMenu();
  }

  // Play mode (state == 1)
  else if (state == 1) {
    updateGestures();
    checkIfIncreaseSpeed();
    checkEndGame();
    checkSelfCollision();
    checkFoodCollision();
    gestureInputHandler();
    drawGame();
  }

  // Paused game (state == 2)
  else if (state == 2) {
    updateGestures();
    displayPausedMenu();
    resumeGameHandler();
  }

  // End game (state == 3)
  else if (state == 3) {
    displayEndMessage();
    delay(1000);
  }

  delay(delayTime);
}


void checkIfIncreaseSpeed() {
  levelIterations--;
  if (levelIterations == 0 && delayTime > 20) {
    delayTime -= 20;
    levelIterations = 100;
  }
}


void updateGestures() {
  Gesture1.getResult(result1);
  Gesture2.getResult(result2);
}


void checkFoodCollision() {
  if (sensor1Initialized) {
    int collisionIndex = snake1->checkFoodCollision(*food);
    if (collisionIndex != -1) {
      snake1->grow();
      food->generateNewPosition(collisionIndex);
    }
  }

  if (sensor2Initialized) {
    int collisionIndex = snake2->checkFoodCollision(*food);
    if (collisionIndex != -1) {
      snake2->grow();
      food->generateNewPosition(collisionIndex);
    }
  }
}

void checkSelfCollision() {
  if (sensor1Initialized && snake1->checkCollisionWithSelf()) {
    if (sensor2Initialized) {
      winner = 2;
    } else {
      winner = 0;
    }
    state = 3;
    return;
  }

  if (sensor2Initialized && snake2->checkCollisionWithSelf()) {
    if (sensor1Initialized) {
      winner = 1;
    } else {
      winner = 0;
    }
    state = 3;
    return;
  }

}

void checkEndGame() {
  if (sensor1Initialized && snake1->getLength() >= MAX_LENGTH) {
    winner = 1;
    state = 3;
    return;
  }

  if (sensor2Initialized && snake2->getLength() >= MAX_LENGTH) {
    if (sensor1Initialized) {
      winner = 2;
    } else {
      winner = 1;
    }
    state = 3;
    return;
  }
}

void drawGame() {
  if (sensor1Initialized) snake1->draw();
  if (sensor2Initialized) snake2->draw();
  food->draw();
}

// ------------------------------------
// ----- Display y lógica de menú -----
// ------------------------------------

void displayErrorMenu() {
  tft.setTextSize(5);
  tft.setCursor(100, 100);
  tft.println("ERROR");

  tft.setTextSize(2);
  tft.setCursor(0, 150);
  tft.println("Check wires and reset game");
}


void displayMainMenu() {
  tft.setTextSize(5);
  tft.setCursor(12, 100);
  tft.println("START GAME");

  tft.setTextSize(2);
  tft.setCursor(12, 150);
  tft.println("Touch the screen to begin");

  waitForTouch();
}


void displayPausedMenu() {
  tft.setTextSize(5);
  tft.setCursor(80, 100);
  tft.println("PAUSED");

  tft.setTextSize(2);
  tft.setCursor(12, 150);
  tft.println("Close one hand to resume");
}

void waitForTouch() {
  TSPoint p = ts.getPoint();
  pinMode(YP, OUTPUT);
  pinMode(XM, OUTPUT);

  if (p.z > TS_MINX && p.z < TS_MAXX) {
    state = 1;
    tft.fillScreen(BG_COLOR);
  }
}

void displayEndMessage() {
  tft.fillScreen(BG_COLOR);

  // Single mode and player loses
  if (winner == 0) {
    tft.setTextSize(5);
    tft.setCursor(12, 100);
    tft.println("GAME OVER!");
    tft.setTextSize(2);
    tft.setCursor(20, 150);
    tft.println("You lost! Try again...");
    return;
  }

  // Multiplayer mode and player 1 wins
  if (winner == 1) {
    tft.setTextSize(5);
    tft.setCursor(75, 100);
    tft.println("P1 WON");
    tft.setTextSize(2);
    tft.setCursor(70, 150);
    tft.println("Congratulations!");
    return;
  }

  // Multiplayer mode and player 2 wins
  if (winner == 2) {
    tft.setTextSize(5);
    tft.setCursor(75, 100);
    tft.println("P2 WON");
    tft.setTextSize(2);
    tft.setCursor(70, 150);
    tft.println("Congratulations!");
    return;
  }


}

// ------------------------------------
// ----- Gestores de movimientos ------
// ------------------------------------

void gestureInputHandler() {
  if ((sensor1Initialized && pauseGame(&result1)) ||
      (sensor2Initialized && pauseGame(&result2))) {
    state = 2;
    return;
  }

  if (sensor1Initialized) {
    int currentDirection = snake1->getCurrentDirection();
    if (goRight(&result1)) {
      snake1->setCurrentDirection((currentDirection + 1 + 4) % 4);
    } else if (goLeft(&result1)) {
      snake1->setCurrentDirection((currentDirection - 1 + 4) % 4);
    }
    snake1->move();
  }

  if (sensor2Initialized) {
    int currentDirection2 = snake2->getCurrentDirection();
    if (goRight(&result2)) {
      snake2->setCurrentDirection((currentDirection2 + 1 + 4) % 4);
    } else if (goLeft(&result2)) {
      snake2->setCurrentDirection((currentDirection2 - 1 + 4) % 4);
    }
    snake2->move();
  }
}


void resumeGameHandler() {
  if ((sensor1Initialized && resumeGame(&result1)) ||
      (sensor2Initialized && resumeGame(&result2))) {
    tft.fillScreen(BG_COLOR);
    state = 1;
    return;
  }
}

bool pauseGame(pag7660_gesture_t *result) {
  if (result == nullptr) return false;
  if (result->type == 5) return true;
  return false;
}

bool resumeGame(pag7660_gesture_t *result) {
  if (result == nullptr) return false;
  if (result->type == 0) return true;
  return false;
}

bool goLeft(pag7660_gesture_t *result) {
  if (result == nullptr) return false;
  if (result->type == 8) return true;
  if (result->type == 7) Serial.println(result->rotate);
  return false;
}

bool goRight(pag7660_gesture_t *result) {
  if (result == nullptr) return false;
  if (result->type == 9) return true;
  if (result->type == 6) Serial.println(result->rotate);
  return false;
}
