//
//  audio-io.cpp
//  g-thunk
//
//  Created by Chris Devereux on 15/12/2015.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "soundio/soundio.h"
#include "audio-io.h"

struct AudioIO {
  AudioRenderLoop renderloop;
  struct SoundIo *soundio;
  uint64_t framesElapsed;
};


AudioIO *audio_io_alloc() {
  return calloc(sizeof(struct AudioIO), 1);
}

static void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max)
{
  const struct SoundIoChannelLayout *layout = &outstream->layout;
  struct SoundIoChannelArea *areas;
  
  AudioIO *io = (AudioIO *)outstream->userdata;
  AudioRenderLoop render = io->renderloop;
  uint64_t elapsed = io->framesElapsed;
  
  AudioBuffer outputs[layout->channel_count];
  
  float float_sample_rate = outstream->sample_rate;
  int frames_left = frame_count_max;
  int err;
  
  while (frames_left > 0) {
    int frame_count = frames_left;
    
    if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
      fprintf(stderr, "%s\n", soundio_strerror(err));
      exit(1);
    }
    
    if (!frame_count) break;
    
    for (int i = 0; i < layout->channel_count; i++) {
      outputs[i] = (AudioBuffer) {
        .data = (float *)areas[i].ptr,
        .step = areas[i].step / sizeof(float)
      };
    }
    
    render(
      NULL,
      outputs,
      0,
      layout->channel_count,
      elapsed,
      frame_count,
      float_sample_rate
    );
    
    if ((err = soundio_outstream_end_write(outstream))) {
      fprintf(stderr, "%s\n", soundio_strerror(err));
      exit(1);
    }
    
    frames_left -= frame_count;
    elapsed += frame_count;
  }
  
  io->framesElapsed = elapsed;
}

int audio_io_start(AudioIO *io) {
  if (io->soundio) {
    fprintf(stderr, "already running!\n");
    return 1;
  }
  
  io->framesElapsed = 0;
  
  int err;
  io->soundio = soundio_create();
  if (!io->soundio) {
    fprintf(stderr, "out of memory\n");
    return 1;
  }
  
  if ((err = soundio_connect(io->soundio))) {
    fprintf(stderr, "error connecting: %s", soundio_strerror(err));
    return 1;
  }
  
  soundio_flush_events(io->soundio);
  
  int default_out_device_index = soundio_default_output_device_index(io->soundio);
  if (default_out_device_index < 0) {
    fprintf(stderr, "no output device found");
    return 1;
  }
  
  struct SoundIoDevice *device = soundio_get_output_device(io->soundio, default_out_device_index);
  if (!device) {
    fprintf(stderr, "out of memory");
    return 1;
  }
  
  fprintf(stderr, "Output device: %s\n", device->name);
  
  struct SoundIoOutStream *outstream = soundio_outstream_create(device);
  outstream->format = SoundIoFormatFloat32NE;
  outstream->write_callback = write_callback;
  outstream->userdata = io;
  
  if ((err = soundio_outstream_open(outstream))) {
    fprintf(stderr, "unable to open device: %s", soundio_strerror(err));
    return 1;
  }
  
  if (outstream->layout_error)
    fprintf(stderr, "unable to set channel layout: %s\n", soundio_strerror(outstream->layout_error));
  
  if ((err = soundio_outstream_start(outstream))) {
    fprintf(stderr, "unable to start device: %s", soundio_strerror(err));
    return 1;
  }
  
  return 0;
}

void audio_io_stop(AudioIO *io) {
  soundio_disconnect(io->soundio);
  soundio_destroy(io->soundio);
  io->soundio = NULL;
}

void audio_io_set_render_loop(AudioIO *io, AudioRenderLoop loop) {
  io->renderloop = loop;
}
