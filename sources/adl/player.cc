//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "adl/player.h"
#include <memory>
#include <assert.h>

template <Player_Type>
class Player;

template <>
struct Player_Traits<Player_Type::OPL3>
{
    typedef ADL_MIDIPlayer player;
    typedef ADLMIDI_AudioFormat audio_format;
    typedef ADLMIDI_SampleType sample_type;
    static constexpr sample_type sample_type_f32 = ADLMIDI_SampleType_F32;

    struct player_deleter {
        void operator()(player *p) const { adl_close(p); }
    };

    static const char *name() { return "ADLMIDI"; }
    static constexpr auto &version = adl_linkedLibraryVersion;
    static constexpr auto &init = adl_init;
    static constexpr auto &close = adl_close;
    static constexpr auto &reset = adl_reset;
    static constexpr auto &panic = adl_panic;
    static constexpr auto &emulator_name = adl_chipEmulatorName;
    static constexpr auto &get_num_chips = adl_getNumChips;
    static constexpr auto &set_num_chips = adl_setNumChips;
    static constexpr auto &switch_emulator = adl_switchEmulator;
    static constexpr auto &generate_format = adl_generateFormat;
    static constexpr auto &rt_note_on = adl_rt_noteOn;
    static constexpr auto &rt_note_off = adl_rt_noteOff;
    static constexpr auto &rt_note_aftertouch = adl_rt_noteAfterTouch;
    static constexpr auto &rt_channel_aftertouch = adl_rt_channelAfterTouch;
    static constexpr auto &rt_controller_change = adl_rt_controllerChange;
    static constexpr auto &rt_program_change = adl_rt_patchChange;
    static constexpr auto &rt_pitchbend_ml = adl_rt_pitchBendML;
    static constexpr auto &open_bank_file = adl_openBankFile;
};

template <Player_Type Pt>
class Player : public Generic_Player
{
    typedef Player_Traits<Pt> traits;
    typedef typename traits::player player_type;
    typedef typename traits::player_deleter player_deleter;
    std::unique_ptr<player_type, player_deleter> player_;

public:
    Player_Type type() const override { return Pt; }
    void init(unsigned sample_rate) override;
    void close() override
        { player_.reset(); }
    void reset() override
        { traits::reset(player_.get()); }
    void panic() override
        { traits::panic(player_.get()); }
    const char *emulator_name() const override
        { return traits::emulator_name(player_.get()); }
    void set_emulator(unsigned emu) override
        { traits::switch_emulator(player_.get(), emu); }
    unsigned num_chips() override
        { return traits::get_num_chips(player_.get()); }
    bool set_num_chips(unsigned chips) override
        { return traits::set_num_chips(player_.get(), chips) == 0; }
    void play_midi(const uint8_t *msg, unsigned len) override;
    void generate(float *left, float *right, unsigned nframes, unsigned stride) override;
    bool load_bank_file(const char *file) override
        { return traits::open_bank_file(player_.get(), file) >= 0; }
};

template <Player_Type Pt>
void Player<Pt>::init(unsigned sample_rate)
{
    player_type *pl = traits::init(sample_rate);
    if (!pl)
        throw std::runtime_error("cannot initialize player");
    player_.reset(pl);
}

template <Player_Type Pt>
void Player<Pt>::play_midi(const uint8_t *msg, unsigned len)
{
    player_type *pl = reinterpret_cast<player_type *>(player_.get());

    if (len <= 0)
        return;

    uint8_t status = msg[0];
    if ((status & 0xf0) == 0xf0)
        return;

    uint8_t channel = status & 0x0f;
    switch (status >> 4) {
    case 0b1001:
        if (len < 3) break;
        if (msg[2] != 0) {
            traits::rt_note_on(pl, channel, msg[1], msg[2]);
            break;
        }
    case 0b1000:
        if (len < 3) break;
        traits::rt_note_off(pl, channel, msg[1]);
        break;
    case 0b1010:
        if (len < 3) break;
        traits::rt_note_aftertouch(pl, channel, msg[1], msg[2]);
        break;
    case 0b1101:
        if (len < 2) break;
        traits::rt_channel_aftertouch(pl, channel, msg[1]);
        break;
    case 0b1011:
        if (len < 3) break;
        traits::rt_controller_change(pl, channel, msg[1], msg[2]);
        break;
    case 0b1100:
        if (len < 2) break;
        traits::rt_program_change(pl, channel, msg[1]);
        break;
    case 0b1110:
        if (len < 3) break;
        traits::rt_pitchbend_ml(pl, channel, msg[2], msg[1]);
        break;
    }
}

template <Player_Type Pt>
void Player<Pt>::generate(float *left, float *right, unsigned nframes, unsigned stride)
{
    typename traits::audio_format format;
    format.type = traits::sample_type_f32;
    format.containerSize = sizeof(float);
    format.sampleOffset = stride * sizeof(float);
    traits::generate_format(
        player_.get(), 2 * nframes, (uint8_t *)left, (uint8_t *)right, &format);
}

template <Player_Type Pt>
std::vector<std::string> enumerate_emulators()
{
    typedef Player_Traits<Pt> traits;
    typedef typename traits::player player;

    std::unique_ptr<player, typename traits::player_deleter> pl(traits::init(44100));
    std::vector<std::string> names;
    for (unsigned i = 0; traits::switch_emulator(pl.get(), i) == 0; ++i)
        names.push_back(traits::emulator_name(pl.get()));
    return names;
}

std::vector<std::string> enumerate_emulators(Player_Type pt)
{
    switch (pt) {
        default: assert(false); return {};
        #define CASE(x) case Player_Type::x: return enumerate_emulators<Player_Type::x>();
        EACH_PLAYER_TYPE(CASE);
        #undef CASE
    }
}

Generic_Player *instantiate_player(Player_Type pt)
{
    switch (pt) {
        default: return nullptr;
        #define CASE(x) case Player_Type::x: return new Player<Player_Type::x>;
        EACH_PLAYER_TYPE(CASE);
        #undef CASE
    }
}

