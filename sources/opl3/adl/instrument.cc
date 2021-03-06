//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "instrument.h"
#include "player.h"
#include <adlmidi.h>
#include <cstring>
#include <cassert>

#define EACH_INS_FIELD(F)                                               \
    F(note_offset1) F(note_offset2)                                     \
    F(midi_velocity_offset) F(second_voice_detune) F(percussion_key_number) \
    F(inst_flags)                                                       \
    F(fb_conn1_C0) F(fb_conn2_C0)                                       \
    F(delay_on_ms) F(delay_off_ms)

#define EACH_OP_FIELD(F)                        \
    F(avekf_20) F(ksl_l_40) F(atdec_60) F(susrel_80) F(waveform_E0)

Instrument Instrument::from_adlmidi(const ADL_Instrument &o) noexcept
{
    Instrument ins;
    std::memcpy(static_cast<ADL_Instrument *>(&ins), &o, sizeof(ADL_Instrument));
    return ins;
}

Instrument Instrument::from_wopl(const WOPLInstrument &o) noexcept
{
    Instrument ins;
    ins.version = ADLMIDI_InstrumentVersion;

    #define F(x) ins.x = o.x;
    EACH_INS_FIELD(F)
    #undef F

    for (unsigned op = 0; op < 4; ++op) {
        #define F(x) ins.operators[op].x = o.operators[op].x;
        EACH_OP_FIELD(F)
        #undef F
    }

    std::memcpy(ins.name, o.inst_name, 32);

    return ins;
}

WOPLInstrument Instrument::to_wopl() const noexcept
{
    WOPLInstrument ins = {};

    #define F(x) ins.x = this->x;
    EACH_INS_FIELD(F)
    #undef F

    for (unsigned op = 0; op < 4; ++op) {
        #define F(x) ins.operators[op].x = this->operators[op].x;
        EACH_OP_FIELD(F)
        #undef F
    }

    std::memcpy(ins.inst_name, name, 32);

    return ins;
}

void Instrument::describe(FILE *out) const noexcept
{
    fprintf(stderr,
            "Instrument\n"
            " - 4Op %u Ps4Op %u Blank %u\n"
            " - 1-2 Feedback %u Conn %u Tune %d\n"
            " - 3-4 Feedback %u Conn %u Tune %d\n"
            " - Velocity offset %d\n"
            " - Second voice fine tune %d\n"
            " - Percussion note %u\n",
            four_op(), pseudo_four_op(), blank(),
            fb12(), con12(), note_offset1,
            fb34(), con34(), note_offset2,
            midi_velocity_offset, second_voice_detune, percussion_key_number);
    for (unsigned op = 0; op < 4; ++op)
        describe_operator(op, out, "    ");
}

void Instrument::describe_operator(unsigned op, FILE *out, const char *indent) const noexcept
{
    const char *text = nullptr;
    switch (op) {
    case WOPL_OP_MODULATOR1: text = "Modulator 1"; break;
    case WOPL_OP_CARRIER1: text = "Carrier 1"; break;
    case WOPL_OP_MODULATOR2: text = "Modulator 2"; break;
    case WOPL_OP_CARRIER2: text = "Carrier 2"; break;
    }
    fprintf(stderr,
            "%sOperator %u: %s\n"
            "%s - ADSR %u %u %u %u\n"
            "%s - Level %u FMul %u KSL %u\n"
            "%s - Trem %u Vib %u Sus %u Env %u\n"
            "%s - Wave %u\n",
            indent, op, text,
            indent, attack(op), decay(op), sustain(op), release(op),
            indent, level(op), fmul(op), ksl(op),
            indent, trem(op), vib(op), sus(op), env(op),
            indent, wave(op));
}

bool Instrument::equal_instrument(const ADL_Instrument &o) const noexcept
{
    return !std::memcmp(static_cast<const ADL_Instrument *>(this), &o, sizeof(ADL_Instrument));
}

bool Instrument::equal_instrument_except_delays(const ADL_Instrument &o) const noexcept
{
    ADL_Instrument samedelay = o;
    samedelay.delay_on_ms = this->delay_on_ms;
    samedelay.delay_off_ms = this->delay_off_ms;
    return equal_instrument(samedelay);
}
