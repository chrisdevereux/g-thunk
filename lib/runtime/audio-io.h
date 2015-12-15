//
//  audio-io.hpp
//  g-thunk
//
//  Created by Chris Devereux on 15/12/2015.
//
//

#ifndef audio_io_hpp
#define audio_io_hpp

typedef struct AudioIO AudioIO;

typedef struct AudioBuffer {
  float *data;
  uint64_t step;
} AudioBuffer;

typedef void(*AudioRenderLoop)(
  AudioBuffer const *inputs,
  AudioBuffer const *outputs,
  uint32_t inputCount,
  uint32_t outputCount,
  uint64_t frame_start,
  uint64_t frame_length,
  float sampleRate
);

AudioIO *audio_io_alloc();
void audio_io_dealloc(AudioIO *io);
int audio_io_start(AudioIO *io);
void audio_io_stop(AudioIO *io);

void audio_io_set_render_loop(AudioIO *io, AudioRenderLoop loop);

#endif /* audio_io_hpp */
