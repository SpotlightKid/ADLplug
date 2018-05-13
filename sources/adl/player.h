//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <adlmidi.h>
#include <string>
#include <vector>

#define EACH_PLAYER_TYPE(F, ...)                \
    F(OPL3, ##__VA_ARGS__)                      \
    /* F(OPN2, ##__VA_ARGS__) */

enum class Player_Type {
    #define ENUMVAL(x) x,
    EACH_PLAYER_TYPE(ENUMVAL)
    #undef ENUMVAL
};

template <Player_Type>
struct Player_Traits;

class Generic_Player
{
public:
    virtual ~Generic_Player() {}
    virtual Player_Type type() const = 0;
    virtual void init(unsigned sample_rate) = 0;
    virtual void close() = 0;
    virtual void reset() = 0;
    virtual void panic() = 0;
    virtual const char *emulator_name() const = 0;
    virtual void set_emulator(unsigned emu) = 0;
    virtual unsigned num_chips() = 0;
    virtual bool set_num_chips(unsigned chips) = 0;
    virtual void play_midi(const uint8_t *msg, unsigned len) = 0;
    virtual void generate(float *left, float *right, unsigned nframes, unsigned stride) = 0;
    virtual bool load_bank_file(const char *file) = 0;
};

std::vector<std::string> enumerate_emulators(Player_Type pt);
Generic_Player *instantiate_player(Player_Type pt);

static constexpr Player_Type all_player_types[] {
    #define ARRAYVAL(x) Player_Type::x,
    EACH_PLAYER_TYPE(ARRAYVAL)
    #undef ARRAYVAL
};
