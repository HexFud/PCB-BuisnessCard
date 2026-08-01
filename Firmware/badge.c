#include <ch552.h>
#include <stdint.h>
#include <stdlib.h>

#define SDA_PIN   P3_2
#define SCL_PIN   P3_3
#define SW1_PIN   P1_1
#define SW2_PIN   P1_5

#define ADDR_IS31FL3731   0x74

#define IS31_REG_CONFIG_BANK  0xFD
#define IS31_BANK_FUNCTION    0x0B
#define IS31_REG_SHUTDOWN     0x0A
#define IS31_REG_PICTURE_DISP 0x01
#define IS31_FRAME0_ONOFF     0x00
#define IS31_FRAME0_PWM       0x24

static void delay_us(uint16_t us) {
    while (us--) {
        __asm nop __endasm; __asm nop __endasm;
        __asm nop __endasm; __asm nop __endasm;
    }
}
static void delay_ms(uint16_t ms) { while (ms--) delay_us(1000); }

static void i2c_sda_high(void) { SDA_PIN = 1; }
static void i2c_sda_low(void)  { SDA_PIN = 0; }
static void i2c_scl_high(void) { SCL_PIN = 1; }
static void i2c_scl_low(void)  { SCL_PIN = 0; }

static void i2c_start(void) {
    i2c_sda_high(); i2c_scl_high(); delay_us(3);
    i2c_sda_low();  delay_us(3);
    i2c_scl_low();  delay_us(3);
}
static void i2c_stop(void) {
    i2c_sda_low(); delay_us(3);
    i2c_scl_high(); delay_us(3);
    i2c_sda_high(); delay_us(3);
}
static uint8_t i2c_write_byte(uint8_t b) {
    uint8_t i, ack;
    for (i = 0; i < 8; i++) {
        if (b & 0x80) i2c_sda_high(); else i2c_sda_low();
        b <<= 1;
        delay_us(2);
        i2c_scl_high(); delay_us(3);
        i2c_scl_low();  delay_us(2);
    }
    i2c_sda_high(); delay_us(2);
    i2c_scl_high(); delay_us(2);
    ack = (SDA_PIN == 0);
    i2c_scl_low(); delay_us(2);
    return ack;
}
static void i2c_write_reg(uint8_t addr7, uint8_t reg, uint8_t val) {
    i2c_start();
    i2c_write_byte((addr7 << 1) | 0);
    i2c_write_byte(reg);
    i2c_write_byte(val);
    i2c_stop();
}

static void is31_select_bank(uint8_t bank) {
    i2c_write_reg(ADDR_IS31FL3731, IS31_REG_CONFIG_BANK, bank);
}
static void is31_init(void) {
    uint8_t i;
    is31_select_bank(IS31_BANK_FUNCTION);
    i2c_write_reg(ADDR_IS31FL3731, IS31_REG_SHUTDOWN, 0x01);
    delay_ms(10);
    i2c_write_reg(ADDR_IS31FL3731, IS31_REG_PICTURE_DISP, 0x00);
    is31_select_bank(0x00);
    for (i = 0; i < 18; i++)
        i2c_write_reg(ADDR_IS31FL3731, IS31_FRAME0_ONOFF + i, 0xFF);
}
static void is31_set_led(uint8_t r, uint8_t c, uint8_t pwm) {
    uint8_t reg = IS31_FRAME0_PWM + (r * 16) + c;
    is31_select_bank(0x00);
    i2c_write_reg(ADDR_IS31FL3731, reg, pwm);
}
static void is31_clear(void) {
    uint8_t r, c;
    for (r = 0; r < 8; r++)
        for (c = 0; c < 8; c++)
            is31_set_led(r, c, 0);
}

static uint8_t sw1_pressed_edge(void) {
    static uint8_t last = 1;
    uint8_t now = SW1_PIN;
    uint8_t edge = (last == 1 && now == 0);
    last = now;
    return edge;
}
static uint8_t sw2_pressed_edge(void) {
    static uint8_t last = 1;
    uint8_t now = SW2_PIN;
    uint8_t edge = (last == 1 && now == 0);
    last = now;
    return edge;
}

static uint8_t frame[8];

static void frame_clear(void) {
    uint8_t r;
    for (r = 0; r < 8; r++) frame[r] = 0;
}
static void frame_set(uint8_t r, uint8_t c, uint8_t on) {
    if (on) frame[r] |= (1 << c);
    else    frame[r] &= ~(1 << c);
}
static uint8_t frame_get(uint8_t r, uint8_t c) {
    return (frame[r] >> c) & 1;
}
static void frame_draw(void) {
    uint8_t r, c;
    for (r = 0; r < 8; r++)
        for (c = 0; c < 8; c++)
            is31_set_led(r, c, frame_get(r, c) ? 0xFF : 0x00);
}

static uint16_t rnd_state = 0xACE1;
static uint8_t rnd8(void) {
    rnd_state ^= rnd_state << 7;
    rnd_state ^= rnd_state >> 9;
    rnd_state ^= rnd_state << 8;
    return (uint8_t)(rnd_state & 0xFF);
}

typedef enum { N, E, S, W } heading_t;

#define MAX_SNAKE 64
static int8_t snake_r[MAX_SNAKE];
static int8_t snake_c[MAX_SNAKE];
static uint8_t snake_len;
static heading_t heading;
static int8_t food_r, food_c;

static void spawn_food(void) {
    uint8_t ok;
    do {
        ok = 1;
        food_r = rnd8() & 0x07;
        food_c = rnd8() & 0x07;
        for (uint8_t i = 0; i < snake_len; i++)
            if (snake_r[i] == food_r && snake_c[i] == food_c) ok = 0;
    } while (!ok);
}

static void snake_reset(void) {
    snake_len = 3;
    snake_r[0] = 4; snake_c[0] = 4;
    snake_r[1] = 4; snake_c[1] = 3;
    snake_r[2] = 4; snake_c[2] = 2;
    heading = E;
    spawn_food();
}

static void snake_turn_left(void)  { heading = (heading == N) ? W : heading - 1; }
static void snake_turn_right(void) { heading = (heading == W) ? N : heading + 1; }

static uint8_t snake_step(void) {
    int8_t nr = snake_r[0], nc = snake_c[0];
    uint8_t i, grow;

    if (heading == N) nr--;
    else if (heading == S) nr++;
    else if (heading == E) nc++;
    else nc--;

    if (nr < 0 || nr > 7 || nc < 0 || nc > 7) return 0;

    for (i = 0; i < snake_len; i++)
        if (snake_r[i] == nr && snake_c[i] == nc) return 0;

    grow = (nr == food_r && nc == food_c);

    for (i = (grow ? snake_len : (uint8_t)(snake_len - 1)); i > 0; i--) {
        snake_r[i] = snake_r[i-1];
        snake_c[i] = snake_c[i-1];
    }
    snake_r[0] = nr; snake_c[0] = nc;
    if (grow) {
        if (snake_len < MAX_SNAKE) snake_len++;
        spawn_food();
    }
    return 1;
}

static void snake_draw(void) {
    uint8_t i;
    frame_clear();
    frame_set(food_r, food_c, 1);
    for (i = 0; i < snake_len; i++)
        frame_set(snake_r[i], snake_c[i], 1);
    frame_draw();
}

static void game_over_blink(void) {
    uint8_t i, r, c;
    for (i = 0; i < 4; i++) {
        for (r = 0; r < 8; r++)
            for (c = 0; c < 8; c++)
                is31_set_led(r, c, 0xFF);
        delay_ms(150);
        is31_clear();
        delay_ms(150);
    }
}

static void play_snake(void) {
    uint16_t tick_ms = 300;
    snake_reset();
    while (1) {
        if (sw1_pressed_edge()) snake_turn_left();
        if (sw2_pressed_edge()) snake_turn_right();

        if (!snake_step()) {
            game_over_blink();
            return;
        }
        snake_draw();
        delay_ms(tick_ms);
    }
}

static void play_catch(void) {
    uint16_t timeout_ms = 1500;
    uint16_t elapsed;
    uint8_t tr, tc;

    frame_clear(); frame_draw();
    delay_ms(300);

    while (1) {
        tr = rnd8() & 0x07;
        tc = rnd8() & 0x07;
        frame_clear();
        frame_set(tr, tc, 1);
        frame_draw();

        elapsed = 0;
        while (elapsed < timeout_ms) {
            if (sw1_pressed_edge()) {

                if (timeout_ms > 300) timeout_ms -= 100;
                delay_ms(80);
                break;
            }
            if (sw2_pressed_edge()) {
                return;
            }
            delay_ms(20);
            elapsed += 20;
        }
        if (elapsed >= timeout_ms) {
            game_over_blink();
            return;
        }
    }
}

static void show_menu_icon(uint8_t game_index) {

    frame_clear();
    if (game_index == 0) {

        frame_set(1,1,1); frame_set(1,2,1); frame_set(1,3,1);
        frame_set(2,3,1); frame_set(3,3,1);
        frame_set(3,2,1); frame_set(3,1,1);
        frame_set(4,1,1); frame_set(5,1,1);
        frame_set(5,2,1); frame_set(5,3,1);
    } else {

        frame_set(3,3,1); frame_set(3,4,1);
        frame_set(4,3,1); frame_set(4,4,1);
        frame_set(1,1,1); frame_set(6,6,1);
        frame_set(1,6,1); frame_set(6,1,1);
    }
    frame_draw();
}

void main(void) {
    uint8_t selected = 0;

    SDA_PIN = 1;
    SCL_PIN = 1;
    delay_ms(50);

    is31_init();
    is31_clear();

    while (1) {

        show_menu_icon(selected);
        if (sw2_pressed_edge()) {
            selected = (selected == 0) ? 1 : 0;
            delay_ms(150);
        }
        if (sw1_pressed_edge()) {
            delay_ms(150);
            if (selected == 0) play_snake();
            else               play_catch();
        }
        delay_ms(20);
    }
}
