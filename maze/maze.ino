#include <Adafruit_SSD1306.h>

typedef unsigned char loc;

const int  SCALE         = 4;
const loc  WIDTH         = 128 / SCALE - 1;
const loc  HEIGHT        = 64 / SCALE - 1;
const char WALL          = '#';
const char SPACE         = ' ';

const int  SCREEN_WIDTH  = 128;
const int  SCREEN_HEIGHT = 64;
const int  MOVE_INTERVAL = 300;
const int  FLASH_DELAY   = 200;

const int  SHIFT_LEFT    = (SCREEN_WIDTH - WIDTH * SCALE) / 2;
const int  SHIFT_TOP     = (SCREEN_HEIGHT - HEIGHT * SCALE) / 2;

const int  LEFT_BUTTON   = 4;
const int  RIGHT_BUTTON  = 18;
const int  UP_BUTTON     = 19;
const int  DOWN_BUTTON   = 5;

const int  BUZZER        = 23;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT);
char maze[WIDTH * HEIGHT];

void get_neighbor(loc dir, loc x, loc y, loc* next_x, loc* next_y) {
  switch (dir) {
    case 0:  // Left
      *next_x = x - 1;
      *next_y = y;
      break;
    case 1:  // Right
      *next_x = x + 1;
      *next_y = y;
      break;
    case 2:  // Up
      *next_x = x;
      *next_y = y - 1;
      break;
    case 3:  // Down
      *next_x = x;
      *next_y = y + 1;
      break;
  }
}

loc get_end() {
  for (loc i = 0; i < HEIGHT; i++) {
    if (maze[WIDTH * i + WIDTH - 1] == SPACE) {
      return i;
    }
  }
  return HEIGHT;
}

bool is_valid(loc next_x, loc next_y, loc curr_x, loc curr_y) {
  if (next_x <= 0 ||
      next_y <= 0 ||
      next_x >= WIDTH ||
      next_y >= HEIGHT - 1) {
    return false;
  }
  if (next_x == WIDTH - 1) {
    loc end = get_end();
    if (end != HEIGHT && next_y != end) {
      return false;
    }
  }
  if (maze[WIDTH * next_y + next_x] == SPACE) {
    return false;
  }
  for (loc i = 0; i < 4; i++) {
    loc neighbor_x, neighbor_y;
    get_neighbor(i, next_x, next_y, &neighbor_x, &neighbor_y);
    if (neighbor_x == curr_x && neighbor_y == curr_y) {
      continue;
    }
    if (neighbor_x == WIDTH) {
      continue;
    }
    if (maze[WIDTH * neighbor_y + neighbor_x] == WALL) {
      continue;
    }
    return false;
  }
  return true;
}

class stack {
private:
  struct frame {
    loc dirs[4];
    loc x, y, i;
  };
  frame buf[WIDTH * HEIGHT];
  int idx;
public:
  stack(): idx(0) {}
  
  void push(loc x, loc y, loc i) {
    buf[idx].dirs[0] = 0;
    buf[idx].dirs[1] = 1;
    buf[idx].dirs[2] = 2;
    buf[idx].dirs[3] = 3;
    for (loc i = 0; i < 4; i++) {
      loc j = random(4);
      loc tmp = buf[idx].dirs[i];
      buf[idx].dirs[i] = buf[idx].dirs[j];
      buf[idx].dirs[j] = tmp;
    }
    
    buf[idx].x = x;
    buf[idx].y = y;
    buf[idx].i = i;
    
    idx++;
  }

  inline loc x() {
    return buf[idx - 1].x;
  }

  inline loc y() {
    return buf[idx - 1].y;
  }

  inline loc i() {
    return buf[idx - 1].i;
  }

  inline loc inc_i() {
    return buf[idx - 1].i++;
  }

  inline loc dirs(int i) {
    return buf[idx - 1].dirs[i];
  }

  inline int depth() {
    return idx;
  }

  inline void pop() {
    idx--;
  }
};

void create_space(loc start) {
  stack s;
  s.push(0, start, 0);
  while (s.depth() > 0) {
    while (s.i() < 4) {
      loc next_x, next_y;
      get_neighbor(s.dirs(s.inc_i()), s.x(), s.y(), &next_x, &next_y);
      if (is_valid(next_x, next_y, s.x(), s.y())) {
        maze[WIDTH * next_y + next_x] = SPACE;
        s.push(next_x, next_y, 0);
      }
    }
    s.pop();
  }
}

loc start, end;

void generate() {
  for (int i = 0; i < WIDTH * HEIGHT; i++) {
    maze[i] = WALL;
  }
  start = random(HEIGHT - 2) + 1;
  maze[WIDTH * start] = SPACE;
  create_space(start);
  end = get_end();
}

void show(String s) {
  display.clearDisplay();

  int x = (SCREEN_WIDTH / 6 - s.length()) * 3;
  if (x < 0) {
    x = 0;
  }
  int y = SCREEN_HEIGHT / 2;
  display.setCursor(x, y);
  display.println(s);
  
  display.display();
}

void draw_cell(loc col, loc row, bool is_rounded, unsigned color) {
  const int x0 = col * SCALE, x1 = (col + 1) * SCALE - 1;
  const int y0 = row * SCALE, y1 = (row + 1) * SCALE - 1;
  for (int y = row * SCALE; y < (row + 1) * SCALE; y++) {
    for (int x = col * SCALE; x < (col + 1) * SCALE; x++) {
      if (is_rounded &&
          (x == x0 && y == y0 ||
           x == x1 && y == y0 ||
           x == x0 && y == y1 ||
           x == x1 && y == y1)) {
        continue;
      }
      display.drawPixel(x + SHIFT_LEFT, y + SHIFT_TOP, color);
    }
  }
}

void draw() {
  display.clearDisplay();
  for (loc row = 0; row < HEIGHT; row++) {
    for (loc col = 0; col < WIDTH; col++) {
      if (maze[WIDTH * row + col] == WALL) {
        draw_cell(col, row, false, WHITE);
      }
    }
  }
  display.display();
}

void check_buttons(bool* up, bool* down, bool* left, bool*right) {
  if (digitalRead(LEFT_BUTTON) == LOW) {
    *left = true;
  }
  if (digitalRead(RIGHT_BUTTON) == LOW) {
    *right = true;
  }
  if (digitalRead(UP_BUTTON) == LOW) {
    *up = true;
  }
  if (digitalRead(DOWN_BUTTON) == LOW) {
    *down = true;
  }
}

void beep(unsigned int duration) {
  digitalWrite(BUZZER, HIGH);
  delay(duration);
  digitalWrite(BUZZER, LOW);
}

loc x, y;

void setup() {
  pinMode(LEFT_BUTTON, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON, INPUT_PULLUP);
  pinMode(UP_BUTTON, INPUT_PULLUP);
  pinMode(DOWN_BUTTON, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  randomSeed(ESP.getCycleCount());
  generate();
  draw();

  x = 0;
  y = start;
}

unsigned long last_color_change = 0;
unsigned long last_move = 0;
bool is_hidden = true;

void move(int new_x, int new_y) {
  if (!is_hidden) {
    draw_cell(x, y, true, BLACK);
  }
  x = new_x;
  y = new_y;
  is_hidden = false;
  draw_cell(x, y, true, WHITE);
  last_move = millis();
  last_color_change = millis();
  beep(5);
}

void loop() {
  if (x == WIDTH - 1) {
    delay(UINT_MAX);
    return;
  }
  
  if (millis() - last_color_change > FLASH_DELAY) {
    is_hidden = !is_hidden;
    draw_cell(x, y, true,is_hidden ? BLACK : WHITE);
    last_color_change = millis();
  }
  
  bool left = false, right = false, up = false, down = false;
  check_buttons(&up, &down, &left, &right);
  if (millis() - last_move > MOVE_INTERVAL) {
    if (left && x >= 1 && maze[WIDTH * y + x - 1] == SPACE) {
      move(x - 1, y);
    }
    if (right && x < WIDTH - 1 && maze[WIDTH * y + x + 1] == SPACE) {
      move(x + 1, y);
    }
    if (up && y > 1 && maze[WIDTH * (y - 1) + x] == SPACE) {
      move(x, y - 1);
    }
    if (down && y < HEIGHT - 2 && maze[WIDTH * (y + 1) + x] == SPACE) {
      move(x, y + 1);
    }
  }
  
  display.display();

  if (x == WIDTH - 1) {
    delay(500);
    show("!!! YOU WIN !!!");
    beep(200);
  }
}
