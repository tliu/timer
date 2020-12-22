#include <Bounce2.h>
#include <Keyboard.h>
#include <limits.h>

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
unsigned long timer = 0;
enum { MAIN, PRE_INSPECTION, INSPECTION, PRE_TIME, TIMING, END_TIME } state = MAIN;
unsigned long times[25];
int time_index = 0;
unsigned long best = LONG_MAX;
boolean needs_update = true;
boolean inspection = true;

void readable(char *str, unsigned long time) {
    if (!time) {
        sprintf(str, "-:--.--");
        return;
    }
    unsigned long all_seconds = time / 1000;    
    unsigned long ms = time % 1000;
    unsigned long minutes = all_seconds / 60;
    unsigned long seconds = all_seconds % 60;
    if (minutes > 9) {
        minutes = 9;
    }
    if (ms > 99) {
        ms /= 10;
    }
    sprintf(str, "%01lu:%02lu.%02lu", minutes, seconds, ms);
    return;
}

unsigned long min_arr(unsigned long* arr, int len) {
    unsigned long curr = ULONG_MAX;
    for (int i = 0; i < len; i++) {
        if (arr[i] < curr) {
            curr = arr[i];
        }
    }
    return curr;
}

unsigned long max_arr(unsigned long* arr, int len) {
    unsigned long curr = 0;
    for (int i = 0; i < len; i++) {
        if (arr[i] > curr) {
            curr = arr[i];
        }
    }
    return curr;
}

void ao(int num) {
    if (time_index < num) {
        write_line(2, "ao5 --:--.--");
        return;
    }
    unsigned long last[num];
    for (int i = 0; i < num; i++) {
        last[i] = times[time_index - i];
    }

    unsigned long min_last = min_arr(last, num);
    unsigned long max_last = max_arr(last, num);
    
    unsigned long sum = 0;
    Serial.println("i?");
    for (int i = 0; i < num; i++) {
        Serial.print(i);
        Serial.print(",");
         Serial.print(last[i]);
        Serial.print("\n");
        if (last[i] != min_last && last[i] != max_last) {
            sum += last[i];
        }
    }
    unsigned long avg = sum / (num - 2);
    char r[8] = {0};
    readable(r, avg);
    char out[12] = {0};
    sprintf(out, "ao5 %s", r);
    write_line(2, out);
}

void main_screen(int num) {
    char curr[8] = {0};
    readable(curr, times[time_index]);
    unsigned long b = best;
    if (best == LONG_MAX) {
        b = 0; 
    }
    char best_str[8] = {0};
    readable(best_str, b);
    ao(num);
    char out[17] = {0};
    sprintf(out, "%s b%s", curr, best_str);
    write_line(1, out);
}

void write_line(int line, String msg) {
    Serial1.write(0xFE);
    switch(line) {
        case 1:
            Serial1.write(0x80);
            break;
        case 2:
            Serial1.write(0x80 + 64);
            break;
    }
    delay(10);
    Serial1.print(msg);
}

void clear_screen() {
    Serial1.write(0xFE);
    Serial1.write(0x01);
    delay(10);
}

void setup() {
    Serial.begin(9600);
    Serial1.begin(9600);
    pinMode(A8, INPUT_PULLUP);
    pinMode(A9, INPUT_PULLUP);

    delay(500);
    swap_state(MAIN);
}

void swap_state(int s) {
    clear_screen();
    state = s;
    needs_update = true;
}

unsigned long last_millis = 0;
boolean plus_two = false;
unsigned long inspection_timer = 15;

void loop() {
    boolean left = digitalRead(A8) == LOW;
    boolean right = digitalRead(A9) == LOW;
    
    if (Serial1.availableForWrite()) {
        if (state == PRE_TIME) {
            write_line(1, "   release to");
            write_line(2, "     start");
            if (!left && !right) {
                swap_state(TIMING);
            }
        } else if (state == TIMING) {
            if (needs_update) {
                last_millis = millis();
                needs_update = false;
                write_line(1, "     solve!     ");
            } else {
                int pt = plus_two ? 2000 : 0;
                timer = millis() - last_millis + pt;
            }
            char r[8] = {0};
            readable(r, timer);
            char out[12] = {0};
            sprintf(out, "    %s", r);
            write_line(2, out);
            if (left && right) {
                swap_state(END_TIME);
            }
        } else if (state == END_TIME) {
            if (!left && !right) {
                time_index++;
                times[time_index] = timer;
                Serial.println("adding time");
                for (int i = 0; i < time_index; i++) {
                    Serial.print(times[i]);
                    Serial.print(", ");
                }
                Serial.print("\n");
                if (timer < best) {
                    best = timer;
                }
                if (time_index > 99) {
                    //reset times?
                }
                swap_state(MAIN);
            }
        } else if (state == MAIN) {
            if (needs_update) { 
                main_screen(5);
                needs_update = false;
            }
            if (left && right) {
                if (inspection) {
                    swap_state(PRE_INSPECTION);
                } else {
                    swap_state(PRE_TIME);
                }
            }
        } else if (state == PRE_INSPECTION) {
                write_line(1, "  release for");
                write_line(2, "   inspection");
                if (!left && !right) {
                    swap_state(INSPECTION);
                }
        } else if (state == INSPECTION) {
            if (needs_update) {
                last_millis = millis();
                inspection_timer = 15;
                needs_update = false;
            } else {
                timer = millis() - last_millis;
            }
            long time_spent = inspection_timer - timer / 1000;
            const char buffer[17] = {0};
            sprintf(buffer, "       %2lu       ", time_spent);
            write_line(1, buffer);  
            write_line(2, "                ");
            if (time_spent < 0) {
                plus_two = true;
                swap_state(TIMING);
            }
            if (left && right && timer > 1000) {
                plus_two = false;
                swap_state(PRE_TIME);
            }       
        }
    }
    Serial.flush();
    Serial1.flush();
}
