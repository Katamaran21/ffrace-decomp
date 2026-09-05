#include <math.h>

#include "ff_physics.h"
#include "ff_lap.h"
#include "ff_opponent.h"
#include "ff_race.h"
#include "ff_racer.h"
#include "ff_settings.h"

/* FFRace.exe .data 0x00083308, indexed by 0x000832f8 and multiplied into the
   throttle gain by 0x0004f6bc. */
static const int ff_ship_accel[FF_SHIP_COUNT] = {
    0, 36, 42, 48, 54, 58, 60, 62, 63
};

/* FFRace.exe .data 0x00083330; 0x0004f6bc adds (0x00083384 - 2) * 4 to it. */
static const int ff_ship_top[FF_SHIP_COUNT] = {
    0, 38, 41, 43, 46, 48, 50, 53, 54
};

static int   ff_ship = 1;       /* 0x000832f8 */
static int   ff_ships_open = 1; /* 0x000833a4 */
static int   ff_ai_ship = 1;    /* 0x000a7674 */
static float ff_steer;          /* 0x000a77b4 */
static int   ff_throttle;       /* 0x000a7788 */
static int   ff_brake;          /* 0x000a778c */
static char  ff_left;           /* 0x00090279 */
static char  ff_right;          /* 0x00090281 */
static int   ff_damage;         /* 0x000a7670 */
static int   ff_finished;       /* 0x000a77cc */
static int   ff_countdown;      /* 0x000a7728 */
static int   ff_end_seg;        /* 0x000a7784 */
static int   ff_end_flag;       /* 0x000a77a8 */
static int   ff_end_chase;      /* 0x000a7668 */
static int   ff_hit_pending;    /* 0x000a7734 */
static int   ff_hit_seg;        /* 0x000865d0 + 4 * 0x000a7734 */
static int   ff_hit_sound;
static int   ff_end_sound;

void Physics_Reset(void)
{
    Racer_SetSpeed(FF_RACER_PLAYER, 0.0f);
    Racer_SetDerived(FF_RACER_PLAYER, 0.0f);
    ff_steer       = 0.0f;
    ff_throttle    = 0;
    ff_brake       = 0;
    ff_right       = 0;
    ff_left        = 0;
    ff_damage      = 0;
    ff_finished    = 0;
    ff_end_seg     = 0;
    ff_end_flag    = 0;
    ff_hit_pending = 0;
    ff_hit_seg     = 0;
    ff_hit_sound   = 0;
    ff_end_sound   = 0;
}

/* FFRace.exe 0x0004f6bc WM_KEYDOWN after 0x000506a8: key 0x000833c4, prompt
   0x000846fc "Turn Left", writes 0x00090279 = 1 and 0x00090281 = 0; key
   0x000833c8 the reverse; 0x000833d8 / 0x000833dc write 0x000a7788 /
   0x000a778c. */
void Physics_SetInput(int which, int down)
{
    switch (which) {
    case FF_INPUT_RIGHT:
        ff_right = (char)(down != 0);
        if (down)
            ff_left = 0;
        break;
    case FF_INPUT_LEFT:
        ff_left = (char)(down != 0);
        if (down)
            ff_right = 0;
        break;
    case FF_INPUT_THROTTLE:
        ff_throttle = (down != 0);
        if (down)
            ff_brake = 0;
        break;
    case FF_INPUT_BRAKE:
        ff_brake = (down != 0);
        if (down)
            ff_throttle = 0;
        break;
    default:
        break;
    }
}

int Physics_Input(int which)
{
    switch (which) {
    case FF_INPUT_RIGHT:    return ff_right;
    case FF_INPUT_LEFT:     return ff_left;
    case FF_INPUT_THROTTLE: return ff_throttle;
    case FF_INPUT_BRAKE:    return ff_brake;
    default:                return 0;
    }
}

/* FFRace.exe Screen_Set 0x00014924 keeps 0x000832f8 inside 1 .. 0x000833a4. */
void Physics_SetShip(int ship)
{
    if (ship < 1)
        ship = 1;
    if (ship > ff_ships_open)
        ship = ff_ships_open;
    if (ship >= FF_SHIP_COUNT)
        ship = FF_SHIP_COUNT - 1;

    ff_ship = ship;
}

int Physics_Ship(void)
{
    return ff_ship;
}

void Physics_SetShipsUnlocked(int count)
{
    if (count < 1)
        count = 1;
    if (count >= FF_SHIP_COUNT)
        count = FF_SHIP_COUNT - 1;

    ff_ships_open = count;
    if (ff_ship > ff_ships_open)
        ff_ship = ff_ships_open;
}

int Physics_ShipsUnlocked(void)
{
    return ff_ships_open;
}

/* FFRace.exe Race_Init 0x00013aec at dump 1147 writes 0x000a7674 = 0x000832f8;
   the racer loop of 0x000542a0 indexes 0x00083330 and 0x00083308 with it. */
void Physics_SetAiShip(int ship)
{
    ff_ai_ship = ship;
}

int Physics_AiShip(void)
{
    return ff_ai_ship;
}

int Physics_ShipTop(int ship)
{
    if (ship < 0)
        ship = 0;
    if (ship >= FF_SHIP_COUNT)
        ship = FF_SHIP_COUNT - 1;

    return ff_ship_top[ship];
}

int Physics_ShipAccel(int ship)
{
    if (ship < 0)
        ship = 0;
    if (ship >= FF_SHIP_COUNT)
        ship = FF_SHIP_COUNT - 1;

    return ff_ship_accel[ship];
}

void Physics_SetCountdown(int value)
{
    ff_countdown = value;
}

int Physics_Countdown(void)
{
    return ff_countdown;
}

/* FFRace.exe 0x0004f6bc between 0x00051690 and 0x00051d00: the top-speed test
   0x00083330[ship] + (0x00083384 - 2) * 4, then six speed terms in order. */
static void Speed_Step(void)
{
    int    top_raw = ff_ship_top[ff_ship] + (Settings_Difficulty() - 2) * 4;
    int    driven  = (ff_throttle == 1 || Settings_AutoAccel() == 1);
    float  speed   = Racer_Speed(FF_RACER_PLAYER);
    float  derived = Racer_Derived(FF_RACER_PLAYER);
    double gain;

    if (ff_hit_pending) {
        if (ff_hit_seg < Race_PlayerSegment())
            Race_SetPlayerSegment(ff_hit_seg);
        ff_hit_pending = 0;
        ff_hit_sound++;
        if (ff_finished == 0)
            ff_damage = (int)(speed + (float)ff_damage);
        speed -= 150.0f;
    }

    if ((double)(derived * 10.0f) < (double)(top_raw * 130) * (1.0 / 55.0) &&
        driven) {
        gain = 22.0 - ((double)((float)ff_left * derived) +
                       (double)((float)ff_right * derived)) * 0.5;
        speed = (float)(gain * (double)ff_ship_accel[ff_ship] *
                        (1.0 / 60.0) + (double)speed);
    }

    if (driven)
        speed = (float)((double)speed - (double)top_raw * 0.2);
    else
        speed = (float)((double)speed - (double)speed * (1.0 / 60.0));

    if (Race_Length() < Race_PlayerSegment() || ff_brake == 1 ||
        ff_finished == 1)
        speed -= 25.0f;

    if (speed < 0.0f)
        speed = 0.0f;

    derived = (float)sqrt((double)(speed * 0.1f));

    Racer_SetSpeed(FF_RACER_PLAYER, speed);
    Racer_SetDerived(FF_RACER_PLAYER, derived);
}

/* FFRace.exe 0x0004f6bc at 0x00051d00: 0x00090281 drives the falling ramp,
   0x00090279 the rising one, then the 0x000832c0 * -1.2 .. 1.2 clamp and the
   0x000a77b4 decay. */
static void Steer_Step(int dt_ms)
{
    double a     = (double)(float)dt_ms * 0.01;
    float  scale = Race_CurveScale();

    if (ff_right) {
        if (ff_ship < 0) {
            ff_steer = -1.0f;
        } else {
            ff_steer = (float)((double)ff_steer - a * 0.1);
            if ((double)ff_steer > (double)(-scale) * 0.5)
                ff_steer = (float)((double)ff_steer - a * 0.39);
            if (ff_steer > 0.0f)
                ff_steer = (float)((double)ff_steer - a * 0.46);
            if ((double)ff_steer < (double)(-scale) * 1.2)
                ff_steer = (float)((double)(-scale) * 1.2);
        }
    }

    if (ff_left) {
        if (ff_ship < 0) {
            ff_steer = 1.0f;
        } else {
            ff_steer = (float)((double)ff_steer + a * 0.1);
            if ((double)ff_steer < (double)scale * 0.5)
                ff_steer = (float)((double)ff_steer + a * 0.39);
            if (ff_steer < 0.0f)
                ff_steer = (float)((double)ff_steer + a * 0.46);
            if ((double)ff_steer > (double)scale * 1.2)
                ff_steer = (float)((double)scale * 1.2);
        }
    }

    if (!ff_left && !ff_right) {
        ff_steer = (float)((double)ff_steer - (a * (double)ff_steer) * 0.7);
        if ((double)ff_steer > -0.2 && (double)ff_steer < 0.2)
            ff_steer = 0.0f;
    }
}

/* FFRace.exe 0x0004f6bc at 0x000523b8, reached from the 0x00052320 gate. */
static void Steer_Integrate(int dt_ms)
{
    double a       = (double)(float)dt_ms * 0.01;
    double grip    = (double)Racer_Derived(FF_RACER_PLAYER) * 0.03 + 1.0;
    double sens    = (double)Settings_Sensitivity();
    double steer   = (double)ff_steer;
    float  dist    = Race_Dist();

    Race_SetCurve((float)(sens * steer * grip * a * 0.34 +
                          (double)Race_Curve()));
    Race_SetPlayerX((float)((double)Race_PlayerX() -
                            (((double)(dist / (float)FF_SEGMENT_LEN) + 0.6) *
                             sens * steer * grip * a * 0.34)));
}

/* FFRace.exe 0x0004f6bc at 0x00052320: steering is dropped once the road has
   drifted past +/-2 * 0x000832c0 on the side the player is turning to. */
static void Steer_Gate(int dt_ms)
{
    int   seg   = Race_PlayerSegment();
    float scale = Race_CurveScale();
    float off   = (Race_TrackCentre(seg + 1) + Race_Curve()) -
                  Race_TrackCentre(seg);

    if (ff_steer > 0.0f && !(off >= scale * 2.0f))
        Steer_Integrate(dt_ms);
    else if (ff_steer < 0.0f && off > scale * -2.0f)
        Steer_Integrate(dt_ms);
    else
        ff_steer = 0.0f;
}

/* FFRace.exe 0x0004f6bc between 0x000524c8 and 0x000526f0, run on every tick
   whether or not the gate above integrated. */
static void Centre_Step(int dt_ms)
{
    int    seg      = Race_PlayerSegment();
    float  next     = Race_TrackCentre(seg + 1);
    float  off      = (next + Race_Curve()) - Race_TrackCentre(seg);
    float  derived  = Racer_Derived(FF_RACER_PLAYER);
    double dt_a     = (double)(float)dt_ms * 0.01;
    double off_term = (double)off * 0.1;
    double frac     = ((double)((1.0f / (float)FF_SEGMENT_LEN) * Race_Dist()) +
                       0.6) * dt_a;

    Race_SetCurve((float)((double)Race_Curve() -
                          (double)derived * off_term * dt_a * 0.35));
    Race_SetPlayerX((float)((double)derived * off_term * frac * 0.35 +
                            (double)Race_PlayerX()));
}

/* FFRace.exe 0x0004f6bc writes 0x000a7670 = 0x9c4a, 0x000a7784, 0x000a77a8 and
   0x000a77cc before either wall response, and each of the two copies plays
   playSound(0x000a4db0, 0x000a4c20, 0x10000000) inside that gate. */
static void Wall_EndRun(void)
{
    int counter = Race_SegCounter();

    if (Race_Mode() == FF_RACE_ENDLESS && ff_end_seg == 0) {
        ff_damage  = 0x9c4a;
        ff_end_seg = counter;
        if (ff_end_flag < counter)
            ff_end_flag = counter;
        ff_finished = 1;
        ff_end_sound++;
    }
}

/* FFRace.exe 0x0004f6bc at 0x000532a8 biases 0x0008e32c by 0x000a77fc for the
   two wall tests and subtracts it again before the distance step. */
static void Wall_Step(void)
{
    int    seg      = Race_PlayerSegment();
    float  next     = Race_TrackCentre(seg + 1) + Race_Curve();
    int    half_raw = FF_ROAD_WIDTH - 1;
    float  half;
    float  slope    = next - Race_TrackCentre(seg);
    float  speed    = Racer_Speed(FF_RACER_PLAYER);
    float  interp;

    if (half_raw < 0)
        half_raw = FF_ROAD_WIDTH;
    half   = (float)(half_raw >> 1);
    interp = slope * (Race_Dist() * (1.0f / (float)FF_SEGMENT_LEN));

    if ((double)Race_PlayerX() < (double)((-next - half) - interp) - 0.2) {
        Wall_EndRun();
        ff_hit_sound++;
        interp = slope * (Race_Dist() / (float)FF_SEGMENT_LEN);
        Race_SetPlayerX((float)(0.4 - (double)(interp + half + next)));
        if (ff_finished == 0)
            ff_damage = (int)(speed + (float)ff_damage);
        speed -= 150.0f;
    }

    if ((double)Race_PlayerX() > (double)((half - next) - interp) + 0.2) {
        Wall_EndRun();
        ff_hit_sound++;
        interp = slope * (Race_Dist() / (float)FF_SEGMENT_LEN);
        Race_SetPlayerX((float)((double)((half - next) - interp) - 0.4));
        if (ff_finished == 0)
            ff_damage = (int)(speed + (float)ff_damage);
        speed -= 150.0f;
    }

    Racer_SetSpeed(FF_RACER_PLAYER, speed);
}

/* FFRace.exe 0x0004f6bc, the 0x000a779c == 2 block between 0x000526f0 and
   0x000532a8: 0x000a77cc, 0x000a7784 and 0x000a7668 latch once 0x000a76bc and
   0x000a7754 differ by more than 0x1e either way, and the 0x000a7698 store
   follows only while 0x000a7754 + 0x1e < 0x000a76bc; its 0x000a77d0 bump needs
   a setter this port does not model. */
static void Pursuit_End(void)
{
    int counter = Race_SegCounter();
    int chase   = Race_ChaseSegment();
    int ahead   = chase + 0x1e;

    if (Race_Mode() != FF_RACE_ENDLESS_2 || ff_finished != 0)
        return;
    if (ahead >= counter && counter + 0x1e >= chase)
        return;

    ff_finished  = 1;
    ff_end_seg   = counter;
    ff_end_chase = chase;

    Lap_CapturePlayer();

    if (ahead < counter)
        Lap_NoteWin();
}

/* FFRace.exe 0x0004f6bc returns after 0x0004e3c8 without touching the physics
   while 0x000a7728 is negative; the racer loop of 0x000542a0 follows the player
   pass of 0x00053430 inside that same guarded section. */
void Physics_Step(int dt_ms)
{
    if (ff_countdown < 0)
        return;

    Speed_Step();
    Steer_Step(dt_ms);
    Steer_Gate(dt_ms);
    Centre_Step(dt_ms);
    Pursuit_End();
    Wall_Step();

    Race_Advance((float)dt_ms * 0.001f *
                 Racer_Derived(FF_RACER_PLAYER) * 20.0f);
    Opponent_Step(dt_ms);
}

float Physics_Speed(void)
{
    return Racer_Speed(FF_RACER_PLAYER);
}

float Physics_Derived(void)
{
    return Racer_Derived(FF_RACER_PLAYER);
}

float Physics_Steer(void)
{
    return ff_steer;
}

int Physics_Damage(void)
{
    return ff_damage;
}

int Physics_Finished(void)
{
    return ff_finished;
}

void Physics_SetFinished(int finished)
{
    ff_finished = (finished != 0);
}

int Physics_EndSegment(void)
{
    return ff_end_seg;
}

int Physics_EndFlag(void)
{
    return ff_end_flag;
}

int Physics_EndChase(void)
{
    return ff_end_chase;
}

void Physics_NoteHit(int opponent_segment)
{
    ff_hit_pending = 1;
    ff_hit_seg     = opponent_segment;
}

int Physics_TakeHitSound(void)
{
    int pending = ff_hit_sound;

    ff_hit_sound = 0;
    return pending;
}

int Physics_TakeEndSound(void)
{
    int pending = ff_end_sound;

    ff_end_sound = 0;
    return pending;
}
