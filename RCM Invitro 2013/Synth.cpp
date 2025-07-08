
#include "Engine.hpp"
#include <SDL/SDL.h>

#define OSC_TYPE_NONE      0
#define OSC_TYPE_SAWTOOTH  1
#define OSC_TYPE_SQUARE    2

#define NOTE_A    0
#define NOTE_B    2
#define NOTE_C    3
#define NOTE_D    5
#define NOTE_E    7
#define NOTE_F    8
#define NOTE_G    10

#define REPEAT2(str) str str
#define REPEAT3(str) REPEAT2(str) str
#define REPEAT4(str) REPEAT2(str) REPEAT2(str)
#define REPEAT8(str) REPEAT4(str) REPEAT4(str)
#define REPEAT7(str) REPEAT4(str) REPEAT3(str)

struct ChannelEvent: ChannelState
{
   int channel;
   double time; // in samples

   ChannelEvent(): channel(0), time(0)
   {
   }

   bool operator<(const ChannelEvent& other) const
   {
      return time < other.time;
   }
};

struct Channel: ChannelState
{
   double buf1, buf2, buf3, buf4; // lowpass filter ladder

   Channel(): buf1(0), buf2(0), buf3(0), buf4(0)
   {
   }

   // cubic spline
   static double envelope(const double t, const double t0,
                        const double t1, const double t2, const double y)
   {
      if(t <= t0)
         return 0.0;
      else if(t <= t1)
      {
         const double x = (t - t0) / (t1 - t0);
         return ((3.0 - 2.0 * x) * x * x) * y;
      }
      else if(t <= t2)
      {
         const double x = 1.0 - (t - t1) / (t2 - t1);
         return ((3.0 - 2.0 * x) * x * x) * y;
      }
      return 0.0;
   }

   // boxfiltered sawtooth waveform
   static inline double sawtooth(const double t0, const double t1)
   {
      double a = fmod(t0, 1.0) * 2.0 - 1.0, b = fmod(t1, 1.0) * 2.0 - 1.0;

      a = (a * a) * 0.25;
      b = (b * b) * 0.25;

      return (b - a) / (t1 - t0); // sawtooth
   }

   // boxfiltered square waveform
   static inline double square(const double t0, const double t1)
   {
      // integral of square wave is pyramid
      double a = fmod(t0, 1.0), b = fmod(t1, 1.0);

      a = ((a <= 0.5) ? (1.0 - a * 4.0) : ((-1.0) + (a - 0.5) * 4.0)) * 0.25;
      b = ((b <= 0.5) ? (1.0 - b * 4.0) : ((-1.0) + (b - 0.5) * 4.0)) * 0.25;

      return (b - a) / (t1 - t0); // square
   }

   inline double osc(double t) const
   {
      return (t + t_offset) * freq * pow(freq_zoom + 1.0, t + t_offset);
   }

   double sample(const double t); // t is in samples
};

static const int noise_table_size = 16384 * 8;
static double noise_table[noise_table_size] = { 0.0 };

static const int num_channels = 32;
static Channel channels[num_channels];

static std::vector<ChannelEvent> channel_events;
static int current_event = 0, num_events = 0;

static int total_samples = 0;

static const int beats_per_minute = 130, beats_per_bar = 4, samplerate = 44100;

static const double samples_per_beat = 1.0 / double(beats_per_minute) * 60.0 * double(samplerate);

static const double global_amp_adjust = 1.2;

double Channel::sample(const double t) // t is in samples
{
   if(freq == 0.0 || amp == 0.0)
      return 0.0;

   double in = 0.0;

   switch(type)
   {
      case OSC_TYPE_SAWTOOTH:
         in = sawtooth(osc(t), osc(t + 1.0));
         break;

      case OSC_TYPE_SQUARE:
         in = square(osc(t), osc(t + 1.0));
         break;
   }

   const double n = noise_table[int(t) & (noise_table_size - 1)];

   in = std::max(-1.0, std::min(1.0, in + n * noise)); // pre clip

   const double c = use_cut_env ? envelope(t, cut_env_t0, cut_env_t1, cut_env_t2, cut) : cut;

   double resoclip = buf4;

   if(resoclip > 1.0)
      resoclip = 1.0;

   in -= resoclip * res;

   buf1 += (in - buf1) * c;
   buf2 += (buf1 - buf2) * c;
   buf3 += (buf2 - buf3) * c;
   buf4 += (buf3 - buf4) * c;

   double out = invert_filter ? (in - buf4) : buf4;

   out *= use_amp_env ? envelope(t, amp_env_t0, amp_env_t1, amp_env_t2, amp) : amp;

   return out;
}

void audioCallback(void *userdata, Uint8 *stream, int len)
{
   Sint16* samples = (Sint16*)stream;
   const int num_samples = len / (sizeof(Sint16) * 2);

   for(int i = 0; i < num_samples; ++i)
   {
      double out_l = 0.0, out_r = 0.0, time = double(total_samples);

      if(current_event < num_events)
      {
         while(current_event < num_events && channel_events[current_event].time <= time)
         {
            const ChannelEvent& e = channel_events[current_event];
            ((ChannelState&)channels[e.channel]) = (ChannelState&)e;
            ++current_event;
         }
      }

      for(int j = 0; j < num_channels; ++j)
      {
         Channel& ch = channels[j];
         double sample = ch.sample(time);
         out_l += sample * (ch.pan * -0.5 + 0.5);
         out_r += sample * (ch.pan * +0.5 + 0.5);
      }

      samples[i * 2 + 0] = Sint16(std::min(+1.0, std::max(-1.0, out_l)) * 32767.0);
      samples[i * 2 + 1] = Sint16(std::min(+1.0, std::max(-1.0, out_r)) * 32767.0);

      ++total_samples;
   }
}

// returns frequency in cycles per sample
static double frequencyForNote(const int note, const int octave, const double detune = 0.0)
{
   return 440.0 * pow(2.0, (double(note) + detune) / 12.0 + double(octave)) / double(samplerate);
}

// returns time in samples
static double barTime(const int bar, const int beat)
{
   return double(bar * beats_per_bar + beat) / double(beats_per_minute) * 60.0 * double(samplerate);
}

// returns time in samples
static double secondsTime(const double seconds)
{
   return seconds * double(samplerate);
}

static void addNotes(const char *const str, const ChannelState& st,
           const int first_ch, const int num_ch, const double time, const int divisor = 1)
{
   int ch = 0, beat = 0, octave = 0;
   const double detune = st.freq;
   ChannelEvent event;
   for(const char* c = str; *c; ++c)
   {
      if(!isalpha(c[0]))
      {
         if(isdigit(c[0]))
         {
            switch(c[0])
            {
               case '9': octave = -4; break; // ack...
               case '0': octave = -3; break;
               case '1': octave = -2; break;
               case '2': octave = -1; break;
               case '3': octave = +0; break;
               case '4': octave = +1; break;
               case '5': octave = +2; break;
            }
         }
         else if(c[0] == '-')
            beat += beats_per_bar;
         else if(c[0] == '.')
            beat += 1;

         continue;
      }

      int note = 0;
      int sharp_or_flat = 0; // +1 = sharp, 0 = natural, -1 = flat

      char c0 = toupper(c[0]), c1 = toupper(c[1]);


      if(c1 == '#')
         sharp_or_flat = +1;
      else if(c1 == '%')
         sharp_or_flat = -1;


      if(c0 == 'A')
         note = NOTE_A;
      else if(c0 == 'B')
         note = NOTE_B;
      else if(c0 == 'C')
         note = NOTE_C;
      else if(c0 == 'D')
         note = NOTE_D;
      else if(c0 == 'E')
         note = NOTE_E;
      else if(c0 == 'F')
         note = NOTE_F;
      else if(c0 == 'G')
         note = NOTE_G;


      // copy all data
      (ChannelState&)event = st;

      event.amp *= global_amp_adjust;

      event.time = time + (double(beat) * samples_per_beat) *
               ((divisor > 0) ? (1.0 / double(divisor)) : double(-divisor));

      event.channel = first_ch + (ch % num_ch);
      event.freq = frequencyForNote(note + sharp_or_flat, octave, detune);

      if(event.freq_zoom != 0.0) // this is a hack
         event.t_offset -= event.time;

      event.amp_env_t0 += event.time;
      event.amp_env_t1 += event.time;
      event.amp_env_t2 += event.time;

      event.cut_env_t0 += event.time;
      event.cut_env_t1 += event.time;
      event.cut_env_t2 += event.time;

      channel_events.push_back(event);
      ++ch;
      ++beat;
   }
}

int initAudio()
{
   SDL_AudioSpec desired;

   memset(&desired, 0, sizeof(desired));

   desired.freq = samplerate;
   desired.format =  AUDIO_S16LSB;
   desired.samples = 4096;
   desired.callback = audioCallback;
   desired.userdata = NULL;

   if(SDL_OpenAudio(&desired, NULL))
   {
      log("SDL_OpenAudio failed.\n");
      return -1;
   }

   for(int i = 0; i < noise_table_size; ++i)
      noise_table[i] = double(rand()) / double(RAND_MAX) * 2.0 - 1.0;

   const double drums_start_time = barTime(16, 0);

#if 1
   // first part

   // pad 1
   {
      ChannelState state;
      state.type = OSC_TYPE_SAWTOOTH;
      state.use_cut_env = true;
      state.use_amp_env = true;
      state.amp = 0.3;
      state.amp_env_t0 = 0.0;
      state.amp_env_t1 = 15000.0;
      state.amp_env_t2 = 200000.0;
      state.cut = 0.5;
      state.cut_env_t0 = 0.0;
      state.cut_env_t1 = 10000.0;
      state.cut_env_t2 = 300000.0;
      state.res = 0.0;
      state.pan = 0.1;
      state.freq = 0.0; // detune
      const char notes[] = REPEAT3(REPEAT2("2DA#1F.") "1G2A#A.1G2A#D.");
      addNotes(notes, state, 0, 4, barTime(0, 0), -4);
      state.freq = 0.1; // detune
      state.cut = 0.4;
      state.pan = -0.2;
      addNotes(notes, state, 4, 4, barTime(0, 0), -4);
   }

   // pad 2
   {
      ChannelState state;
      state.type = OSC_TYPE_SAWTOOTH;
      state.use_cut_env = true;
      state.use_amp_env = true;
      state.amp = 0.3;
      state.amp_env_t0 = 0.0;
      state.amp_env_t1 = 1000.0;
      state.amp_env_t2 = 200000.0;
      state.cut = 0.5;
      state.cut_env_t0 = 0.0;
      state.cut_env_t1 = 5000.0;
      state.cut_env_t2 = 200000.0;
      state.res = 0.2;
      state.pan = 0.3;
      state.freq = 0.12; // detune
      const char notes[] = REPEAT2(REPEAT2("1DA#0F.") "0G1A#A.0G1A#D.");
      addNotes(notes, state, 8, 4, barTime(16, 0), -4);
      state.freq = 0.14; // detune
      state.cut = 0.4;
      state.pan = -0.3;
      addNotes(notes, state, 12, 4, barTime(16, 0), -4);
   }

   // lead 1
   {
      ChannelEvent state;
      state.type = OSC_TYPE_SQUARE;
      state.use_cut_env = true;
      state.use_amp_env = true;
      state.invert_filter = false;
      state.amp = 0.3;
      state.amp_env_t0 = 0.0;
      state.amp_env_t1 = 50.0;
      state.amp_env_t2 = 20000.0;
      state.cut = 0.8;
      state.cut_env_t0 = 0.0;
      state.cut_env_t1 = 50.0;
      state.cut_env_t2 = 10000.0;
      state.res = 0.1;
      state.pan = -0.1;
      state.freq = 0.0; // detune

      const char notes[] = "..4GDA#.3G.D......." "..4GDA#.3G.D.....DG"
                           "4A#.........A....." "D..............."
                           "..4GDA#.3G.D......." "..4GDA#.3G.D.....DG"
                           "3A#.........A....." "2G"; // 8 bars

      addNotes(notes, state, 16, 2, barTime(32, 0), 4);
      state.pan = 0.0;
      state.amp = 0.2;
      state.cut = 0.6;
      addNotes(notes, state, 18, 2, barTime(32, 0) + barTime(0, 1), 4);
      state.pan = 0.4;
      state.amp = 0.2;
      state.cut = 0.4;
      addNotes(notes, state, 20, 2, barTime(32, 0) + barTime(0, 2), 4);

      const char notes2[] = REPEAT2("......3D4A3G......." "......3D4A3G......."
                           "......3D4A3G...5A4G.." "................");

      state.pan = -0.1;
      state.amp = 0.3;
      state.cut = 0.8;
      addNotes(notes2, state, 16, 2, barTime(32 + 8, 0), 4);
      state.pan = 0.0;
      state.amp = 0.2;
      state.cut = 0.6;
      addNotes(notes2, state, 18, 2, barTime(32 + 8, 0) + barTime(0, 1), 4);
      state.pan = 0.4;
      state.amp = 0.2;
      state.cut = 0.4;
      addNotes(notes2, state, 20, 2, barTime(32 + 8, 0) + barTime(0, 2), 4);

   }

   // hi-hat 2
   {
      ChannelEvent state;
      state.type = OSC_TYPE_NONE;
      state.use_cut_env = true;
      state.use_amp_env = true;
      state.invert_filter = true;
      state.amp = 0.25;
      state.amp_env_t0 = 0.0;
      state.amp_env_t1 = 5.0;
      state.amp_env_t2 = 800.0;
      state.cut = 0.95;
      state.cut_env_t0 = 0.0;
      state.cut_env_t1 = 5.0;
      state.cut_env_t2 = 10000.0;
      state.res = 0.0;
      state.pan = 0.4;
      state.freq = 0.0; // detune
      state.freq_zoom = 0.0;
      state.noise = 1.0;
      addNotes(REPEAT4(REPEAT8("AAAAAAAA")), state, 28, 1, barTime(16, 0) + drums_start_time, 4);
   }

   // kick
   {
      ChannelEvent state;
      state.type = OSC_TYPE_SQUARE;
      state.use_cut_env = true;
      state.use_amp_env = true;
      state.amp = 0.7;
      state.amp_env_t0 = 0.0;
      state.amp_env_t1 = 5.0;
      state.amp_env_t2 = 5000.0;
      state.cut = 0.5;
      state.cut_env_t0 = 0.0;
      state.cut_env_t1 = 5.0;
      state.cut_env_t2 = 10000.0;
      state.res = 0.4;
      state.pan = 0.0;
      state.freq = 0.0; // detune
      state.freq_zoom = -0.0001;
      state.noise = 0.3;
      addNotes(REPEAT4(REPEAT8("1A..A.A..")), state, 29, 1, barTime(0, 0) + drums_start_time, 2);
   }

   // hi-hat
   {
      ChannelEvent state;
      state.type = OSC_TYPE_NONE;
      state.use_cut_env = true;
      state.use_amp_env = true;
      state.invert_filter = true;
      state.amp = 0.3;
      state.amp_env_t0 = 0.0;
      state.amp_env_t1 = 5.0;
      state.amp_env_t2 = 1000.0;
      state.cut = 0.93;
      state.cut_env_t0 = 0.0;
      state.cut_env_t1 = 5.0;
      state.cut_env_t2 = 10000.0;
      state.res = 0.0;
      state.pan = 0.1;
      state.freq = 0.0; // detune
      state.freq_zoom = 0.0;
      state.noise = 1.0;
      addNotes(REPEAT4(REPEAT8("1A.AA.AAA")), state, 30, 1, barTime(0, 0) + drums_start_time, 2);
      state.amp = 0.3;
      state.pan = -0.1;
      state.amp_env_t0 = 0.0;
      state.amp_env_t1 = 5.0;
      state.amp_env_t2 = 2000.0;
      addNotes(REPEAT4(REPEAT8("1.A..A...")), state, 30, 1, barTime(0, 0) + drums_start_time, 2);
   }

   // snare
   {
      ChannelEvent state;
      state.type = OSC_TYPE_SQUARE;
      state.use_cut_env = true;
      state.use_amp_env = true;
      state.invert_filter = false;
      state.amp = 0.15;
      state.amp_env_t0 = 0.0;
      state.amp_env_t1 = 1.0;
      state.amp_env_t2 = 10000.0;
      state.cut = 0.9;
      state.cut_env_t0 = 0.0;
      state.cut_env_t1 = 1.0;
      state.cut_env_t2 = 10000.0;
      state.res = 0.0;
      state.pan = 0.1;
      state.freq = 0.0; // detune
      state.freq_zoom = -0.0001;
      state.noise = 2.0;
      addNotes(REPEAT4(REPEAT7("1....A.......A..A") "1....A.......AA.."), state, 31, 1, barTime(0, 0) + drums_start_time, 4);
   }

#endif

   // second part

   const int second_part_bar_start = 48, second_part_bar_start2 = 52;

   // pad 3
   {
      ChannelState state;
      state.type = OSC_TYPE_SAWTOOTH;
      state.use_cut_env = true;
      state.use_amp_env = true;
      state.amp = 0.4;
      state.amp_env_t0 = 0.0;
      state.amp_env_t1 = 15000.0;
      state.amp_env_t2 = 200000.0;
      state.cut = 0.5;
      state.cut_env_t0 = 0.0;
      state.cut_env_t1 = 10000.0;
      state.cut_env_t2 = 300000.0;
      state.res = 0.0;
      state.pan = 0.1;
      state.freq = 0.0; // detune
      const char notes[] = REPEAT2("2G.DCG.DC");
      addNotes(notes, state, 8, 2, barTime(second_part_bar_start2 + 8, 0), -4);
      state.freq = 0.1; // detune
      state.cut = 0.4;
      state.pan = 0.2;
      addNotes(notes, state, 10, 2, barTime(second_part_bar_start2 + 8, 0), -4);
   }

   // pad 4
   {
      ChannelState state;
      state.type = OSC_TYPE_SAWTOOTH;
      state.use_cut_env = true;
      state.use_amp_env = true;
      state.amp = 0.2;
      state.amp_env_t0 = 0.0;
      state.amp_env_t1 = 15000.0;
      state.amp_env_t2 = 400000.0;
      state.cut = 0.5;
      state.cut_env_t0 = 0.0;
      state.cut_env_t1 = 10000.0;
      state.cut_env_t2 = 600000.0;
      state.res = 0.0;
      state.pan = 0.1;
      state.freq = 0.05; // detune
      const char notes[] = REPEAT3("3G.DCG.DC");
      addNotes(notes, state, 12, 2, barTime(second_part_bar_start2 + 8, 0), -8);
      state.freq = 0.06; // detune
      state.cut = 0.4;
      state.pan = 0.2;
      addNotes(notes, state, 14, 2, barTime(second_part_bar_start2 + 8, 0), -8);
   }

   // BASS
   {
      ChannelState state;
      state.type = OSC_TYPE_SAWTOOTH;
      state.use_cut_env = true;
      state.use_amp_env = false;
      state.amp = 0.8;
      state.cut = 0.2;
      state.cut_env_t0 = 0.0;
      state.cut_env_t1 = 10.0;
      state.cut_env_t2 = 8000.0;
      state.res = 0.9;
      state.pan = -0.1;
      state.freq = 0.0; // detune
      const char notes[] = REPEAT4(
                              REPEAT2("0C.D.DD.D.D.DD.CC") "0C.D.DD.D.C.CC.CC" "9G.G.GG.G.G.GG.GG"
                              REPEAT2("0C.D.DD.D.D.DD.CC") "0FF.F.F.FF.FFE.EE" "0C.C.CC.C.C.CC.CC"
                              );
      addNotes(notes, state, 0, 8, barTime(second_part_bar_start2, 0), 4);
   }

   // lead 2
   {
      ChannelEvent state;
      state.type = OSC_TYPE_SQUARE;
      state.use_cut_env = true;
      state.use_amp_env = true;
      state.invert_filter = false;
      state.amp = 0.25;
      state.amp_env_t0 = 0.0;
      state.amp_env_t1 = 50.0;
      state.amp_env_t2 = 25000.0;
      state.cut = 0.8;
      state.cut_env_t0 = 0.0;
      state.cut_env_t1 = 50.0;
      state.cut_env_t2 = 15000.0;
      state.res = 0.1;
      state.pan = -0.1;
      state.freq = 0.0; // detune

      const char notes[] = REPEAT4("..G.D..CG......CF..G..4A.3G......."
                                   "..G.D..CG......4AC..3G..4A.3G.......");

      addNotes(notes, state, 16, 2, barTime(second_part_bar_start2, 0), 2);
      state.pan = 0.0;
      state.amp = 0.2;
      state.cut = 0.4;
      addNotes(notes, state, 18, 2, barTime(second_part_bar_start2, 0) + barTime(0, 2) * 0.33, 2);
      state.pan = 0.4;
      state.amp = 0.2;
      state.cut = 0.3;
      addNotes(notes, state, 20, 2, barTime(second_part_bar_start2, 0) + barTime(0, 2) * 0.33 * 2.0, 2);
   }

   const double drums_start_time2 = barTime(second_part_bar_start, 0);

   // hi-hat 2
   {
      ChannelEvent state;
      state.type = OSC_TYPE_NONE;
      state.use_cut_env = true;
      state.use_amp_env = true;
      state.invert_filter = true;
      state.amp = 0.25;
      state.amp_env_t0 = 0.0;
      state.amp_env_t1 = 5.0;
      state.amp_env_t2 = 800.0;
      state.cut = 0.95;
      state.cut_env_t0 = 0.0;
      state.cut_env_t1 = 5.0;
      state.cut_env_t2 = 10000.0;
      state.res = 0.0;
      state.pan = 0.4;
      state.freq = 0.0; // detune
      state.freq_zoom = 0.0;
      state.noise = 1.0;
      addNotes(REPEAT8(REPEAT8("AAAAAAAA")), state, 28, 1, drums_start_time2, 4);
   }

   // kick
   {
      ChannelEvent state;
      state.type = OSC_TYPE_SQUARE;
      state.use_cut_env = true;
      state.use_amp_env = true;
      state.amp = 0.7;
      state.amp_env_t0 = 0.0;
      state.amp_env_t1 = 5.0;
      state.amp_env_t2 = 5000.0;
      state.cut = 0.5;
      state.cut_env_t0 = 0.0;
      state.cut_env_t1 = 5.0;
      state.cut_env_t2 = 6000.0;
      state.res = 0.8;
      state.pan = 0.0;
      state.freq = 0.6; // detune
      state.freq_zoom = -0.0001;
      state.noise = 0.3;
      addNotes(REPEAT4(REPEAT8("1AA.....A.AA.....")), state, 29, 1, barTime(0, 0) + drums_start_time2, 4);
   }

   // hi-hat
   {
      ChannelEvent state;
      state.type = OSC_TYPE_NONE;
      state.use_cut_env = true;
      state.use_amp_env = true;
      state.invert_filter = true;
      state.amp = 0.3;
      state.amp_env_t0 = 0.0;
      state.amp_env_t1 = 5.0;
      state.amp_env_t2 = 1000.0;
      state.cut = 0.93;
      state.cut_env_t0 = 0.0;
      state.cut_env_t1 = 5.0;
      state.cut_env_t2 = 10000.0;
      state.res = 0.0;
      state.pan = 0.1;
      state.freq = 0.0; // detune
      state.freq_zoom = 0.0;
      state.noise = 1.0;
      addNotes(REPEAT4(REPEAT8("1A.A.A.A.")), state, 30, 1, barTime(0, 0) + drums_start_time2, 2);
      state.amp = 0.3;
      state.pan = -0.1;
      state.amp_env_t0 = 0.0;
      state.amp_env_t1 = 5.0;
      state.amp_env_t2 = 2000.0;
      addNotes(REPEAT4(REPEAT8("1.A.A.A.A")), state, 30, 1, barTime(0, 0) + drums_start_time2, 2);
   }

   // snare
   {
      ChannelEvent state;
      state.type = OSC_TYPE_SQUARE;
      state.use_cut_env = true;
      state.use_amp_env = true;
      state.invert_filter = false;
      state.amp = 0.2;
      state.amp_env_t0 = 0.0;
      state.amp_env_t1 = 1.0;
      state.amp_env_t2 = 10000.0;
      state.cut = 0.9;
      state.cut_env_t0 = 0.0;
      state.cut_env_t1 = 1.0;
      state.cut_env_t2 = 20000.0;
      state.res = 1.2;
      state.pan = -0.1;
      state.freq = 0.0; // detune
      state.freq_zoom = -0.0001;
      state.noise = 2.0;
      addNotes(REPEAT4(REPEAT8("1..A...A.")), state, 31, 1, barTime(0, 0) + drums_start_time2, 2);
   }



   std::sort(channel_events.begin(), channel_events.end());

   num_events = channel_events.size();

   SDL_PauseAudio(0);

   return 0;
}

void uninitAudio()
{
   SDL_PauseAudio(1);
}

